#pragma once

#include "worker.hxx"
#include "scheduler.hxx"
#include "job.hxx"
#include "llama.h"
#include <nlohmann/json.hpp>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <iostream>
#include <algorithm>
#include <vector>

class Server {
public:
    int server_fd;
    int n_workers; // number of worker threads
    int port; 
    std::vector<std::unique_ptr<Worker>> workers;
    std::vector<std::thread> threads;
    Scheduler& sch;
    llama_model* model; // hmm model in the server class?

    Server(int p, Scheduler& s, llama_model* m, int n = 4) : port (p), sch(s) , model(m), n_workers(n) {}

    // start the server 
    void start() {
        // create worker threads
        for (int i = 0; i<n_workers; i++) {
            workers.emplace_back(std::make_unique<Worker> (sch, model));
            threads.emplace_back(&Worker::run, workers.back().get()); // .back -> vector op, .get() -> unique_ptr method
        }

        // create socket
        server_fd = socket(AF_INET, SOCK_STREAM, 0);

        if (server_fd < 0) {perror("socket"); return;} // perror -> print error

        // configure address
        sockaddr_in addr{}; // stardard socket api code
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);

        // bind socket to prot 
        bind(server_fd, (sockaddr*)&addr, sizeof(addr));

        // start listening
        listen(server_fd, 10);
        std::cout << "server listening on port: " << port << "\n";

        // accept
        while(true) {
            int client_fd = accept(server_fd, nullptr, nullptr);
            std::thread ([this, client_fd](){ // spawn threads for each new request. one thread per request.
                handleClient(client_fd);
            }).detach();
        }
    }

    // handle POST /infer
    void handleClient(int client_fd) {
        char buf [ 4096]= {0} ; // buffer for result

        // read reqest
        int bytes = recv(client_fd, buf, sizeof(buf), 0);
        if (bytes < 0) {close(client_fd);return;}

        std::string request (buf, bytes);

        if (request.find("POST /infer") != std::string::npos) {
            std::cout << request << "\n";
            size_t pos = request.find("\r\n\r\n");
            if (pos == std::string::npos) {
                close(client_fd);
                return;
            }
            std::string body = request.substr(pos+4);
            auto j = nlohmann::json::parse(body);
            std::string query = j["query"];

            // create a new job with query
            auto job = std::make_unique<Job> (); // create a new empty job unique_ptr
            // set job data
            job->query = query;
            job->p = std::make_shared<std::promise<std::string>> ();// do we need this?
            job->startTime = std::chrono::steady_clock::now();
            auto future = job->p->get_future(); 
            // the detached thread holds the future, the worker thread fulfills the promise. by job->p->set_value(result);
            // Two different threads, one communication channel.
            sch.enque(std::move(job));
            // wait here....
            std::string result = future.get();
            result += "\n";
            std::string response =
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: application/json\r\n"
                "Content-Length: " + std::to_string(result.size()) + "\r\n"
                "\r\n" +
                result;

            send(client_fd, response.c_str(), response.size(), 0);
            close(client_fd);

            } else if (request.find("GET /metrics") != std::string::npos) {
                handleMetrics(client_fd);
                return;
            } else {
                close(client_fd);
            }
        }

    // handle POST /metrics
    void handleMetrics(int client_fd) {
        std::lock_guard<std::mutex> lock(sch.m);
        std::vector<float> sorted = sch.latencies;
        std::sort(sorted.begin(), sorted.end());
        float p99 = 0.0f;
        if (!sorted.empty()) {
            p99 = sorted[(int) (0.99 * (sorted.size() - 1))];
        }
        nlohmann::json j;
        j["p99_latency_ms"] = p99;
        std::string body = j.dump();
        std::string response =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: " + std::to_string(body.size()) + "\r\n"
            "\r\n" + body;
        send(client_fd, response.c_str(), response.size(), 0);
        close(client_fd);
    }

    ~Server() {
        for (auto& t : threads) {
            if (t.joinable()) t.join();
        }
    }
};