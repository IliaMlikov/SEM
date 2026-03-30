#include <iostream>
#include <thread>
#include <mutex>
#include <semaphore>
#include <chrono>

class ParkingLot {
private:
    int capacity;
    int occupied;
    std::counting_semaphore<> semaphore;
    std::mutex mtx;

public:
    ParkingLot(int size) : capacity(size), occupied(0), semaphore(size) {}

    void park(bool isVIP, int timeout_ms) {
        auto start = std::chrono::steady_clock::now();
        
        while (true) {
            if (semaphore.try_acquire()) {
                std::lock_guard<std::mutex> lock(mtx);
                occupied++;
                
                std::cout << "thread:" << std::this_thread::get_id() 
                          << " " << (isVIP ? "VIP" : "regular")
                          << " parked, occupied:" << occupied 
                          << " free:" << (capacity - occupied) << std::endl;
                
                std::this_thread::yield();
                return;
            }
            
            auto now = std::chrono::steady_clock::now();
            if (!isVIP && std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count() >= timeout_ms) {
                std::lock_guard<std::mutex> lock(mtx);
                std::cout << "thread:" << std::this_thread::get_id() 
                          << " regular timeout" << std::endl;
                return;
            }
            
            std::this_thread::yield();
        }
    }
    
    void leave() {
        semaphore.release();
        
        std::lock_guard<std::mutex> lock(mtx);
        occupied--;
        
        std::cout << "thread:" << std::this_thread::get_id() 
                  << " left, occupied:" << occupied 
                  << " free:" << (capacity - occupied) << std::endl;
        
        std::this_thread::yield();
    }
    
    void set_capacity(int new_capacity) {
        std::lock_guard<std::mutex> lock(mtx);
        int diff = new_capacity - capacity;
        capacity = new_capacity;
        if (diff > 0) {
            for (int i = 0; i < diff; ++i) {
                semaphore.release();
            }
        } else if (diff < 0) {
            for (int i = 0; i < -diff; ++i) {
                semaphore.acquire();
            }
        }
    }
};