#pragma once 

#include<sys/socket.h>
#include<netinet/in.h>
#include<unistd.h>
#include<cstring>
#include<iostream>
#include<string>

#include "scheduler.hxx"
#include "job.hxx"
#include "llama.h"
#include "nlohmann/json.hpp"
#include "worker.hxx"

class Server {
public: 
    int server_fd;
    int nWorkers;
    int port;
    std::vector<std::unique_ptr<Worker>> workers;
    std::vector<std::thread> threads;
    Scheduler& sch;
    llama_model* model;
    // llama_context* context;
    Server(int p, Scheduler& s, llama_model* m, int n = 4)
        : port(p), sch(s), model(m), nWorkers(n) {}

    void start() {

        // creating nWorker threads
        for (int i = 0; i < nWorkers; i++) {
            workers.emplace_back(std::make_unique<Worker>(sch, model));
            threads.emplace_back(&Worker::run, workers.back().get());
        }

        // 1. create socket
        server_fd = socket(AF_INET, SOCK_STREAM, 0);

        if (server_fd < 0) {
            perror("socket");
            return; 
        }

        // 2. configure address
        sockaddr_in addr{};
        addr.sin_family = AF_INET; // AF_INET : constant for IPv4 address
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);

        // 3. bind socket to port
        if (bind(server_fd, (sockaddr*)&addr, sizeof(addr)) < 0){
            perror("bind");
            return;
        }

        // 4. start listening
        if (listen(server_fd, 10) < 0) {
            perror("lisetn");
            return;
        }

        std::cout << "Server listening on port " << port << "\n";

        // 5. accepting 
        while (true) {
            int client_fd = accept(server_fd, nullptr, nullptr);
            if (client_fd< 0) {
                perror ("accept");
                continue;
            }
            std::thread([this, client_fd]() {
                handleClient(client_fd);
                }).detach();
            // for (int i =0;i<threads.size(); i++) {
            //     threads[i].join();
            // }

            // handleClient(client_fd);
        }
    }

    void handleClient(int client_fd) {

        char buf[4096] = {0};

        // read request
        int bytes = recv(client_fd, buf, sizeof(buf), 0);

        if (bytes <= 0) {
            close(client_fd); 
            return;
        }

        std::string request (buf, bytes);
        if (request.find("\r\n\r\n") == std::string::npos) {
            close(client_fd);
            return;
        }
        if (request.find("POST /infer") != std::string::npos) {

            std::cout << request <<"\n";
            size_t pos = request.find("\r\n\r\n");
            std::string body = request.substr(pos+4);
            auto j = nlohmann::json::parse(body);
            std::string query = j["query"];

            auto job = std::make_unique<Job> ();
            job->query = query;
            job->oldPriority = 1;
            job->currPriority= 1;
            job->p = std::make_shared<std::promise<std::string>>();
            job->startTime = std::chrono::steady_clock::now();
            auto future = job->p->get_future();
            sch.enqueue(std::move(job));
            //wait
            std::string result = future.get();
            result+="\n";

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
        }
    }

    void handleMetrics(int client_fd) {
        std::lock_guard<std::mutex> lock(sch.m);
        std::vector<float> sorted = sch.latencies;
        std::sort(sorted.begin(), sorted.end());
        float p99 = 0.0f;
        if (!sorted.empty()) {
            p99 = sorted[(int)(0.99 * sorted.size())];
        }
        nlohmann::json j;
        j["queue_depth"] = sch.p.size();
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
        for (auto& t : threads) t.join();
    }
};
