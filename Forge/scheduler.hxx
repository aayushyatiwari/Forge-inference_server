#pragma once

#include<iostream>
#include<mutex>
#include<memory>
#include<queue>
#include<thread>
#include "job.hxx"

// ref: https://blog.andreiavram.ro/job-scheduler-cpp/

struct Compare {
    bool operator() (const std::unique_ptr<Job>& a,const std::unique_ptr<Job>& b){
        return a->currPriority < b->currPriority;
    }
};

class Scheduler {
public: 
    std::priority_queue<std::unique_ptr<Job>, std::vector<std::unique_ptr<Job>>, Compare> p; // can't take only job objects, since we want automatic memory handling
    std::thread aging; // thread that will implement aging
    float aging_rate = 0.2;
    std::atomic<bool> shutdown;
    std::mutex m;
    std::condition_variable cv; // what if the scheduler is empty? how to reduce CPU load?
    std::vector <float> latencies;


    Scheduler () {
        shutdown = false ;
        aging = std::thread(&Scheduler::agingLoop, this);
    }

    void enqueue (std::unique_ptr<Job> job) {
        // add job
        std::unique_lock<std::mutex> lock(m);
        p.push(std::move(job));
        cv.notify_one();
    }

    std::unique_ptr<Job> dequeue () {
        // return top job and pop 
        std::lock_guard<std::mutex> lock(m);
        if (p.size() == 0) {
            return nullptr;
        } else {
            std::unique_ptr temp = std::move(const_cast<std::unique_ptr<Job>&>(p.top()));
            p.pop();
            return temp;
        }
    }

    void agingLoop () {
        // implement aging
        while (!shutdown) {

            // pull everything out in a temp vector
            std::this_thread::sleep_for(std::chrono::seconds(1));
            std::vector <std::unique_ptr<Job>> temp;
            std::lock_guard<std::mutex> lock(m);

            while (p.size() != 0) {
                temp.push_back(std::move(const_cast<std::unique_ptr<Job>&>(p.top())));
                p.pop();
            }

            // fix currPriority and put everything back
            for (auto& i : temp) {
                i->currPriority += aging_rate;
                p.push(std::move(i));
            }
        }
    }

    size_t size() {
        std::lock_guard<std::mutex> lock(m);

        return p.size();
    }
    
    ~Scheduler () {
        shutdown = true ;
        aging.join();
    }

};
