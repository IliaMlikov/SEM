#include <iostream>
#include <queue>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>

template<typename T>
struct PriorityItem {
    T value;
    int priority;
    
    bool operator<(const PriorityItem& other) const {
        return priority < other.priority;
    }
};

template<typename T>
class PriorityQueue {
private:
    std::priority_queue<PriorityItem<T>> queue;
    std::mutex mtx;
    std::condition_variable cv;
    
public:
    void push(T value, int priority) {
        {
            std::lock_guard<std::mutex> lock(mtx);
            queue.push({value, priority});
            std::cout << "thread:" << std::this_thread::get_id() 
                      << " push: " << value << " (priority=" << priority << ")" << std::endl;
        }
        cv.notify_one();
    }
    
    T pop() {
        std::unique_lock<std::mutex> lock(mtx);
        
        cv.wait(lock, [this]() {
            return !queue.empty();
        });
        
        T value = queue.top().value;
        queue.pop();
        
        std::cout << "thread:" << std::this_thread::get_id() 
                  << " pop: " << value << std::endl;
        
        std::this_thread::yield();
        
        return value;
    }
    
    bool empty() {
        std::lock_guard<std::mutex> lock(mtx);
        return queue.empty();
    }
    
    size_t size() {
        std::lock_guard<std::mutex> lock(mtx);
        return queue.size();
    }
};