#include "llama.h"
#include "scheduler.hxx"
#include "worker.hxx"
#include "server.hxx"
#include "job.hxx"

#include <cstdlib>
#include <string>

#ifndef MODEL_PATH
#define MODEL_PATH ""  // optional default: cmake -B build -DFORGE_MODEL_PATH=/path/to/model.gguf
#endif


int main(int argc, char** argv) {
    // model path precedence: argv[1] > $FORGE_MODEL_PATH > compile-time MODEL_PATH
    std::string model_path = MODEL_PATH;
    if (const char* env = std::getenv("FORGE_MODEL_PATH")) model_path = env;
    if (argc > 1) model_path = argv[1];

    if (model_path.empty()) {
        std::cerr << "No model path configured. Usage: forge /path/to/model.gguf "
                     "(or set FORGE_MODEL_PATH, or configure with -DFORGE_MODEL_PATH=...)"
                  << std::endl;
        return 1;
    }

    llama_model_params mparas = llama_model_default_params();
    llama_model* model = llama_model_load_from_file(model_path.c_str(), mparas);
    if (!model) {
        std::cerr << "Failed to load model: " << model_path << std::endl;
        return 1;
    }
    Scheduler sch;
    Server server(8080, sch, model);
    server.start();
}
