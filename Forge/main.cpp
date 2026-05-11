#include "llama.h"
#include "scheduler.hxx"
#include "job.hxx"
#include "http.hxx"
#include "worker.hxx"

int main() {
    llama_model_params mparams = llama_model_default_params();
    llama_model* model = llama_model_load_from_file("/home/imaayush/code/Forge/Forge/Llama-3.2-1B-Instruct-Q4_K_M.gguf", mparams); 
    Scheduler sch;
    Server server(8080, sch, model);
    server.start();
}