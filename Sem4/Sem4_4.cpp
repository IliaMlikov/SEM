#include <iostream>
#include <queue>
#include <string>
#include <thread>
#include <mutex>
#include <semaphore>
#include <chrono>

struct PrintJob {
    std::string doc;
    int priority;
    int id;
    
    bool operator<(const PrintJob& other) const {
        return priority < other.priority;
    }
};

class PrinterQueue {
private:
    int n_printers;
    std::counting_semaphore<> semaphore;
    std::mutex mtx;
    std::priority_queue<PrintJob> queue;
    int next_id;
    
public:
    PrinterQueue(int printers) : n_printers(printers), semaphore(printers), next_id(0) {}
    
    void printJob(std::string doc, int priority, int timeout_ms) {
        int job_id = next_id++;
        
        {
            std::lock_guard<std::mutex> lock(mtx);
            queue.push({doc, priority, job_id});
            
            std::cout << "thread:" << std::this_thread::get_id() 
                      << " priority:" << priority 
                      << " job_id:" << job_id 
                      << " queued" << std::endl;
        }
        
        auto start = std::chrono::steady_clock::now();
        
        while (true) {
            if (semaphore.try_acquire()) {
                PrintJob job;
                bool has_job = false;
                
                {
                    std::lock_guard<std::mutex> lock(mtx);
                    if (!queue.empty() && queue.top().id == job_id) {
                        job = queue.top();
                        queue.pop();
                        has_job = true;
                    }
                }
                
                if (has_job) {
                    std::cout << "thread:" << std::this_thread::get_id() 
                              << " priority:" << priority 
                              << " job_id:" << job_id 
                              << " printing" << std::endl;
                    
                    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                    
                    semaphore.release();
                    
                    std::cout << "thread:" << std::this_thread::get_id() 
                              << " job_id:" << job_id 
                              << " completed" << std::endl;
                    
                    std::this_thread::yield();
                    return;
                } else {
                    semaphore.release();
                }
            }
            
            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count() >= timeout_ms) {
                bool removed = false;
                
                {
                    std::lock_guard<std::mutex> lock(mtx);
                    std::priority_queue<PrintJob> temp;
                    while (!queue.empty()) {
                        PrintJob j = queue.top();
                        queue.pop();
                        if (j.id != job_id) {
                            temp.push(j);
                        } else {
                            removed = true;
                        }
                    }
                    queue = temp;
                }
                
                if (removed) {
                    std::cout << "thread:" << std::this_thread::get_id() 
                              << " priority:" << priority 
                              << " job_id:" << job_id 
                              << " timeout" << std::endl;
                }
                
                std::this_thread::yield();
                return;
            }
            
            std::this_thread::yield();
        }
    }
    
    int get_queue_size() {
        std::lock_guard<std::mutex> lock(mtx);
        return queue.size();
    }
};