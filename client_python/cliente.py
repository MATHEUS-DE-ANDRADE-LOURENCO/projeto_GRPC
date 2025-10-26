import grpc
import grpc
import sys
import os
from typing import Iterator

# Import gerados pelo protoc (assumidos em config_python)
from config_python import file_processor_pb2 as pb2
from config_python import file_processor_pb2_grpc as pb2_grpc

STORAGE_DIR = os.path.join(os.path.dirname(__file__), 'storage')


# Lista arquivos na pasta de storage
def list_storage_files() -> list[str]:
    if not os.path.isdir(STORAGE_DIR):
        os.makedirs(STORAGE_DIR, exist_ok=True)
    return [f for f in os.listdir(STORAGE_DIR) if os.path.isfile(os.path.join(STORAGE_DIR, f))]

# Permite ao usuário escolher um arquivo da pasta de storage
def choose_file() -> str:
    files = list_storage_files() # lista arquivos na pasta storage
    if not files:
        print(f"Pasta de storage vazia: {STORAGE_DIR}")
        return ''
    
    print("Arquivos disponíveis na pasta storage:")
    for i, name in enumerate(files, 1): # Itera sobre os arquivos disponíveis
        print(f"  {i}) {name}")

    while True:
        sel = input("Selecione o número do arquivo: ")
        try:
            idx = int(sel)
            if 1 <= idx <= len(files):
                return os.path.join(STORAGE_DIR, files[idx - 1])
        except ValueError:
            pass
        print("Seleção inválida. Tente novamente.")

# Gera stream de FileRequest a partir do arquivo e preenche os parâmetros
def stream_file_requests(path: str, params_filler) -> Iterator[pb2.FileRequest]:
    file_name = os.path.basename(path)
    first = True

    # Abre o arquivo e lê em chunks pouco a pouco para envio
    with open(path, 'rb') as f:
        while True:
            data = f.read(1024 * 1024)

            if not data:
                break

            # Cria chunk para requisição de envio
            chunk = pb2.FileChunk(content=data)
            req = pb2.FileRequest(file_name=file_name, file_content=chunk) # envia o chunk

            # Preenche os parâmetros específicos da operação na primeira requisição
            if first:
                params_filler(req)
                first = False
            yield req # Envia a requisição

# Grava as respostas do servidor em um arquivo de saída
def write_responses_to_file(responses, output_path: str):
    os.makedirs(os.path.dirname(output_path), exist_ok=True)
    with open(output_path, 'wb') as out:
        # Itera as respostas do servidor
        for resp in responses:
            if resp.file_content and resp.file_content.content:
                out.write(resp.file_content.content) # grava o conteúdo recebido
            if resp.status_message:
                print(f"[server] {resp.status_message} (success={resp.success})") # mostra mensagens de status

# Serviço de Compressão de PDF
def do_compress_pdf(stub, input_path: str):
    def fill_params(req: pb2.FileRequest): # Preenche parâmetros específicos
        req.compress_pdf_params.CopyFrom(pb2.CompressPDFRequest())

    # Chama o serviço do servidor
    responses = stub.CompressPDF(stream_file_requests(input_path, fill_params))

    # Define o caminho de saída
    base = os.path.splitext(os.path.basename(input_path))[0]
    output_path = os.path.join(STORAGE_DIR, f"{base}_compressed.pdf")
    
    # Escreve a resposta da requisição em arquivo de saída
    write_responses_to_file(responses, output_path)
    print(f"Saída salva em: {output_path}")


def do_convert_to_txt(stub, input_path: str):
    def fill_params(req: pb2.FileRequest): # Preenche parâmetros específicos
        req.convert_to_txt_params.CopyFrom(pb2.ConvertToTXTRequest())

    # Chama o serviço do servidor
    responses = stub.ConvertToTXT(stream_file_requests(input_path, fill_params))

    # Define o caminho de saída
    base = os.path.splitext(os.path.basename(input_path))[0]
    output_path = os.path.join(STORAGE_DIR, f"{base}.txt")

    # Escreve a resposta da requisição em arquivo de saída
    write_responses_to_file(responses, output_path)
    print(f"Saída salva em: {output_path}")


def do_convert_image_format(stub, input_path: str):
    # Solicita o formato de saída ao usuário
    out_format = input("Formato de saída (ex: png, jpg, webp): ").strip()

    def fill_params(req: pb2.FileRequest): # Preenche parâmetros específicos
        req.convert_image_format_params.CopyFrom(pb2.ConvertImageFormatRequest(output_format=out_format))

    # Chama o serviço do servidor
    responses = stub.ConvertImageFormat(stream_file_requests(input_path, fill_params))

    # Define o caminho de saída
    base = os.path.splitext(os.path.basename(input_path))[0]
    output_path = os.path.join(STORAGE_DIR, f"{base}.{out_format}")

    # Escreve a resposta da requisição em arquivo de saída
    write_responses_to_file(responses, output_path)
    print(f"Saída salva em: {output_path}")


def do_resize_image(stub, input_path: str):
    while True:
        try: # Solicita dimensões ao usuário
            width = int(input("Largura (px): ").strip())
            height = int(input("Altura (px): ").strip())
            break
        except ValueError:
            print("Valores inválidos, tente novamente.")

    # Preenche parâmetros específicos
    def fill_params(req: pb2.FileRequest):
        req.resize_image_params.CopyFrom(pb2.ResizeImageRequest(width=width, height=height))

    # Chama o serviço do servidor
    responses = stub.ResizeImage(stream_file_requests(input_path, fill_params))

    # Define o caminho de saída
    base = os.path.splitext(os.path.basename(input_path))[0]
    output_path = os.path.join(STORAGE_DIR, f"{base}_{width}x{height}.img")

    # Escreve a resposta da requisição em arquivo de saída
    write_responses_to_file(responses, output_path)
    print(f"Saída salva em: {output_path}")


def main():
    host = os.environ.get('GRPC_HOST', 'localhost')
    port = os.environ.get('GRPC_PORT', '50051')
    address = f"{host}:{port}"

    # Conecta ao servidor gRPC
    with grpc.insecure_channel(address) as channel:
        stub = pb2_grpc.FileProcessorServiceStub(channel)

        # Apresenta menu para seleção de serviços
        while True:
            print("\n=== Cliente Python ===")
            print("1) CompressPDF")
            print("2) ConvertToTXT")
            print("3) ConvertImageFormat")
            print("4) ResizeImage")
            print("0) Sair")

            opt = input("Escolha: ").strip()

            if opt == '0':
                break
            if opt not in {'1','2','3','4'}:
                print("Opção inválida")
                continue
            path = choose_file()
            if not path:
                continue

            # Chama o serviço de acordo com a opção escolhida
            try:
                if opt == '1':
                    do_compress_pdf(stub, path)
                elif opt == '2':
                    do_convert_to_txt(stub, path)
                elif opt == '3':
                    do_convert_image_format(stub, path)
                elif opt == '4':
                    do_resize_image(stub, path)
            # Erro na seleção do serviço
            except grpc.RpcError as e:
                print(f"Erro gRPC: {e.code()} - {e.details()}")


if __name__ == '__main__':
    main()