import os
import grpc
from config_python import file_processor_pb2 as pb2
from config_python import file_processor_pb2_grpc as pb2_grpc

# Arquivo batch.py: processa arquivos em lote sem interação do usuário.

STORAGE_DIR = os.path.join(os.path.dirname(__file__), 'storage')


def stream_file_requests(path: str, params_filler):
    file_name = os.path.basename(path)
    first = True
    with open(path, 'rb') as f:
        while True:
            data = f.read(1024 * 1024)
            if not data:
                break
            req = pb2.FileRequest(file_name=file_name, file_content=pb2.FileChunk(content=data))
            if first:
                params_filler(req)
                first = False
            yield req


def save_responses(responses, out_path: str):
    os.makedirs(os.path.dirname(out_path), exist_ok=True)
    with open(out_path, 'wb') as f:
        for r in responses:
            if r.file_content and r.file_content.content:
                f.write(r.file_content.content)


def run_batch():
    host = os.environ.get('GRPC_HOST', 'localhost')
    port = os.environ.get('GRPC_PORT', '50051')
    address = f"{host}:{port}"
    with grpc.insecure_channel(address) as channel:
        stub = pb2_grpc.FileProcessorServiceStub(channel)
        # CompressPDF
        in_pdf = os.path.join(STORAGE_DIR, 'sample.pdf')
        if os.path.exists(in_pdf):
            resp = stub.CompressPDF(stream_file_requests(in_pdf, lambda r: r.compress_pdf_params.CopyFrom(pb2.CompressPDFRequest())))
            save_responses(resp, os.path.join(STORAGE_DIR, 'sample_compressed.pdf'))
        # ConvertToTXT
        if os.path.exists(in_pdf):
            resp = stub.ConvertToTXT(stream_file_requests(in_pdf, lambda r: r.convert_to_txt_params.CopyFrom(pb2.ConvertToTXTRequest())))
            save_responses(resp, os.path.join(STORAGE_DIR, 'sample.txt'))
        # ConvertImageFormat
        in_png = os.path.join(STORAGE_DIR, 'pixel.png')
        if os.path.exists(in_png):
            resp = stub.ConvertImageFormat(stream_file_requests(in_png, lambda r: r.convert_image_format_params.CopyFrom(pb2.ConvertImageFormatRequest(output_format='jpg'))))
            save_responses(resp, os.path.join(STORAGE_DIR, 'pixel.jpg'))
        # ResizeImage
        if os.path.exists(in_png):
            resp = stub.ResizeImage(stream_file_requests(in_png, lambda r: r.resize_image_params.CopyFrom(pb2.ResizeImageRequest(width=64, height=64))))
            save_responses(resp, os.path.join(STORAGE_DIR, 'pixel_64x64.img'))


if __name__ == '__main__':
    run_batch()
