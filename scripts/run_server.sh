#!/usr/bin/env bash
# Execução do servidor (C++) com preparação de ambiente e build automáticos.
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT_DIR"

build_cpp() {
  echo "[server] Gerando código gRPC C++ (protoc)..."
  protoc -I=proto \
    --cpp_out=config_cpp \
    --grpc_out=config_cpp \
    --plugin=protoc-gen-grpc=$(which grpc_cpp_plugin) \
    proto/file_processor.proto

  echo "[server] Compilando servidor..."
  g++ -std=c++17 server_cpp/servidor.cpp \
    config_cpp/file_processor.pb.cc config_cpp/file_processor.grpc.pb.cc \
    -Iconfig_cpp $(pkg-config --cflags --libs grpc++) $(pkg-config --cflags --libs protobuf) \
    -o server_cpp/servidor
}

echo "[server] Configurando ambiente..."
bash scripts/env_setup.sh

if [[ ! -x server_cpp/servidor ]]; then
  build_cpp
fi

ADDR="${1:-0.0.0.0:50051}"
echo "[server] Iniciando servidor em ${ADDR}"
exec server_cpp/servidor "${ADDR}"
