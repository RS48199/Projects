# NetWatch

Network control plane dashboard for Raspberry Pi Zero W.
Monitors Mikrotik LAN devices (bandwidth, presence) and ISP WiFi device presence.

## Quick Start

### 1. Mikrotik prep (run on Mikrotik terminal)

Enable REST API and IP accounting:
```
/ip/service/set www enabled=yes port=80
/ip accounting set enabled=yes
/ip accounting web-access set accessible-via-web=yes
```

Create a read-only API user (recommended over using admin):
```
/user/group/add name=netwatch policy=read,api,!write,!policy,!test,!winbox,!password,!web,!sniff,!sensitive,!romon
/user/add name=netwatch password=YOURPASSWORD group=netwatch
```

### 2. Deploy to Pi

Copy the netwatch/ folder to your Pi, then:
```bash
cd netwatch
bash install.sh
```

### 3. Configure

Edit `/home/pi/netwatch/config.json`:
```json
{
  "mikrotik_ip": "192.168.1.X",      <- Your Mikrotik IP on ISP subnet
  "mikrotik_user": "netwatch",
  "mikrotik_pass": "YOURPASSWORD",
  "mikrotik_port": 80,
  "mikrotik_use_ssl": false,
  "mikrotik_lan": "10.10.10.0/24",   <- Your Mikrotik downstream subnet
  "isp_subnet": "192.168.1.0/24",    <- Your ISP router subnet
  "poll_interval_sec": 10,
  "arp_scan_interval_sec": 60,
  "db_retention_days": 7
}
```

Then restart:
```bash
sudo systemctl restart netwatch
```

### 4. Access

Open browser on any LAN device:
```
http://<pi-ip>:5000
```

## Architecture

```
Mikrotik REST API (/rest/ip/arp, /rest/ip/accounting, /rest/interface)
    └─► app.py polling thread (10s interval)
            └─► SQLite (netwatch.db, 7-day rolling)
                    └─► Flask API (/api/status, /api/wan/history, etc.)
                            └─► Web dashboard (single-page, Chart.js)

/proc/net/arp or arp-scan ──► ISP subnet device presence
```

## What it monitors

| Data | Source | Notes |
|---|---|---|
| Mikrotik LAN device list | /rest/ip/arp | Full — IP, MAC, online status |
| Per-IP traffic (Mikrotik LAN) | /rest/ip/accounting | Requires IP accounting enabled |
| WAN throughput | /rest/interface (ether1) | Total in/out rate |
| WiFi device presence | arp-scan / /proc/net/arp | Presence only, no traffic data |

## Known limitations

- WiFi devices on ISP subnet: presence only, no traffic data (ISP router is opaque)
- Mikrotik IP accounting resets on reboot — counters are relative, not absolute
- Pi Zero W WiFi is single-band 2.4GHz — if Pi is far from router, add a powered USB-Ethernet adapter for reliability

## Troubleshooting

**Can't reach Mikrotik API:**
- Verify `www` service is enabled on port 80: `/ip/service/print`
- Confirm Pi can reach Mikrotik IP: `ping <mikrotik-ip>`
- Check credentials in config.json

**No bandwidth data:**
- Confirm IP accounting is enabled: `/ip accounting print`
- Wait 1-2 poll cycles after enabling

**arp-scan not finding devices:**
- Run `sudo arp-scan --localnet` manually on Pi to verify
- May need `sudo` — if so, add `sudoers` entry for `arp-scan`
