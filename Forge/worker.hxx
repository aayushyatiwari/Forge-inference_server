#pragma once // include the libraries only once no matter how many times they are included

#include <mutex> 
#include <thread>
#include <vector>
#include <cassert>
#include "scheduler.hxx"
#include "job.hxx"
#include "llama.h"

// using llama.cpp API to get a response 
// all llama.cpp code from: https://github.com/ggml-org/llama.cpp/blob/master/examples/simple/simple.cpp

class Worker {
public:
    Scheduler& sch;
    llama_model* model;
    llama_context* context;
    const llama_vocab* vocab;
    int n_predict = 1028; // number of tokens to predict

    Worker (Scheduler& s, llama_model* m) : sch(s) , model(m) {
        assert(model != nullptr);
        llama_context_params cparams = llama_context_default_params();
        context = llama_init_from_model(model, cparams); // init the specific context for the scheduler. one sch: one context
        assert(context != nullptr);
        vocab = llama_model_get_vocab(model);
    }


    void run () {
        // thread takes the job from scheduler and gives it to runINference method which then computes result
        while (true) {
            std::unique_lock<std::mutex> lock(sch.m);
            sch.cv.wait(lock, [this] {return !sch.q.empty() || sch.shutdown;}); // make the thread wake up exactly when a job comes.
            if (sch.shutdown && sch.q.empty()) break;
            std::unique_ptr<Job> job = sch.deque_locked(); // use a version that assumes lock is held or handle it
            lock.unlock();
            
            if (job) {
                std::string result = runInference(job->query);
                job->p->set_value(result);
                auto latency = (float)std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - job->startTime
                ).count();

                std::lock_guard<std::mutex> lock_metrics(sch.m);
                sch.latencies.push_back(latency);
            }
        }

    }

 std::string runInference (const std::string& prompt) {
        std::string result;

        // 1. tokenize
        // first call to get size
        int n_tokens = llama_tokenize(vocab, prompt.c_str(), prompt.size(), NULL, 0, true, true);
        std::vector<llama_token> prompt_tokens (std::abs(n_tokens));
        n_tokens = llama_tokenize(vocab, prompt.c_str(), prompt.size(), prompt_tokens.data(), prompt_tokens.size(), true, true);
        if (n_tokens < 0) {
            prompt_tokens.resize(-n_tokens);
            llama_tokenize(vocab, prompt.c_str(), prompt.size(), prompt_tokens.data(), prompt_tokens.size(), true, true);
        }

        // sampler : decides out of the batch of possible probabilities which token to choose
        // has greedy, and temperature... we will do greedy
        auto sparams = llama_sampler_chain_default_params();
        sparams.no_perf = false;
        llama_sampler* smpl = llama_sampler_chain_init(sparams);

        llama_sampler_chain_add(smpl, llama_sampler_init_greedy());

        // 2. create batch

        llama_batch batch = llama_batch_get_one(prompt_tokens.data(), prompt_tokens.size());

        // 3. look: decode, sample next token, detokenize append to result, check eog

       int n_decode = 0;
       llama_token new_token_id;
       
       for (int n_pos =0; n_pos + batch.n_tokens < (int)prompt_tokens.size() + n_predict;) {
        // evaluate current batch with transformer
        if (llama_decode(context, batch) != 0) break;
        n_pos += batch.n_tokens;

        // sample next tokens
        {
            new_token_id = llama_sampler_sample(smpl, context, -1);

            // end of generation
            if (llama_vocab_is_eog(vocab, new_token_id)) {
                break;
            }

            char buf[128];
            int n = llama_token_to_piece(vocab, new_token_id, buf, sizeof(buf), 0, true);
            std::string s(buf, n);
            result += s;
            
            printf("%s", s.c_str());
            fflush(stdout);

            // prepare the next batch with the sampled token
            batch = llama_batch_get_one(&new_token_id, 1);

            n_decode += 1;
        }
       }
        // 4. return result string
        llama_sampler_free(smpl);
        return result;
    }

    ~Worker() {
        if (context) llama_free(context); // free context after worker done
    }

}; 