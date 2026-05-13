#pragma once
#include<mutex>
#include<chrono>
#include<string>
#include<future>

struct Job {

    int id; // good for logging, not required for minimal
    std::string query;
    
    std::chrono::steady_clock::time_point startTime;
    std::chrono::steady_clock::time_point endTime;

    int currPriority = 1;
    int initPriority = 1;
    std::shared_ptr<std::promise<std::string>> p;

};