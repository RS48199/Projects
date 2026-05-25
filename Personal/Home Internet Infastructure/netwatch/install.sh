#!/bin/bash
# NetWatch setup script — run as pi user on the Pi Zero W
# Usage: bash install.sh

set -e
echo "[netwatch] Installing dependencies..."
sudo apt-get update -q
sudo apt-get install -y python3-flask arp-scan

echo "[netwatch] Copying files..."
DEST="/home/pi/netwatch"
mkdir -p "$DEST/templates"
cp app.py "$DEST/"
cp config.json "$DEST/"
cp -r templates/ "$DEST/templates/"

echo "[netwatch] Installing systemd service..."
sudo cp netwatch.service /etc/systemd/system/netwatch.service
sudo systemctl daemon-reload
sudo systemctl enable netwatch.service
sudo systemctl restart netwatch.service

echo ""
echo "[netwatch] Done."
echo "[netwatch] Edit $DEST/config.json with your Mikrotik IP and credentials."
echo "[netwatch] Then: sudo systemctl restart netwatch"
echo "[netwatch] Dashboard available at: http://$(hostname -I | awk '{print $1}'):5000"
