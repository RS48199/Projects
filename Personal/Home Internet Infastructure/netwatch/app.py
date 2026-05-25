#!/usr/bin/env python3
import os, time, json, sqlite3, threading, subprocess, ipaddress
from datetime import datetime, timedelta
from flask import Flask, jsonify, render_template, request
import urllib.request, urllib.error, base64, ssl

app = Flask(__name__)
DB_PATH = os.path.join(os.path.dirname(__file__), 'netwatch.db')

# --- CONFIG --- load from config.json if present
CFG_PATH = os.path.join(os.path.dirname(__file__), 'config.json')
def load_config():
    defaults = {
        "mikrotik_ip": "192.168.1.1",
        "mikrotik_user": "admin",
        "mikrotik_pass": "",
        "mikrotik_port": 80,
        "mikrotik_use_ssl": False,
        "isp_subnet": "192.168.1.0/24",
        "poll_interval_sec": 10,
        "arp_scan_interval_sec": 60,
        "db_retention_days": 7
    }
    if os.path.exists(CFG_PATH):
        with open(CFG_PATH) as f:
            saved = json.load(f)
        defaults.update(saved)
    return defaults

CFG = load_config()

def save_config(new_cfg):
    global CFG
    CFG.update(new_cfg)
    with open(CFG_PATH, 'w') as f:
        json.dump(CFG, f, indent=2)

# --- DATABASE ---
def db_connect():
    conn = sqlite3.connect(DB_PATH)
    conn.row_factory = sqlite3.Row
    return conn

def db_init():
    conn = db_connect()
    c = conn.cursor()
    c.executescript("""
        CREATE TABLE IF NOT EXISTS devices (
            mac TEXT PRIMARY KEY,
            ip TEXT,
            hostname TEXT,
            segment TEXT,
            first_seen INTEGER,
            last_seen INTEGER,
            is_online INTEGER DEFAULT 0
        );

        CREATE TABLE IF NOT EXISTS bandwidth (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            ts INTEGER NOT NULL,
            mac TEXT,
            ip TEXT,
            rx_bytes INTEGER DEFAULT 0,
            tx_bytes INTEGER DEFAULT 0,
            rx_rate INTEGER DEFAULT 0,
            tx_rate INTEGER DEFAULT 0
        );

        CREATE TABLE IF NOT EXISTS wan_stats (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            ts INTEGER NOT NULL,
            rx_bytes INTEGER DEFAULT 0,
            tx_bytes INTEGER DEFAULT 0,
            rx_rate INTEGER DEFAULT 0,
            tx_rate INTEGER DEFAULT 0
        );

        CREATE TABLE IF NOT EXISTS events (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            ts INTEGER NOT NULL,
            event_type TEXT,
            mac TEXT,
            ip TEXT,
            description TEXT
        );

        CREATE INDEX IF NOT EXISTS idx_bandwidth_ts ON bandwidth(ts);
        CREATE INDEX IF NOT EXISTS idx_wan_ts ON wan_stats(ts);
        CREATE INDEX IF NOT EXISTS idx_bandwidth_mac ON bandwidth(mac);
    """)
    conn.commit()
    conn.close()

def db_purge_old():
    cutoff = int(time.time()) - CFG['db_retention_days'] * 86400
    conn = db_connect()
    conn.execute("DELETE FROM bandwidth WHERE ts < ?", (cutoff,))
    conn.execute("DELETE FROM wan_stats WHERE ts < ?", (cutoff,))
    conn.execute("DELETE FROM events WHERE ts < ?", (cutoff,))
    conn.commit()
    conn.close()

# --- MIKROTIK REST API ---
def mikrotik_get(path):
    proto = 'https' if CFG['mikrotik_use_ssl'] else 'http'
    url = f"{proto}://{CFG['mikrotik_ip']}:{CFG['mikrotik_port']}/rest{path}"
    creds = base64.b64encode(f"{CFG['mikrotik_user']}:{CFG['mikrotik_pass']}".encode()).decode()
    req = urllib.request.Request(url, headers={
        'Authorization': f'Basic {creds}',
        'Content-Type': 'application/json'
    })
    ctx = ssl.create_default_context()
    ctx.check_hostname = False
    ctx.verify_mode = ssl.CERT_NONE
    try:
        with urllib.request.urlopen(req, timeout=5, context=ctx) as resp:
            return json.loads(resp.read().decode())
    except Exception as e:
        return None

# --- PREV COUNTERS for rate calculation ---
_prev_iface_counters = {}
_prev_accounting = {}
_prev_ts = {}

def poll_mikrotik():
    global _prev_iface_counters, _prev_accounting, _prev_ts
    now = int(time.time())

    # 1. WAN interface stats (ether1)
    ifaces = mikrotik_get('/interface')
    if ifaces:
        wan = next((i for i in ifaces if i.get('name') == 'ether1'), None)
        if wan:
            rx = int(wan.get('rx-byte', 0))
            tx = int(wan.get('tx-byte', 0))
            prev = _prev_iface_counters.get('ether1', {})
            prev_ts = _prev_ts.get('ether1', now)
            dt = max(now - prev_ts, 1)
            rx_rate = max(0, (rx - prev.get('rx', rx)) // dt) if prev else 0
            tx_rate = max(0, (tx - prev.get('tx', tx)) // dt) if prev else 0
            _prev_iface_counters['ether1'] = {'rx': rx, 'tx': tx}
            _prev_ts['ether1'] = now
            conn = db_connect()
            conn.execute("INSERT INTO wan_stats (ts, rx_bytes, tx_bytes, rx_rate, tx_rate) VALUES (?,?,?,?,?)",
                         (now, rx, tx, rx_rate, tx_rate))
            conn.commit()
            conn.close()

    # 2. Per-IP accounting (10.10.10.x devices)
    accounting = mikrotik_get('/ip/accounting/snapshot')
    if accounting:
        conn = db_connect()
        for entry in accounting:
            src = entry.get('src-address', '')
            dst = entry.get('dst-address', '')
            packets = int(entry.get('packets', 0))
            acct_bytes = int(entry.get('bytes', 0))
            # Only track Mikrotik LAN devices
            try:
                net = ipaddress.ip_network(CFG.get('mikrotik_lan', '10.10.10.0/24'), strict=False)
                src_ip = ipaddress.ip_address(src) if src else None
                if src_ip and src_ip in net:
                    prev_b = _prev_accounting.get(src, {}).get('bytes', acct_bytes)
                    prev_t = _prev_accounting.get(src, {}).get('ts', now)
                    dt = max(now - prev_t, 1)
                    rate = max(0, (acct_bytes - prev_b) // dt) if src in _prev_accounting else 0
                    _prev_accounting.setdefault(src, {})
                    _prev_accounting[src]['bytes'] = acct_bytes
                    _prev_accounting[src]['ts'] = now
                    # Lookup MAC for this IP
                    row = conn.execute("SELECT mac FROM devices WHERE ip=?", (src,)).fetchone()
                    mac = row['mac'] if row else 'unknown'
                    conn.execute("INSERT INTO bandwidth (ts, mac, ip, tx_bytes, tx_rate) VALUES (?,?,?,?,?)",
                                 (now, mac, src, acct_bytes, rate))
            except Exception:
                pass
        conn.commit()
        conn.close()

    # 3. ARP table — discover Mikrotik LAN devices
    arp = mikrotik_get('/ip/arp')
    if arp:
        conn = db_connect()
        for entry in arp:
            ip = entry.get('address', '')
            mac = entry.get('mac-address', '').upper()
            iface = entry.get('interface', '')
            if not mac or mac == '00:00:00:00:00:00':
                continue
            existing = conn.execute("SELECT * FROM devices WHERE mac=?", (mac,)).fetchone()
            if existing:
                if existing['ip'] != ip:
                    conn.execute("INSERT INTO events (ts,event_type,mac,ip,description) VALUES (?,?,?,?,?)",
                                 (now, 'ip_change', mac, ip, f"IP changed from {existing['ip']} to {ip}"))
                conn.execute("UPDATE devices SET ip=?, last_seen=?, is_online=1, segment='mikrotik' WHERE mac=?",
                             (ip, now, mac))
            else:
                conn.execute("INSERT INTO devices (mac,ip,hostname,segment,first_seen,last_seen,is_online) VALUES (?,?,?,?,?,?,1)",
                             (mac, ip, '', 'mikrotik', now, now))
                conn.execute("INSERT INTO events (ts,event_type,mac,ip,description) VALUES (?,?,?,?,?)",
                             (now, 'new_device', mac, ip, f"New device on Mikrotik LAN: {mac}"))
        conn.commit()
        conn.close()

def arp_scan_isp():
    now = int(time.time())
    subnet = CFG.get('isp_subnet', '192.168.1.0/24')
    try:
        result = subprocess.run(['arp-scan', '--localnet', '-q'], capture_output=True, text=True, timeout=30)
        lines = result.stdout.strip().split('\n')
        conn = db_connect()
        found_macs = set()
        for line in lines:
            parts = line.split('\t')
            if len(parts) >= 2:
                ip = parts[0].strip()
                mac = parts[1].strip().upper()
                hostname = parts[2].strip() if len(parts) > 2 else ''
                if not mac or len(mac) < 10:
                    continue
                found_macs.add(mac)
                existing = conn.execute("SELECT * FROM devices WHERE mac=?", (mac,)).fetchone()
                if existing:
                    conn.execute("UPDATE devices SET ip=?, last_seen=?, is_online=1 WHERE mac=?", (ip, now, mac))
                else:
                    conn.execute("INSERT INTO devices (mac,ip,hostname,segment,first_seen,last_seen,is_online) VALUES (?,?,?,?,?,?,1)",
                                 (mac, ip, hostname, 'wifi', now, now))
                    conn.execute("INSERT INTO events (ts,event_type,mac,ip,description) VALUES (?,?,?,?,?)",
                                 (now, 'new_device', mac, ip, f"New device on WiFi: {mac} ({hostname})"))
        # Mark unseen as offline (not in Mikrotik segment)
        conn.execute("""UPDATE devices SET is_online=0 WHERE segment='wifi' AND mac NOT IN ({})
                        AND last_seen < ?""".format(','.join('?'*len(found_macs))) if found_macs else
                     "UPDATE devices SET is_online=0 WHERE segment='wifi' AND last_seen < ?",
                     (list(found_macs) + [now - 120]) if found_macs else [now - 120])
        conn.commit()
        conn.close()
    except FileNotFoundError:
        # arp-scan not installed, fallback to /proc/net/arp
        try:
            with open('/proc/net/arp') as f:
                lines = f.readlines()[1:]
            conn = db_connect()
            for line in lines:
                parts = line.split()
                if len(parts) >= 4:
                    ip, _, flags, mac = parts[0], parts[1], parts[2], parts[3]
                    if mac == '00:00:00:00:00:00' or flags == '0x0':
                        continue
                    mac = mac.upper()
                    existing = conn.execute("SELECT * FROM devices WHERE mac=?", (mac,)).fetchone()
                    if existing:
                        conn.execute("UPDATE devices SET ip=?, last_seen=?, is_online=1 WHERE mac=?", (ip, now, mac))
                    else:
                        conn.execute("INSERT INTO devices (mac,ip,hostname,segment,first_seen,last_seen,is_online) VALUES (?,?,?,?,?,?,1)",
                                     (mac, ip, '', 'wifi', now, now))
            conn.commit()
            conn.close()
        except Exception:
            pass
    except Exception:
        pass

# --- POLLING THREAD ---
_stop_event = threading.Event()

def polling_loop():
    last_arp_scan = 0
    last_purge = 0
    while not _stop_event.is_set():
        try:
            poll_mikrotik()
        except Exception as e:
            pass
        now = time.time()
        if now - last_arp_scan > CFG['arp_scan_interval_sec']:
            try:
                arp_scan_isp()
            except Exception:
                pass
            last_arp_scan = now
        if now - last_purge > 3600:
            try:
                db_purge_old()
            except Exception:
                pass
            last_purge = now
        _stop_event.wait(CFG['poll_interval_sec'])

# --- API ROUTES ---
@app.route('/')
def index():
    return render_template('index.html')

@app.route('/api/status')
def api_status():
    conn = db_connect()
    devices = [dict(r) for r in conn.execute("SELECT * FROM devices ORDER BY segment, last_seen DESC").fetchall()]
    total = len(devices)
    online = sum(1 for d in devices if d['is_online'])

    wan = conn.execute("SELECT * FROM wan_stats ORDER BY ts DESC LIMIT 1").fetchone()
    wan_data = dict(wan) if wan else {}

    recent_events = [dict(r) for r in conn.execute(
        "SELECT * FROM events ORDER BY ts DESC LIMIT 20").fetchall()]

    conn.close()
    return jsonify({
        'devices': devices,
        'total_devices': total,
        'online_devices': online,
        'wan': wan_data,
        'events': recent_events,
        'ts': int(time.time())
    })

@app.route('/api/bandwidth/history')
def api_bandwidth_history():
    period = request.args.get('period', '1h')
    mac = request.args.get('mac', None)
    periods = {'1h': 3600, '6h': 21600, '24h': 86400, '7d': 604800}
    secs = periods.get(period, 3600)
    since = int(time.time()) - secs

    conn = db_connect()
    if mac:
        rows = conn.execute(
            "SELECT ts, rx_rate, tx_rate, rx_bytes, tx_bytes FROM bandwidth WHERE mac=? AND ts>? ORDER BY ts",
            (mac, since)).fetchall()
    else:
        rows = conn.execute(
            "SELECT ts, SUM(rx_rate) as rx_rate, SUM(tx_rate) as tx_rate FROM bandwidth WHERE ts>? GROUP BY ts ORDER BY ts",
            (since,)).fetchall()
    conn.close()
    return jsonify([dict(r) for r in rows])

@app.route('/api/wan/history')
def api_wan_history():
    period = request.args.get('period', '1h')
    periods = {'1h': 3600, '6h': 21600, '24h': 86400, '7d': 604800}
    secs = periods.get(period, 3600)
    since = int(time.time()) - secs
    conn = db_connect()
    rows = conn.execute(
        "SELECT ts, rx_rate, tx_rate FROM wan_stats WHERE ts>? ORDER BY ts", (since,)).fetchall()
    conn.close()
    return jsonify([dict(r) for r in rows])

@app.route('/api/device/<mac>/rename', methods=['POST'])
def rename_device(mac):
    data = request.json
    name = data.get('hostname', '').strip()[:64]
    conn = db_connect()
    conn.execute("UPDATE devices SET hostname=? WHERE mac=?", (name, mac))
    conn.commit()
    conn.close()
    return jsonify({'ok': True})

@app.route('/api/config', methods=['GET', 'POST'])
def api_config():
    if request.method == 'POST':
        data = request.json
        allowed = ['mikrotik_ip', 'mikrotik_user', 'mikrotik_pass', 'mikrotik_port',
                   'mikrotik_use_ssl', 'isp_subnet', 'poll_interval_sec',
                   'arp_scan_interval_sec', 'mikrotik_lan', 'db_retention_days']
        new_cfg = {k: data[k] for k in allowed if k in data}
        save_config(new_cfg)
        return jsonify({'ok': True})
    safe = {k: v for k, v in CFG.items() if k != 'mikrotik_pass'}
    safe['mikrotik_pass'] = '***' if CFG.get('mikrotik_pass') else ''
    return jsonify(safe)

@app.route('/api/mikrotik/test')
def api_test_mikrotik():
    res = mikrotik_get('/system/resource')
    if res:
        return jsonify({'ok': True, 'data': res})
    return jsonify({'ok': False, 'error': 'Cannot reach Mikrotik REST API'}), 503

if __name__ == '__main__':
    db_init()
    t = threading.Thread(target=polling_loop, daemon=True)
    t.start()
    app.run(host='0.0.0.0', port=5000, debug=False)
