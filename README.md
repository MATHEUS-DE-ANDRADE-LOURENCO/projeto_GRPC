# Projeto: Serviço de Processamento de Arquivos com gRPC

Este projeto implementa um servidor gRPC (C++) e dois clientes (Python e C++) para processar arquivos via streaming com os serviços:
- CompressPDF
- ConvertToTXT
- ConvertImageFormat
- ResizeImage

Os diretórios `storage/` em cada entidade são usados para entrada/saída dos arquivos.

## Requisitos
- protoc e `grpc_cpp_plugin`
- gRPC C++ e Protobuf (com `pkg-config`)
- Python 3.10+
- Dependências Python: `grpcio`, `protobuf`
- Ferramentas externas (opcional para processamento real):
  - Ghostscript (`gs`) para compressão de PDF
  - Poppler (`pdftotext`) para PDF -> TXT
  - ImageMagick (`convert`) para conversões/redimensionamento de imagens

Se as ferramentas externas não estiverem disponíveis, o servidor executa um fallback (cópia do arquivo/homônimo) apenas para fins de teste.

## Build C++

Use o script:

```
bash scripts/build_cpp.sh
```

Isso vai:
- Gerar os arquivos C++ do Protobuf em `config_cpp/`
- Compilar `server_cpp/servidor` e `client_cpp/cliente`

## Executar servidor

```
bash scripts/run_server.sh
```

## Cliente Python (interativo)

Coloque os arquivos de teste em `client_python/storage/` e execute:

```
PYTHONPATH=. python3 client_python/cliente.py
```

## Cliente C++ (interativo)

Coloque os arquivos de teste em `client_cpp/storage/` e execute:

```
./client_cpp/cliente
```

## Testes automatizados

```
bash scripts/run_tests.sh
```

O script:
- Prepara amostras (PNG e PDF mínimos) nas pastas `storage/`
- Instala dependências Python (se necessário)
- Build do C++
- Sobe o servidor e executa o cliente Python em modo batch (`client_python/batch.py`)
- Lista os artefatos gerados em `client_python/storage/`

## Observações
- Os clientes listam a pasta `storage/` e oferecem menu com os 4 serviços.
- O servidor grava os arquivos recebidos em `server_cpp/storage/` e retorna o resultado como stream de chunks.
- Ajuste as ferramentas externas no ambiente para resultados reais (PDF comprimido, TXT extraído, imagens convertidas/redimensionadas).
