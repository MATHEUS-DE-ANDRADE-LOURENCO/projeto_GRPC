// Compilação (funcionou neste ambiente):
// protoc -I=proto --cpp_out=config_cpp --grpc_out=config_cpp --plugin=protoc-gen-grpc=$(which grpc_cpp_plugin) proto/file_processor.proto
// g++ -std=c++17 server_cpp/servidor.cpp config_cpp/file_processor.pb.cc config_cpp/file_processor.grpc.pb.cc \
//   -Iconfig_cpp $(pkg-config --cflags --libs grpc++) $(pkg-config --cflags --libs protobuf) -o server_cpp/servidor

#include <iostream>
#include <memory>
#include <string>
#include <fstream>

#include <grpcpp/grpcpp.h>

#include "../config_cpp/file_processor.grpc.pb.h"
#include "../config_cpp/file_processor.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using grpc::ServerReaderWriter;

using file_processor::FileProcessorService;
using file_processor::FileRequest;
using file_processor::FileResponse;
using file_processor::FileChunk;

class FileProcessorServiceImpl final : public FileProcessorService::Service {
public:
    Status CompressPDF(ServerContext* context,
                       ServerReaderWriter<FileResponse, FileRequest>* stream) override {
        FileRequest req;
        // Simplesmente ecoa o conteúdo recebido como resposta
        while (stream->Read(&req)) {
            FileResponse resp;
            resp.set_file_name(req.file_name());
            if (req.has_file_content()) {
                FileChunk* out_chunk = resp.mutable_file_content();
                out_chunk->set_content(req.file_content().content());
            }
            resp.set_success(true);
            resp.set_status_message("Processed (echo)");
            stream->Write(resp);
        }
        return Status::OK;
    }

    // Métodos não implementados retornam UNIMPLEMENTED por padrão (gerado)
};

void RunServer(const std::string& server_address) {
    FileProcessorServiceImpl service;

    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Servidor gRPC ouvindo em " << server_address << std::endl;
    server->Wait();
}

int main(int argc, char** argv) {
    std::string address = "0.0.0.0:50051";
    if (argc >= 2) address = argv[1];
    RunServer(address);
    return 0;
}
