#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <random>
#include <vector>

class ResourceManager {
private:
    int available_machines;
    std::mutex mtx;
    std::condition_variable cv;
    std::atomic<int> workers_completed;
    std::mutex console_mtx;

public:
    ResourceManager(int machines) : available_machines(machines), workers_completed(0) {}

    void use_machine(int worker_id) {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this]() { return available_machines > 0; });
        available_machines--;

        {
            std::lock_guard<std::mutex> console_lock(console_mtx);
            std::cout << "Worker " << worker_id << " started working" << std::endl;
        }

        lock.unlock();

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dist(1000, 3000);
        std::this_thread::sleep_for(std::chrono::milliseconds(dist(gen)));

        lock.lock();
        available_machines++;
        {
            std::lock_guard<std::mutex> console_lock(console_mtx);
            std::cout << "Worker " << worker_id << " finished working" << std::endl;
        }

        workers_completed++;
        cv.notify_one();
    }

    void wait_for_completion(int total_workers) {
        while (workers_completed.load() < total_workers) {
            std::this_thread::yield();
        }
    }
};

int main() {
    ResourceManager manager(3);
    std::vector<std::thread> workers;

    for (int i = 1; i <= 5; ++i) {
        workers.emplace_back([&manager, i]() {
            manager.use_machine(i);
        });
    }

    for (auto& t : workers) {
        t.join();
    }

    manager.wait_for_completion(5);

    return 0;
}