/*
 * Servidor gRPC para processamento de arquivos.
 * Padrão de comentários: estilo ANSI-C.
 *
 * Fluxo geral:
 *  - Recebe stream de FileRequest (primeiro com parâmetros, demais com chunks do arquivo).
 *  - Persiste o arquivo recebido em server_cpp/storage/.
 *  - Executa a transformação solicitada (usando ferramentas externas quando disponíveis).
 *  - Envia stream de FileResponse contendo os chunks do arquivo de saída e mensagens de status.
 */

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

#include <vector>
#include <filesystem>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace fs = std::filesystem;

// Lista diretório de armazenamento
static std::string StorageDir() {
    return (fs::path(__FILE__).parent_path() / "storage").string();
}

// Arquivos de log
static std::mutex g_log_mutex;

// Retorna o caminho do arquivo de log
static std::string LogFilePath() {
    return (fs::path(__FILE__).parent_path() / "server.log").string();
}

// Obtém horário atual
static std::string NowTimestamp() {

    using clock = std::chrono::system_clock;
    auto now = clock::now(); // Horário atual

    // Converte para time_t
    std::time_t t = clock::to_time_t(now);
    std::tm tm{};

    // Plataforma específica para conversão thread-safe
    #if defined(_WIN32)
        localtime_s(&tm, &t);
    #else
        localtime_r(&t, &tm);
    #endif
        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
        return oss.str();
}

// Registra operação no arquivo de log
static void LogOperation(const std::string& service_name,
                         const std::string& file_name,
                         bool success,
                         const std::string& message) {

    std::lock_guard<std::mutex> lk(g_log_mutex);
    std::ofstream log(LogFilePath(), std::ios::app);
    if (log.is_open()) {
        log << "[" << NowTimestamp() << "] "
            << (success ? "SUCCESS" : "FAIL")
            << " - Service: " << service_name
            << ", File: " << (file_name.empty() ? "-" : file_name)
            << ", Message: " << message
            << std::endl;
    }
}

// Verifica se comando existe no sistema
static bool CommandExists(const std::string& cmd) {
    std::string c = "command -v '" + cmd + "' >/dev/null 2>&1";
    int r = std::system(c.c_str());
    return r == 0;
}

// Executa comando shell
static int RunShell(const std::string& cmd) {
    return std::system(cmd.c_str());
}

// Escreve vetor de bytes em arquivo
static bool WriteAll(const std::string& path, const std::vector<uint8_t>& data) {
    std::ofstream out(path, std::ios::binary);
    if (!out) return false; // Não conseguiu abrir o arquivo

    // Escreve todos os bytes
    out.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
    return true;
}

// Lê stream de FileRequest para vetor de bytes
static bool ReadStreamToVector(ServerReaderWriter<FileResponse, FileRequest>* stream, std::string& file_name, std::vector<uint8_t>& out_data) {
    FileRequest req;
    bool has_params = false;
    while (stream->Read(&req)) {
        if (file_name.empty() && !req.file_name().empty()) file_name = req.file_name();
        if (req.has_file_content()) {
            const auto& c = req.file_content().content();
            out_data.insert(out_data.end(), c.begin(), c.end());
        }
        // Qualquer um dos quatro params sinaliza a operação escolhida
        if (req.has_compress_pdf_params() || req.has_convert_to_txt_params() || req.has_convert_image_format_params() || req.has_resize_image_params()) {
            has_params = true;
        }
        // Observação: os valores específicos de params serão lidos diretamente nos handlers (ver abaixo)
    }
    return has_params;
}

static void StreamFileBack(ServerReaderWriter<FileResponse, FileRequest>* stream, const std::string& out_file, const std::string& status_prefix, bool success) {
    const size_t CHUNK = 1024 * 1024;
    std::ifstream in(out_file, std::ios::binary);
    if (!in) {
        FileResponse resp; resp.set_success(false); resp.set_status_message("Falha ao abrir saída: " + out_file);
        stream->Write(resp); return;
    }
    std::vector<char> buf(CHUNK);
    while (in) {
        in.read(buf.data(), buf.size()); auto n = in.gcount(); if (n<=0) break;
        FileResponse resp; resp.set_success(success); resp.set_status_message(status_prefix);
        auto* ch = resp.mutable_file_content(); ch->set_content(buf.data(), (size_t)n);
        stream->Write(resp);
    }
}

class FileProcessorServiceImpl final : public FileProcessorService::Service {
public:
    Status CompressPDF(ServerContext* context,
                       ServerReaderWriter<FileResponse, FileRequest>* stream) override {
        std::string fname; std::vector<uint8_t> data; bool got_params=false;
        FileRequest req;
        while (stream->Read(&req)) {
            if (fname.empty() && !req.file_name().empty()) fname = req.file_name();
            if (req.has_compress_pdf_params()) got_params = true;
            if (req.has_file_content()) {
                const auto& c = req.file_content().content(); data.insert(data.end(), c.begin(), c.end());
            }
        }
        if (!got_params) { FileResponse r; r.set_success(false); r.set_status_message("Parâmetros ausentes"); stream->Write(r); LogOperation("CompressPDF", fname, false, "Parâmetros ausentes"); return Status::OK; }

        fs::create_directories(StorageDir());
        fs::path in = fs::path(StorageDir()) / ("in_" + fname);
        fs::path out = fs::path(StorageDir()) / ("out_compressed_" + fs::path(fname).stem().string() + ".pdf");
        if (!WriteAll(in.string(), data)) { FileResponse r; r.set_success(false); r.set_status_message("Falha ao salvar entrada"); stream->Write(r); LogOperation("CompressPDF", fname, false, "Falha ao salvar entrada"); return Status::OK; }

        bool ok = false; std::string msg;
        if (CommandExists("gs")) {
            std::string cmd = "gs -sDEVICE=pdfwrite -dCompatibilityLevel=1.4 -dPDFSETTINGS=/screen -dNOPAUSE -dQUIET -dBATCH -sOutputFile='"+out.string()+"' '"+in.string()+"'";
            ok = (RunShell(cmd) == 0); msg = ok?"PDF comprimido":"Falha na compressão (gs)";
        } else {
            // Fallback: copia como está
            std::ofstream o(out, std::ios::binary); std::ifstream i(in, std::ios::binary); o<<i.rdbuf(); ok = o.good(); msg = ok?"Fallback: arquivo copiado":"Falha no fallback";
        }
        StreamFileBack(stream, out.string(), msg, ok);
        LogOperation("CompressPDF", fname, ok, msg);
        return Status::OK;
    }

    Status ConvertToTXT(ServerContext* context,
                        ServerReaderWriter<FileResponse, FileRequest>* stream) override {
        std::string fname; std::vector<uint8_t> data; bool got_params=false; FileRequest req;
        while (stream->Read(&req)) {
            if (fname.empty() && !req.file_name().empty()) fname = req.file_name();
            if (req.has_convert_to_txt_params()) got_params = true;
            if (req.has_file_content()) { const auto& c = req.file_content().content(); data.insert(data.end(), c.begin(), c.end()); }
        }
        if (!got_params) { FileResponse r; r.set_success(false); r.set_status_message("Parâmetros ausentes"); stream->Write(r); LogOperation("ConvertToTXT", fname, false, "Parâmetros ausentes"); return Status::OK; }
        fs::create_directories(StorageDir()); fs::path in = fs::path(StorageDir()) / ("in_" + fname);
        fs::path out = fs::path(StorageDir()) / (fs::path(fname).stem().string() + ".txt");
        if (!WriteAll(in.string(), data)) { FileResponse r; r.set_success(false); r.set_status_message("Falha ao salvar entrada"); stream->Write(r); LogOperation("ConvertToTXT", fname, false, "Falha ao salvar entrada"); return Status::OK; }
        bool ok=false; std::string msg;
        if (CommandExists("pdftotext")) { std::string cmd = "pdftotext '"+in.string()+"' '"+out.string()+"'"; ok = (RunShell(cmd)==0); msg = ok?"Convertido para TXT":"Falha pdftotext"; }
        else { // Fallback: tenta tratar bytes como texto
            std::ofstream o(out, std::ios::binary); o.write(reinterpret_cast<const char*>(data.data()), (std::streamsize)data.size()); ok=o.good(); msg = ok?"Fallback: bytes gravados em .txt":"Falha fallback";
        }
        StreamFileBack(stream, out.string(), msg, ok); return Status::OK;
        LogOperation("ConvertToTXT", fname, ok, msg);
        return Status::OK;
    }

    Status ConvertImageFormat(ServerContext* context,
                              ServerReaderWriter<FileResponse, FileRequest>* stream) override {
        std::string fname; std::vector<uint8_t> data; bool got_params=false; std::string out_ext="png"; FileRequest req;
        while (stream->Read(&req)) {
            if (fname.empty() && !req.file_name().empty()) fname = req.file_name();
            if (req.has_convert_image_format_params()) { got_params = true; if (!req.convert_image_format_params().output_format().empty()) out_ext = req.convert_image_format_params().output_format(); }
            if (req.has_file_content()) { const auto& c = req.file_content().content(); data.insert(data.end(), c.begin(), c.end()); }
        }
        if (!got_params) { FileResponse r; r.set_success(false); r.set_status_message("Parâmetros ausentes"); stream->Write(r); return Status::OK; }
        fs::create_directories(StorageDir()); fs::path in = fs::path(StorageDir()) / ("in_" + fname);
        if (!WriteAll(in.string(), data)) { FileResponse r; r.set_success(false); r.set_status_message("Falha ao salvar entrada"); stream->Write(r); LogOperation("ConvertImageFormat", fname, false, "Falha ao salvar entrada"); return Status::OK; }
        fs::path out = fs::path(StorageDir()) / (fs::path(fname).stem().string() + "." + out_ext);
        bool ok=false; std::string msg;
        if (CommandExists("convert")) {
            std::string cmd = "convert '"+in.string()+"' -strip '"+out.string()+"'"; ok=(RunShell(cmd)==0); msg = ok?"Imagem convertida":"Falha ImageMagick";
        } else { std::ofstream o(out, std::ios::binary); std::ifstream i(in, std::ios::binary); o<<i.rdbuf(); ok=o.good(); msg = ok?"Fallback: cópia":"Falha fallback"; }
        StreamFileBack(stream, out.string(), msg, ok);
        LogOperation("ConvertImageFormat", fname, ok, msg);
        return Status::OK;
    }

    Status ResizeImage(ServerContext* context,
                       ServerReaderWriter<FileResponse, FileRequest>* stream) override {
        std::string fname; std::vector<uint8_t> data; bool got_params=false; int width=512,height=512; FileRequest req;
        while (stream->Read(&req)) {
            if (fname.empty() && !req.file_name().empty()) fname = req.file_name();
            if (req.has_resize_image_params()) { got_params=true; if (req.resize_image_params().width()>0) width = req.resize_image_params().width(); if (req.resize_image_params().height()>0) height = req.resize_image_params().height(); }
            if (req.has_file_content()) { const auto& c = req.file_content().content(); data.insert(data.end(), c.begin(), c.end()); }
        }
        if (!got_params) { FileResponse r; r.set_success(false); r.set_status_message("Parâmetros ausentes"); stream->Write(r); return Status::OK; }
        fs::create_directories(StorageDir()); fs::path in = fs::path(StorageDir()) / ("in_" + fname);
        if (!WriteAll(in.string(), data)) { FileResponse r; r.set_success(false); r.set_status_message("Falha ao salvar entrada"); stream->Write(r); LogOperation("ResizeImage", fname, false, "Falha ao salvar entrada"); return Status::OK; }
        fs::path out = fs::path(StorageDir()) / (fs::path(fname).stem().string() + "_" + std::to_string(width) + "x" + std::to_string(height) + ".img");
        bool ok=false; std::string msg;
        if (CommandExists("convert")) {
            std::string size = std::to_string(width) + "x" + std::to_string(height);
            std::string cmd = "convert '"+in.string()+"' -resize " + size + " '" + out.string() + "'";
            ok=(RunShell(cmd)==0); msg = ok?"Imagem redimensionada":"Falha ImageMagick";
        } else { std::ofstream o(out, std::ios::binary); std::ifstream i(in, std::ios::binary); o<<i.rdbuf(); ok=o.good(); msg = ok?"Fallback: cópia":"Falha fallback"; }
        StreamFileBack(stream, out.string(), msg, ok);
        LogOperation("ResizeImage", fname, ok, msg);
        return Status::OK;
    }
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
