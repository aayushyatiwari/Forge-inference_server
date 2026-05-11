#pragma once 

#include "job.hxx"
#include "scheduler.hxx"
#include "llama.h"

// using llama.cpp API to get a response 
// all llama.cpp code from: https://github.com/ggml-org/llama.cpp/blob/master/examples/simple/simple.cpp

class Worker {
public:

    Scheduler& sch;
    llama_model* model;
    llama_context* context;
    const llama_vocab* vocab;
    int n_predict = 1028;
    
    Worker (Scheduler& s, llama_model* m, llama_context* c) : sch(s) , model(m), context(c) {
        vocab = llama_model_get_vocab(model);
        assert(model!=nullptr);
        assert(context!=nullptr);
    }

    void run() {
        while (true) {
            std::unique_lock<std::mutex> lock(sch.m);
            sch.cv.wait(lock, [this] {return !sch.p.empty(); });
            std::unique_ptr<Job> job = std::move(const_cast<std::unique_ptr<Job>&>(sch.p.top()));
            sch.p.pop();
            lock.unlock();
            std::string result = runInference(job->query);
            job->p->set_value(result);
            auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - job->startTime
            ).count();
            std::lock_guard<std::mutex> lk(sch.m);
            sch.latencies.push_back(latency);
        }
    }
    std::string runInference (const std::string& prompt) {
        std::string result;

        // 1. tokenize
        const int n_prompt = -llama_tokenize(vocab, prompt.c_str(), prompt.size(), NULL, 0, true, true);
        std::vector<llama_token> prompt_tokens (n_prompt);
        llama_tokenize(vocab, prompt.c_str(), prompt.size(), prompt_tokens.data(), prompt_tokens.size(), true, true);

        // sampler : decides out of the batch of possible probabilities which token to choose
        // has greedy, and temperature... we will do greedy
        auto sparams = llama_sampler_chain_default_params();
        sparams.no_perf = false;
        llama_sampler* smpl = llama_sampler_chain_init(sparams);

        llama_sampler_chain_add(smpl, llama_sampler_init_greedy());

        // print the prompt token-by-token

        // for (auto id : prompt_tokens) {
        //     char buf[128];
        //     int n = llama_token_to_piece(vocab, id, buf, sizeof(buf), 0, true);
        //     if (n < 0) {
        //         fprintf(stderr, "%s: error: failed to convert token to piece\n", __func__);
        //         return "";
        //     }
        //     std::string s(buf, n);
        //     printf("%s", s.c_str());
        // }

        // 2. create batch

        llama_batch batch = llama_batch_get_one(prompt_tokens.data(), prompt_tokens.size());

        if (llama_model_has_encoder(model)) {
            llama_token decoder_start_token_id = llama_model_decoder_start_token(model);
            if (decoder_start_token_id == LLAMA_TOKEN_NULL){
                decoder_start_token_id = llama_vocab_bos(vocab);
            }
            batch = llama_batch_get_one(&decoder_start_token_id, 1);
        }

        // 3. look: decode, sample next token, detokenize append to result, check eog

       int n_decode = 0;
       llama_token new_token_id;
       
       for (int n_pos =0; n_pos+ batch.n_tokens < n_prompt + n_predict;) {
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

};