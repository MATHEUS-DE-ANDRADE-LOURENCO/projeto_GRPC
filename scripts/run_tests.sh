#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."

echo "[tests] Preparando amostras..."
bash scripts/prepare_samples.sh

echo "[tests] Instalando dependências Python (se necessário)..."
python3 -m pip -q install -r client_python/requirements.txt || true

echo "[tests] Build C++..."
bash scripts/build_cpp.sh

echo "[tests] Iniciando servidor em background..."
server_cpp/servidor &
SERVER_PID=$!
trap 'kill ${SERVER_PID} 2>/dev/null || true' EXIT
sleep 1

echo "[tests] Executando cliente Python em modo batch..."
PYTHONPATH=. python3 client_python/batch.py

echo "[tests] Listando arquivos gerados em client_python/storage:"
ls -la client_python/storage || true

echo "[tests] Encerrando servidor..."
kill ${SERVER_PID}
wait ${SERVER_PID} 2>/dev/null || true
echo "[tests] Concluído."
