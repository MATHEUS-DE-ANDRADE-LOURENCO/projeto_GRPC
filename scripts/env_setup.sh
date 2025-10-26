#!/usr/bin/env bash
# Configuração do ambiente (Linux/Ubuntu): C++, Python e ferramentas usadas pelo servidor.
set -euo pipefail

echo "[setup] Atualizando índices e instalando dependências do sistema..."
export DEBIAN_FRONTEND=noninteractive
sudo apt-get update -y
sudo apt-get install -y \
  build-essential cmake pkg-config \
  protobuf-compiler libprotobuf-dev libprotoc-dev \
  protobuf-compiler-grpc libgrpc++-dev libabsl-dev libc-ares-dev \
  libssl-dev zlib1g-dev libre2-dev \
  ghostscript poppler-utils imagemagick \
  ca-certificates curl python3-pip

echo "[setup] Verificando protoc e grpc_cpp_plugin..."
protoc --version || { echo "protoc não encontrado"; exit 1; }
which grpc_cpp_plugin >/dev/null 2>&1 || { echo "grpc_cpp_plugin não encontrado"; exit 1; }

echo "[setup] Instalando dependências Python..."
python3 -m pip install --upgrade pip
python3 -m pip install --upgrade grpcio>=1.75.1 protobuf>=4.25.1

echo "[setup] Preparando diretórios de storage..."
mkdir -p client_python/storage client_cpp/storage server_cpp/storage config_cpp config_python

echo "[setup] Pronto. Ambiente configurado."
