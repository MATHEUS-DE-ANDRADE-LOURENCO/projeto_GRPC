#!/usr/bin/env bash
# Teste automático ponta a ponta: build, servidor (bg), batch Python, verificação de saídas.
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT_DIR"

log() { echo "[tests] $*"; }

ensure_env() { bash scripts/env_setup.sh; }

build_all() {
  log "Gerando código gRPC C++ (protoc)..."
  protoc -I=proto \
    --cpp_out=config_cpp \
    --grpc_out=config_cpp \
    --plugin=protoc-gen-grpc=$(which grpc_cpp_plugin) \
    proto/file_processor.proto

  log "Compilando servidor e cliente C++..."
  g++ -std=c++17 server_cpp/servidor.cpp \
    config_cpp/file_processor.pb.cc config_cpp/file_processor.grpc.pb.cc \
    -Iconfig_cpp $(pkg-config --cflags --libs grpc++) $(pkg-config --cflags --libs protobuf) \
    -o server_cpp/servidor

  g++ -std=c++17 client_cpp/cliente.cpp \
    config_cpp/file_processor.pb.cc config_cpp/file_processor.grpc.pb.cc \
    -Iconfig_cpp $(pkg-config --cflags --libs grpc++) $(pkg-config --cflags --libs protobuf) \
    -o client_cpp/cliente
}

prepare_samples() {
  mkdir -p client_python/storage client_cpp/storage server_cpp/storage
  # PNG
  if [[ ! -f client_python/storage/sample.png ]]; then
    log "Criando PNG de amostra..."
    convert -size 32x32 xc:red client_python/storage/sample.png || true
    if [[ ! -f client_python/storage/sample.png ]]; then
      base64 -d > client_python/storage/sample.png << 'EOF' || true
iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR4nGNgYAAAAAMAAWgmWQ0AAAAASUVORK5CYII=
EOF
    fi
  fi
  cp -f client_python/storage/sample.png client_cpp/storage/sample.png

  # PDF
  if [[ ! -f client_python/storage/sample.pdf ]]; then
    log "Criando PDF de amostra..."
    if command -v convert >/dev/null 2>&1; then
      convert -size 100x100 xc:white -gravity center -pointsize 18 \
        -annotate 0 "Sample" client_python/storage/sample.pdf || true
    fi
    if [[ ! -f client_python/storage/sample.pdf ]]; then
      base64 -d > client_python/storage/sample.pdf << 'EOF' || true
JVBERi0xLjQKJeLjz9MKMSAwIG9iago8PCAvVHlwZSAvQ2F0YWxvZyAvUGFnZXMgMiAwIFIgPj4KZW5kb2JqCjIgMCBvYmoKPDwgL1R5cGUgL1BhZ2VzIC9LaWRzIFszIDAgUiBdIC9Db3VudCAxID4+CmVuZG9iagozIDAgb2JqCjw8IC9UeXBlIC9QYWdlIC9QYXJlbnQgMiAwIFIgL1Jlc291cmNlcyA8PCA+PiAvTWVkaWFCb3hbMCAwIDYxMiA3OTJdIC9Db250ZW50cyA0IDAgUiA+PgplbmRvYmoKNCAwIG9iago8PCAvTGVuZ3RoIDYxID4+CnN0cmVhbQpCBTAgVGYKMTAwIDcwMCBUZAooU2FtcGxlIFBERikgVGoKRVQKZW5kc3RyZWFtCmVuZG9iagp4cmVmCjAgNQowMDAwMDAwMDAwIDY1NTM1IGYgCjAwMDAwMDAxMTAgMDAwMDAgbiAKMDAwMDAwMDA2MCAwMDAwMCBuIAowMDAwMDAwMTYzIDAwMDAwIG4gCjAwMDAwMDAyNTEgMDAwMDAgbiAKdHJhaWxlcgo8PCAvU2l6ZSA1IC9Sb290IDEgMCBSIC9JRCBbPEU4QjYyMzQ1RjY0RkQzQ0Y+PEU4QjYyMzQ1RjY0RkQzQ0Y+XSA+PgpzdGFydHhyZWYKMzE0CiUlRU9GCiA=
EOF
    fi
  fi
  cp -f client_python/storage/sample.pdf client_cpp/storage/sample.pdf
}

start_server_bg() {
  log "Iniciando servidor em background..."
  server_cpp/servidor "0.0.0.0:50051" > server_cpp/server.out 2>&1 &
  echo $! > server_cpp/server.pid
  # Espera porta abrir rapidamente
  sleep 1
}

stop_server_bg() {
  if [[ -f server_cpp/server.pid ]]; then
    local pid
    pid=$(cat server_cpp/server.pid || true)
    if [[ -n "${pid}" ]] && ps -p "${pid}" >/dev/null 2>&1; then
      log "Parando servidor (pid ${pid})..."
      kill "${pid}" || true
      wait "${pid}" 2>/dev/null || true
    fi
    rm -f server_cpp/server.pid
  fi
}

run_batch_client() {
  log "Executando cliente Python em modo batch..."
  PYTHONPATH="${ROOT_DIR}" python3 client_python/batch.py
}

verify_outputs() {
  log "Verificando saídas no storage do cliente Python..."
  shopt -s nullglob
  local out_dir="client_python/storage"
  local files=("${out_dir}"/*)
  if (( ${#files[@]} == 0 )); then
    echo "Nenhum arquivo de saída encontrado em ${out_dir}" >&2
    return 1
  fi
  printf "Arquivos gerados (%d):\n" "${#files[@]}"
  for f in "${files[@]}"; do
    printf " - %s (%d bytes)\n" "$f" "$(stat -c%s "$f" 2>/dev/null || wc -c < "$f")"
  done
  return 0
}

main() {
  trap stop_server_bg EXIT
  ensure_env
  build_all
  prepare_samples
  start_server_bg
  run_batch_client
  verify_outputs
  log "Teste concluído com sucesso. Verifique também o log: server_cpp/server.log"
}

main "$@"
