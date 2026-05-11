#pragma once

#include <chrono>
#include <future>
#include <string>

struct Job {
    int id;

    std::string query;

    std::chrono::steady_clock::time_point arrivalTime;
    std::chrono::steady_clock::time_point startTime;
    std::chrono::steady_clock::time_point endTime;

    int oldPriority = 1;
    int currPriority = 1;

    std::shared_ptr<std::promise<std::string>> p;
};
