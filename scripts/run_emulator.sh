#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

python3 emulator/lidar_sensor_emulator.py \
  --udp-ip 127.0.0.1 \
  --udp-port 25000 \
  --delay 0.1