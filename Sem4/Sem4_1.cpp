#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <semaphore>
#include <chrono>

template<typename T>
class ResourcePool {
private:
    std::vector<T> resources;
    std::counting_semaphore<> semaphore;
    std::mutex mtx;
    std::atomic<int> failed_attempts;
    
public:
    ResourcePool(size_t size) : semaphore(size), failed_attempts(0) {
        for (size_t i = 0; i < size; ++i) {
            resources.push_back(T());
        }
    }
    
    T acquire(int priority, int timeout_ms) {
        auto start = std::chrono::steady_clock::now();
        
        while (true) {
            if (semaphore.try_acquire()) {
                std::lock_guard<std::mutex> lock(mtx);
                T res = resources.back();
                resources.pop_back();
                
                std::cout << "thread:" << std::this_thread::get_id() 
                          << " priority:" << priority 
                          << " acquire" << std::endl;
                
                std::this_thread::yield();
                return res;
            }
            
            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count() >= timeout_ms) {
                failed_attempts++;
                std::cout << "thread:" << std::this_thread::get_id() 
                          << " priority:" << priority 
                          << " timeout" << std::endl;
                return T();
            }
            
            std::this_thread::yield();
        }
    }
    
    void release(T res) {
        {
            std::lock_guard<std::mutex> lock(mtx);
            resources.push_back(res);
        }
        semaphore.release();
        
        std::cout << "thread:" << std::this_thread::get_id() 
                  << " release" << std::endl;
        
        std::this_thread::yield();
    }
    
    int get_failed_attempts() const {
        return failed_attempts.load();
    }
    
    void add_resource(T res) {
        std::lock_guard<std::mutex> lock(mtx);
        resources.push_back(res);
        semaphore.release();
    }
    
    void remove_resource() {
        std::lock_guard<std::mutex> lock(mtx);
        if (!resources.empty()) {
            resources.pop_back();
            semaphore.acquire();
        }
    }
};