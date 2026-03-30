#include <iostream>
#include <thread>
#include <semaphore>
#include <chrono>
#include <random>
#include <vector>

class WarehouseManager {
private:
    std::counting_semaphore<> machine_sem;
    std::counting_semaphore<> stock_sem;
    std::mutex console_mtx;
    std::atomic<int> processed_items;

public:
    WarehouseManager(int machines, int stock) 
        : machine_sem(machines), stock_sem(stock), processed_items(0) {}

    void process(int worker_id) {
        stock_sem.acquire();
        
        machine_sem.acquire();
        
        {
            std::lock_guard<std::mutex> lock(console_mtx);
            std::cout << "Worker " << worker_id << " started processing" << std::endl;
        }
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dist(1000, 2000);
        std::this_thread::sleep_for(std::chrono::milliseconds(dist(gen)));
        
        {
            std::lock_guard<std::mutex> lock(console_mtx);
            std::cout << "Worker " << worker_id << " finished processing" << std::endl;
        }
        
        machine_sem.release();
        
        stock_sem.release();
        
        processed_items++;
        std::this_thread::yield();
    }
    
    int get_processed() const {
        return processed_items.load();
    }
};

int main() {
    WarehouseManager manager(3, 10);
    std::vector<std::thread> workers;
    
    for (int i = 1; i <= 10; ++i) {
        workers.emplace_back([&manager, i]() {
            manager.process(i);
        });
    }
    
    for (auto& t : workers) {
        t.join();
    }
    
    return 0;
}