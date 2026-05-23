#pragma once


#include "job.hxx"
#include <queue>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <vector>
#include <memory>

// ref: https://blog.andreiavram.ro/job-scheduler-cpp/

class Scheduler {
public:
    struct JobComparator {
        bool operator()(const std::unique_ptr<Job>& a, const std::unique_ptr<Job>& b) {
            return a->currPriority < b->currPriority;
        }
    };

    std::priority_queue<std::unique_ptr<Job>, std::vector<std::unique_ptr<Job>>, JobComparator> q;
    std::mutex m;
    std::thread aging_thread; // thread that will implement aging
    std::atomic<bool> shutdown;
    float aging_rate = 0.2;
    std::condition_variable cv; // what if the scheduler is empty? how to reduce CPU load?
    std::deque<float> latencies;
    std::int32_t MAX_QUEUE_SIZE = 1024;

    Scheduler() {
        shutdown = false;
        aging_thread = std::thread(&Scheduler::aging_loop,this);
    }

    void aging_loop () {
        // aging
        while(!shutdown) {

            std::this_thread::sleep_for(std::chrono::seconds(1));
            std::vector<std::unique_ptr<Job>> t;
            {
                std::lock_guard<std::mutex> lock(m);

                while (!q.empty()) {
                    t.push_back(std::move(const_cast<std::unique_ptr<Job>&>(q.top())));
                    q.pop();
                }
                for (auto& i : t) {
                    i->currPriority += aging_rate;
                    q.push(std::move(i));
                }
            }
            if (!t.empty()) cv.notify_all();
        }
    }

    void enque(std::unique_ptr<Job> job) {
        std::lock_guard<std::mutex> lock(m);
        q.push(std::move(job));
        cv.notify_one();
    }

    std::unique_ptr<Job> deque() {
        std::lock_guard<std::mutex> lock(m);
        if (q.empty()) {
            return nullptr;
        } else {
            std::unique_ptr<Job> temp = std::move(const_cast<std::unique_ptr<Job>&>(q.top()));
            q.pop();
            return temp;
        }
    }

    std::unique_ptr<Job> deque_locked() {
        if (q.empty()) {
            return nullptr;
        } else {
            std::unique_ptr<Job> temp = std::move(const_cast<std::unique_ptr<Job>&>(q.top()));
            q.pop();
            return temp;
        }
    }

    size_t size() {
        std::lock_guard<std::mutex> lock(m);
        return q.size();
    }

    ~Scheduler () {
        shutdown = true;
        cv.notify_all();
        if (aging_thread.joinable()) aging_thread.join();
    }

};
