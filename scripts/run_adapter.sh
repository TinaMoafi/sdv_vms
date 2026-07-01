#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

export VSOMEIP_CONFIGURATION="$PWD/config/vsomeip/adapter.json"
export VSOMEIP_APPLICATION_NAME="lidar_adapter"

./adapter_someip/build/lidar_adapter