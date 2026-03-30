#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>

template<typename T>
class Buffer {
private:
    std::vector<T> buffer;
    size_t capacity;
    std::mutex mtx;
    std::condition_variable cv_produce;
    std::condition_variable cv_consume;
    
public:
    Buffer(size_t size) : capacity(size) {
        buffer.reserve(capacity);
    }
    
    void produce(T value) {
        std::unique_lock<std::mutex> lock(mtx);
        
        cv_produce.wait(lock, [this]() {
            return buffer.size() < capacity;
        });
        
        buffer.push_back(value);
        std::cout << "thread:" << std::this_thread::get_id() 
                  << " produced: " << value << std::endl;
        
        std::this_thread::yield();
        
        cv_consume.notify_one();
    }
    
    T consume() {
        std::unique_lock<std::mutex> lock(mtx);
        
        cv_consume.wait(lock, [this]() {
            return !buffer.empty();
        });
        
        T value = buffer.front();
        buffer.erase(buffer.begin());
        std::cout << "thread:" << std::this_thread::get_id() 
                  << " consumed: " << value << std::endl;
        
        std::this_thread::yield();
        
        cv_produce.notify_one();
        
        return value;
    }
};