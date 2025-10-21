# // Compile (worked here):
# // protoc -I=proto --cpp_out=config_cpp --grpc_out=config_cpp --plugin=protoc-gen-grpc=$(which grpc_cpp_plugin) proto/file_processor.proto
# // g++ -std=c++17 client_cpp/cliente.cpp config_cpp/file_processor.pb.cc config_cpp/file_processor.grpc.pb.cc \
# //   -Iconfig_cpp $(pkg-config --cflags --libs grpc++) $(pkg-config --cflags --libs protobuf) -o client_cpp/cliente
#include <iostream>
#include <memory>
#include <string>
#include <fstream>

#include <grpcpp/grpcpp.h>

#include "../config_cpp/file_processor.grpc.pb.h"
#include "../config_cpp/file_processor.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using grpc::ClientReaderWriter;

using file_processor::FileProcessorService;
using file_processor::FileRequest;
using file_processor::FileResponse;
using file_processor::FileChunk;
using file_processor::CompressPDFRequest;

class FileProcessorClient {
public:
    FileProcessorClient(std::shared_ptr<Channel> channel)
        : stub_(FileProcessorService::NewStub(channel)) {}

    bool CompressPDF(const std::string& input_path, const std::string& output_path) {
        ClientContext context;
        std::unique_ptr<ClientReaderWriter<FileRequest, FileResponse>> stream(
            stub_->CompressPDF(&context));

        // Ler arquivo e enviar como um único chunk
        std::ifstream in(input_path, std::ios::binary);
        if (!in) {
            std::cerr << "Falha ao abrir arquivo de entrada: " << input_path << std::endl;
            return false;
        }
        std::string data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

        FileRequest req;
        req.set_file_name(input_path);
        FileChunk* chunk = req.mutable_file_content();
        chunk->set_content(data);
        CompressPDFRequest* params = req.mutable_compress_pdf_params();

        if (!stream->Write(req)) {
            std::cerr << "Falha ao enviar requisição" << std::endl;
            stream->WritesDone();
            return false;
        }

        stream->WritesDone();

        // Receber respostas
        FileResponse resp;
        std::ofstream out(output_path, std::ios::binary);
        while (stream->Read(&resp)) {
            if (resp.has_file_content()) {
                out.write(resp.file_content().content().data(), resp.file_content().content().size());
            }
            std::cout << "[server] success=" << resp.success() << " message=" << resp.status_message() << std::endl;
        }

        Status status = stream->Finish();
        if (!status.ok()) {
            std::cerr << "gRPC failed: " << status.error_message() << std::endl;
            return false;
        }
        return true;
    }

private:
    std::unique_ptr<FileProcessorService::Stub> stub_;
};

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: cliente <input_file> <output_file> [server_address]" << std::endl;
        return 1;
    }
    std::string input = argv[1];
    std::string output = argv[2];
    std::string server = (argc >= 4) ? argv[3] : "localhost:50051";

    FileProcessorClient client(grpc::CreateChannel(server, grpc::InsecureChannelCredentials()));
    bool ok = client.CompressPDF(input, output);
    if (!ok) return 1;
    std::cout << "Operação finalizada." << std::endl;
    return 0;
}
