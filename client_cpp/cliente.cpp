/*
 * Cliente gRPC em C++ para processamento de arquivos.
 * Padrão de comentários: estilo ANSI-C.
 *
 * Funcionalidades:
 *  - Lista arquivos na pasta client_cpp/storage.
 *  - Envia arquivo ao servidor via streaming para os 4 serviços.
 *  - Recebe arquivos de saída e grava em client_cpp/storage.
 */

#include <iostream>
#include <memory>
#include <string>
#include <fstream>
#include <vector>
#include <filesystem>

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

namespace fs = std::filesystem;

// Obtém o caminho do diretório de armazenamento do cliente
static std::string StorageDir() {
    return (fs::path(__FILE__).parent_path() / "storage").string();
}

// Lista os arquivos na pasta de storage
static std::vector<std::string> ListStorageFiles() {
    std::vector<std::string> files;
    fs::path dir(StorageDir());

    if (!fs::exists(dir)) fs::create_directories(dir); // Cria o diretório se não existir

    for (auto &p : fs::directory_iterator(dir)) { // Percorre os arquivos no diretório
        // Adiciona o nome do arquivo à lista
        if (p.is_regular_file()) files.push_back(p.path().filename().string()); 
    }

    return files;
}

// Retorna o diretório completo de um dos arquivos selecionados pelo usuário
static std::string ChooseFile() {
    auto files = ListStorageFiles(); // Lista arquivos

    if (files.empty()) {
        std::cout << "Pasta de storage vazia: " << StorageDir() << std::endl;
        return {};
    }

    //Imprime os arquivos disponíveis na pasta
    std::cout << "Arquivos disponíveis na pasta storage:" << std::endl;
    for (size_t i = 0; i < files.size(); ++i) std::cout << "  " << (i+1) << ") " << files[i] << std::endl;

    // Permite a seleção de um dos arquivos
    while (true) {
        std::cout << "Selecione o número do arquivo: ";
        int idx;

        if (!(std::cin >> idx)) { 
            std::cin.clear(); std::cin.ignore(1024, '\n'); 
            continue; 
        }

        // Se número selecionado for válido, retorna o caminho completo
        if (idx >= 1 && (size_t)idx <= files.size()) return (fs::path(StorageDir()) / files[idx-1]).string();

        // Se não, continua o loop
        std::cout << "Seleção inválida. Tente novamente." << std::endl;
    }
}

// Cliente gRPC para FileProcessorService
class FileProcessorClient {
public:
    // Cria stub gRPC para comunicação com o servidor
    FileProcessorClient(std::shared_ptr<Channel> channel): 
        stub_(FileProcessorService::NewStub(channel)) {}

    // Serviço de compressão de PDF
    bool CompressPDF(const std::string& input_path, const std::string& output_path) {
        // Cria contexto gRPC
        ClientContext context;

        // Inicia chamada GRPC bidirecional para o método CompressPDF
        std::unique_ptr<ClientReaderWriter<FileRequest, FileResponse>> stream(stub_->CompressPDF(&context));

        // Cria fluxo de leitura do arquivo de entrada
        std::ifstream in(input_path, std::ios::binary);

        // Verifica se o arquivo foi aberto corretamente
        if (!in) {
            std::cerr << "Falha ao abrir arquivo de entrada: " << input_path << std::endl;
            return false;
        }

        // Enviar parâmetros no primeiro chunk, depois dados em pedaços
        {
            FileRequest req;
            req.set_file_name(fs::path(input_path).filename().string());
            req.mutable_compress_pdf_params();
            stream->Write(req);
        }

        // Envia o arquivo em pedaços
        {
            const size_t CHUNK = 1024 * 1024;
            std::vector<char> buf(CHUNK);
            while (in) {
                // Lê parte pré determinada pelo tamanho do chunk
                in.read(buf.data(), buf.size());
                std::streamsize n = in.gcount(); // Tamanho do stream

                if (n <= 0) break;

                FileRequest req; // Cria nova requisição para envio
                req.set_file_name(fs::path(input_path).filename().string());
                req.mutable_file_content()->set_content(buf.data(), static_cast<size_t>(n));

                // Envia o chunk para o servidor
                if (!stream->Write(req)) break;
            }
        }
        stream->WritesDone();

        // Recebe as respostas do servidor e grava no arquivo de saída
        FileResponse resp;
        std::ofstream out(output_path, std::ios::binary);
        while (stream->Read(&resp)) { // Enquanto houver conteúdo para leitura no arquivo recebido
            if (resp.has_file_content()) { // Se tiver conteúdo, escrever no arquivo de saída
                out.write(resp.file_content().content().data(), resp.file_content().content().size());
            }
            // Exibe status da operação
            std::cout << "[server] success=" << resp.success() << " message=" << resp.status_message() << std::endl;
        }

        // Finaliza a chamada gRPC e verifica o status
        Status status = stream->Finish();
        if (!status.ok()) {
            std::cerr << "gRPC failed: " << status.error_message() << std::endl;
            return false;
        }
        return true;
    }

    bool ConvertToTXT(const std::string& input_path, const std::string& output_path) {
        // Cria contexto gRPC
        ClientContext context;
        // Inicia chamada GRPC bidirecional para o método ConvertToTXT
        auto stream = stub_->ConvertToTXT(&context);
        {
            // Enviar parâmetros no primeiro chunk, depois dados em pedaços
            FileRequest req; req.set_file_name(fs::path(input_path).filename().string());
            req.mutable_convert_to_txt_params();
            stream->Write(req);
        }

        // Cria fluxo de leitura do arquivo de entrada
        std::ifstream in(input_path, std::ios::binary);
        const size_t CHUNK = 1024*1024; std::vector<char> buf(CHUNK);

        // Envia o arquivo em pedaços
        while (in) { 
            in.read(buf.data(), buf.size()); 
            auto n=in.gcount();

            if (n<=0) break;

            FileRequest req; 
            req.set_file_name(fs::path(input_path).filename().string()); 
            req.mutable_file_content()->set_content(buf.data(), (size_t)n);

            if(!stream->Write(req)) break; 
        }

        stream->WritesDone();

        // Recebe as respostas do servidor e grava no arquivo de saída
        std::ofstream out(output_path, std::ios::binary); FileResponse resp;
        while (stream->Read(&resp)) 
            if (resp.has_file_content()) 
                out.write(resp.file_content().content().data(), resp.file_content().content().size());
        auto status = stream->Finish(); if(!status.ok()){ std::cerr<<"gRPC failed: "<<status.error_message()<<std::endl; return false; }
        return true;
    }

    bool ConvertImageFormat(const std::string& input_path, const std::string& output_path, const std::string& format) {
        ClientContext context; 
        auto stream = stub_->ConvertImageFormat(&context);
        { 
            FileRequest req; 
            req.set_file_name(fs::path(input_path).filename().string()); 
            req.mutable_convert_image_format_params()->set_output_format(format); 
            stream->Write(req);
        }

            std::ifstream in(input_path, std::ios::binary); 
            const size_t CHUNK=1024*1024; std::vector<char> buf(CHUNK);
            
            while(in){ 
                in.read(buf.data(), buf.size()); auto n=in.gcount();

                if(n<=0) break;

                FileRequest req; 
                req.set_file_name(fs::path(input_path).filename().string()); 
                req.mutable_file_content()->set_content(buf.data(), (size_t)n); 
                if(!stream->Write(req)) break; 
            }

            stream->WritesDone(); std::ofstream out(output_path, std::ios::binary); 
            FileResponse resp;

            while(stream->Read(&resp)) 
                if(resp.has_file_content()) 
                    out.write(resp.file_content().content().data(), resp.file_content().content().size());
            
            auto status=stream->Finish(); 
            
            if(!status.ok()){ 
                std::cerr<<"gRPC failed: "<<status.error_message()<<std::endl; 
                return false;
            } 
            
            return true;
    }

    bool ResizeImage(const std::string& input_path, const std::string& output_path, int width, int height) {
        // Cria contexto gRPC
        ClientContext context; 
        auto stream = stub_->ResizeImage(&context);

        // Envia parâmetros da requisição
        { 
            FileRequest req; 
            req.set_file_name(fs::path(input_path).filename().string()); 
            auto* p=req.mutable_resize_image_params(); 
            p->set_width(width); p->set_height(height); 
            stream->Write(req);
        }

            // Cria fluxo de leitura do arquivo de entrada
            std::ifstream in(input_path, std::ios::binary); 
            const size_t CHUNK=1024*1024; 
            std::vector<char> buf(CHUNK);

            // Envia o arquivo em pedaços
            while(in){ 
                in.read(buf.data(), buf.size()); 
                auto n=in.gcount(); 

                if(n<=0) break; 

                FileRequest req; 
                req.set_file_name(fs::path(input_path).filename().string()); 
                req.mutable_file_content()->set_content(buf.data(), (size_t)n); 

                if(!stream->Write(req)) break; 
            }

            // Indica que todos os dados foram enviados
            stream->WritesDone(); 

            // Cria fluxo de escrita do arquivo de saída
            // Passa o que foi recebido do servidor ao diretório de saída
            std::ofstream out(output_path, std::ios::binary); 
            FileResponse resp;

            // Recebe as respostas do servidor e grava no arquivo de saída
            while(stream->Read(&resp)) 
                if(resp.has_file_content()) 
                    out.write(resp.file_content().content().data(), resp.file_content().content().size());

            auto status=stream->Finish(); 

            if(!status.ok()){ 
                std::cerr<<"gRPC failed: "<<status.error_message()<<std::endl; 
                return false;
            } 

            return true;
    }

private:
    // Stub gRPC para comunicação com o servidor
    std::unique_ptr<FileProcessorService::Stub> stub_;
};

int main() {
    std::string server = "localhost:50051";

    // Cria canal de comunicação com o servidor
    FileProcessorClient client(grpc::CreateChannel(server, grpc::InsecureChannelCredentials()));

    // Criação de Menu para seleção dos serviços
    while (true) {
        std::cout << "\n=== Cliente C++ ===\n1) CompressPDF\n2) ConvertToTXT\n3) ConvertImageFormat\n4) ResizeImage\n0) Sair\nEscolha: ";

        int opt; 
        
        if(!(std::cin>>opt)){ 
            std::cin.clear(); 
            std::cin.ignore(1024,'\n'); 
            continue; 
        }

        if (opt==0) break;

        std::cin.ignore(1024,'\n');

        std::string input_path = ChooseFile();

        if (input_path.empty()) continue;
        fs::path in(input_path); std::string base = in.stem().string();

        if (opt==1) {
            std::string out = (fs::path(StorageDir()) / (base+"_compressed.pdf")).string();
            client.CompressPDF(input_path, out);
            std::cout << "Saída: " << out << std::endl;
        } else if (opt==2) {
            std::string out = (fs::path(StorageDir()) / (base+".txt")).string();
            client.ConvertToTXT(input_path, out);
            std::cout << "Saída: " << out << std::endl;
        } else if (opt==3) {
            std::string format; std::cout << "Formato de saída (png/jpg/webp): "; std::getline(std::cin, format);
            std::string out = (fs::path(StorageDir()) / (base+"."+format)).string();
            client.ConvertImageFormat(input_path, out, format);
            std::cout << "Saída: " << out << std::endl;
        } else if (opt==4) {
            int w,h; std::cout << "Largura: "; std::cin>>w; std::cout << " Altura: "; std::cin>>h; std::cin.ignore(1024,'\n');
            std::string out = (fs::path(StorageDir()) / (base+"_"+std::to_string(w)+"x"+std::to_string(h)+".img")).string();
            client.ResizeImage(input_path, out, w, h);
            std::cout << "Saída: " << out << std::endl;
        }
    }
    return 0;
}
