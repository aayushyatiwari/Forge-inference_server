#include "llama.h"
#include "scheduler.hxx"
#include "worker.hxx"
#include "server.hxx"
#include "job.hxx"

#define MODEL_PATH ""


int main() {
    llama_model_params mparas = llama_model_default_params();
    if (MODEL_PATH[0] == '\0') {
        std::cerr << "Please set the MODEL_PATH macro to the path of the model file in main.cpp" << std::endl;
        return 1;
    }
    llama_model* model = llama_model_load_from_file(MODEL_PATH, mparas);
    Scheduler sch;
    Server server(8080, sch, model);
    server.start();
}
