#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
if [[ ! -x server_cpp/servidor ]]; then
  bash scripts/build_cpp.sh
fi
echo "[run] Iniciando servidor em 0.0.0.0:50051"
exec server_cpp/servidor
