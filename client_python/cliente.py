import grpc
import sys
import os

# Import gerados pelo protoc (assumidos em config_python)
from config_python import file_processor_pb2 as pb2
from config_python import file_processor_pb2_grpc as pb2_grpc


def generate_file_requests(file_path, file_name):
    # Envia o arquivo em um único chunk para simplificar
    with open(file_path, 'rb') as f:
        content = f.read()
    chunk = pb2.FileChunk(content=content)
    request = pb2.FileRequest(file_name=file_name, file_content=chunk)
    # Para exemplo, preencher compress_pdf_params vazio
    request.compress_pdf_params.CopyFrom(pb2.CompressPDFRequest())
    yield request


def compress_pdf(host, port, input_path, output_path):
    address = f"{host}:{port}"
    with grpc.insecure_channel(address) as channel:
        stub = pb2_grpc.FileProcessorServiceStub(channel)
        file_name = os.path.basename(input_path)
        requests = generate_file_requests(input_path, file_name)
        responses = stub.CompressPDF(requests)
        # Recebe stream de FileResponse e grava conteúdo
        try:
            with open(output_path, 'wb') as out:
                for resp in responses:
                    if resp.file_content and resp.file_content.content:
                        out.write(resp.file_content.content)
                    # Exibir mensagens de status
                    print(f"[server] success={resp.success} message={resp.status_message}")
        except grpc.RpcError as e:
            print(f"gRPC error: {e.code()} - {e.details()}")


if __name__ == '__main__':
    if len(sys.argv) < 3:
        print("Usage: python cliente.py <input_file> <output_file> [host] [port]")
        sys.exit(1)
    input_file = sys.argv[1]
    output_file = sys.argv[2]
    host = sys.argv[3] if len(sys.argv) > 3 else 'localhost'
    port = sys.argv[4] if len(sys.argv) > 4 else '50051'
    compress_pdf(host, port, input_file, output_file)
