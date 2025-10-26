#!/usr/bin/env bash
# Execução dos clientes (Python e C++) com preparação de ambiente/build e amostras.
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT_DIR"

ensure_env() {
  bash scripts/env_setup.sh
}

build_cpp_client() {
  echo "[clients] Gerando código gRPC C++ (protoc) para cliente..."
  protoc -I=proto \
    --cpp_out=config_cpp \
    --grpc_out=config_cpp \
    --plugin=protoc-gen-grpc=$(which grpc_cpp_plugin) \
    proto/file_processor.proto

  echo "[clients] Compilando cliente C++..."
  g++ -std=c++17 client_cpp/cliente.cpp \
    config_cpp/file_processor.pb.cc config_cpp/file_processor.grpc.pb.cc \
    -Iconfig_cpp $(pkg-config --cflags --libs grpc++) $(pkg-config --cflags --libs protobuf) \
    -o client_cpp/cliente
}

prepare_samples_if_needed() {
  # Gera arquivos mínimos de amostra caso o storage esteja vazio
  local png="client_python/storage/sample.png"
  local pdf="client_python/storage/sample.pdf"
  mkdir -p client_python/storage client_cpp/storage

  if [[ ! -f "$png" ]]; then
    echo "[clients] Criando PNG de amostra..."
    # Quadrado 32x32 vermelho
    convert -size 32x32 xc:red "$png" || true
    if [[ ! -f "$png" ]]; then
      # Fallback: gerar PNG via base64 (1x1 transparente)
      base64 -d > "$png" << 'EOF' || true
iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR4nGNgYAAAAAMAAWgmWQ0AAAAASUVORK5CYII=
EOF
    fi
  fi

  if [[ ! -f "$pdf" ]]; then
    echo "[clients] Criando PDF de amostra..."
    # Tenta usar convert/gs, senão usa um PDF mínimo base64
    if command -v convert >/dev/null 2>&1; then
      convert -size 100x100 xc:white -gravity center -pointsize 18 \
        -annotate 0 "Sample" "$pdf" || true
    fi
    if [[ ! -f "$pdf" ]]; then
      base64 -d > "$pdf" << 'EOF' || true
JVBERi0xLjQKJeLjz9MKMSAwIG9iago8PCAvVHlwZSAvQ2F0YWxvZyAvUGFnZXMgMiAwIFIgPj4KZW5kb2JqCjIgMCBvYmoKPDwgL1R5cGUgL1BhZ2VzIC9LaWRzIFszIDAgUiBdIC9Db3VudCAxID4+CmVuZG9iagozIDAgb2JqCjw8IC9UeXBlIC9QYWdlIC9QYXJlbnQgMiAwIFIgL1Jlc291cmNlcyA8PCA+PiAvTWVkaWFCb3hbMCAwIDYxMiA3OTJdIC9Db250ZW50cyA0IDAgUiA+PgplbmRvYmoKNCAwIG9iago8PCAvTGVuZ3RoIDYxID4+CnN0cmVhbQpCBTAgVGYKMTAwIDcwMCBUZAooU2FtcGxlIFBERikgVGoKRVQKZW5kc3RyZWFtCmVuZG9iagp4cmVmCjAgNQowMDAwMDAwMDAwIDY1NTM1IGYgCjAwMDAwMDAxMTAgMDAwMDAgbiAKMDAwMDAwMDA2MCAwMDAwMCBuIAowMDAwMDAwMTYzIDAwMDAwIG4gCjAwMDAwMDAyNTEgMDAwMDAgbiAKdHJhaWxlcgo8PCAvU2l6ZSA1IC9Sb290IDEgMCBSIC9JRCBbPEU4QjYyMzQ1RjY0RkQzQ0Y+PEU4QjYyMzQ1RjY0RkQzQ0Y+XSA+PgpzdGFydHhyZWYKMzE0CiUlRU9GCiA=
EOF
    fi
  fi

  # Copia para storage do C++ se não existir
  [[ -f client_cpp/storage/sample.png ]] || cp -f "$png" client_cpp/storage/sample.png
  [[ -f client_cpp/storage/sample.pdf ]] || cp -f "$pdf" client_cpp/storage/sample.pdf
}

run_python_client() {
  echo "[clients] Executando cliente Python..."
  PYTHONPATH="${ROOT_DIR}" python3 client_python/cliente.py
}

run_cpp_client() {
  [[ -x client_cpp/cliente ]] || build_cpp_client
  echo "[clients] Executando cliente C++..."
  client_cpp/cliente
}

menu() {
  echo "Selecione o cliente para executar:"
  echo "  1) Python"
  echo "  2) C++"
  echo "  0) Sair"
  read -rp "> " opt || true
  case "${opt:-}" in
    1) run_python_client ;;
    2) run_cpp_client ;;
    0) exit 0 ;;
    *) echo "Opção inválida" ; exit 1 ;;
  esac
}

ensure_env
prepare_samples_if_needed
menu
