#include <iostream>
#include <queue>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <semaphore>
#include <chrono>

struct Task {
    int id;
    int required_slots;
    int duration_ms;
    int priority;
    std::chrono::steady_clock::time_point submit_time;
    
    bool operator<(const Task& other) const {
        return priority < other.priority;
    }
};

class TaskScheduler {
private:
    std::priority_queue<Task> queue;
    std::counting_semaphore<> resource_semaphore;
    std::mutex queue_mutex;
    std::atomic<int> completed_tasks;
    std::mutex console_mutex;
    int total_slots;
    std::vector<long long> wait_times;
    std::mutex wait_times_mutex;
    
    inline void execute_task(Task& task) {
        {
            std::lock_guard<std::mutex> lock(console_mutex);
            std::cout << "thread:" << std::this_thread::get_id() 
                      << " task_id:" << task.id 
                      << " slots:" << task.required_slots 
                      << " executing" << std::endl;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(task.duration_ms));
        
        {
            std::lock_guard<std::mutex> lock(console_mutex);
            std::cout << "thread:" << std::this_thread::get_id() 
                      << " task_id:" << task.id 
                      << " completed" << std::endl;
        }
        
        completed_tasks++;
    }
    
public:
    TaskScheduler(int total_resource_slots) 
        : resource_semaphore(total_resource_slots), completed_tasks(0), total_slots(total_resource_slots) {}
    
    void submit(Task task) {
        task.submit_time = std::chrono::steady_clock::now();
        std::lock_guard<std::mutex> lock(queue_mutex);
        queue.push(task);
        
        std::lock_guard<std::mutex> console_lock(console_mutex);
        std::cout << "task_id:" << task.id 
                  << " submitted, priority:" << task.priority 
                  << " slots:" << task.required_slots << std::endl;
    }
    
    void worker() {
        while (true) {
            Task task;
            bool has_task = false;
            
            {
                std::lock_guard<std::mutex> lock(queue_mutex);
                if (!queue.empty()) {
                    task = queue.top();
                    queue.pop();
                    has_task = true;
                }
            }
            
            if (!has_task) {
                std::this_thread::yield();
                continue;
            }
            
            auto wait_time = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - task.submit_time).count();
            {
                std::lock_guard<std::mutex> lock(wait_times_mutex);
                wait_times.push_back(wait_time);
            }
            
            {
                std::lock_guard<std::mutex> lock(console_mutex);
                std::cout << "thread:" << std::this_thread::get_id() 
                          << " task_id:" << task.id 
                          << " wait_time:" << wait_time << "ms" << std::endl;
            }
            
            for (int i = 0; i < task.required_slots; ++i) {
                resource_semaphore.acquire();
            }
            
            {
                std::lock_guard<std::mutex> lock(console_mutex);
                std::cout << "thread:" << std::this_thread::get_id() 
                          << " task_id:" << task.id 
                          << " acquired " << task.required_slots 
                          << " slots, available:" << total_slots - task.required_slots << std::endl;
            }
            
            execute_task(task);
            
            for (int i = 0; i < task.required_slots; ++i) {
                resource_semaphore.release();
            }
            
            std::this_thread::yield();
        }
    }
    
    int get_completed_tasks() const {
        return completed_tasks.load();
    }
    
    double get_average_wait_time() {
        std::lock_guard<std::mutex> lock(wait_times_mutex);
        if (wait_times.empty()) return 0.0;
        long long sum = 0;
        for (auto t : wait_times) {
            sum += t;
        }
        return static_cast<double>(sum) / wait_times.size();
    }
    
    void wait_for_completion(int expected_tasks) {
        while (completed_tasks.load() < expected_tasks) {
            std::this_thread::yield();
        }
    }
};