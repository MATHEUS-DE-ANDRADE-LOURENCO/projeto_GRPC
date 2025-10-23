import grpc
import sys
import os

# Import gerados pelo protoc (assumidos em config_python)
from config_python import file_processor_pb2 as pb2
from config_python import file_processor_pb2_grpc as pb2_grpc


def run():
    with grpc.insecure_channel('localhost:50051') as channel:
        stub = pb2.FileProcessorServiceStub(channel)

        # Colocar requisições abaixo