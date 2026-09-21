#include "llama.h"
#include "scheduler.hxx"
#include "worker.hxx"
#include "server.hxx"
#include "job.hxx"

#ifndef MODEL_PATH
#define MODEL_PATH ""  // override with: cmake -B build -DFORGE_MODEL_PATH=/path/to/model.gguf
#endif


int main() {
    llama_model_params mparas = llama_model_default_params();
    if (MODEL_PATH[0] == '\0') {
        std::cerr << "No model path configured. Reconfigure with "
                     "-DFORGE_MODEL_PATH=/path/to/model.gguf, or set MODEL_PATH in main.cpp."
                  << std::endl;
        return 1;
    }
    llama_model* model = llama_model_load_from_file(MODEL_PATH, mparas);
    Scheduler sch;
    Server server(8080, sch, model);
    server.start();
}
