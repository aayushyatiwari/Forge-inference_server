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

    float currPriority = 1;
    float initPriority = 1;
    std::shared_ptr<std::promise<std::string>> p;

};
