#include "llama.h"
#include "scheduler.hxx"
#include "worker.hxx"
#include "http.hxx"
#include "job.hxx"

#define MODEL_PATH "/home/imaayush/code/Forge/Forge/Llama-3.2-1B-Instruct-Q4_K_M.gguf"


int main() {
    llama_model_params mparas = llama_model_default_params();
    llama_model* model = llama_model_load_from_file(MODEL_PATH, mparas);
    Scheduler sch;
    Server server(8080, sch, model);
    server.start();
}