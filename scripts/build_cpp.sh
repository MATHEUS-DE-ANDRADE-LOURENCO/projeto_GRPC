#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."

echo "[build] Gerando arquivos C++ do protobuf..."
protoc -I=proto \
  --cpp_out=config_cpp \
  --grpc_out=config_cpp \
  --plugin=protoc-gen-grpc=$(which grpc_cpp_plugin) \
  proto/file_processor.proto

echo "[build] Compilando servidor..."
g++ -std=c++17 server_cpp/servidor.cpp \
  config_cpp/file_processor.pb.cc config_cpp/file_processor.grpc.pb.cc \
  -Iconfig_cpp $(pkg-config --cflags --libs grpc++) $(pkg-config --cflags --libs protobuf) \
  -o server_cpp/servidor

echo "[build] Compilando cliente C++..."
g++ -std=c++17 client_cpp/cliente.cpp \
  config_cpp/file_processor.pb.cc config_cpp/file_processor.grpc.pb.cc \
  -Iconfig_cpp $(pkg-config --cflags --libs grpc++) $(pkg-config --cflags --libs protobuf) \
  -o client_cpp/cliente

echo "[build] Concluído."
