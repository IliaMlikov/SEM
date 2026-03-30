#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <semaphore>
#include <chrono>
#include <random>

template<typename T>
class SemaphoreBuffer {
private:
    std::vector<std::vector<T>> buffers;
    std::vector<std::counting_semaphore<>> empty;
    std::vector<std::counting_semaphore<>> full;
    std::vector<std::mutex> mtx;
    size_t buffer_size;
    
public:
    SemaphoreBuffer(int num_buffers, size_t size) : buffer_size(size) {
        for (int i = 0; i < num_buffers; ++i) {
            buffers.emplace_back();
            buffers[i].reserve(size);
            empty.emplace_back(size);
            full.emplace_back(0);
            mtx.emplace_back();
        }
    }
    
    void produce(T value, int buffer_index, int timeout_ms) {
        auto start = std::chrono::steady_clock::now();
        
        while (true) {
            if (empty[buffer_index].try_acquire()) {
                std::lock_guard<std::mutex> lock(mtx[buffer_index]);
                buffers[buffer_index].push_back(value);
                
                std::cout << "thread:" << std::this_thread::get_id() 
                          << " buffer:" << buffer_index 
                          << " produced:" << value << std::endl;
                
                full[buffer_index].release();
                std::this_thread::yield();
                return;
            }
            
            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count() >= timeout_ms) {
                std::cout << "thread:" << std::this_thread::get_id() 
                          << " buffer:" << buffer_index 
                          << " produce timeout" << std::endl;
                return;
            }
            
            std::this_thread::yield();
        }
    }
    
    T consume(int buffer_index, int timeout_ms) {
        auto start = std::chrono::steady_clock::now();
        
        while (true) {
            if (full[buffer_index].try_acquire()) {
                std::lock_guard<std::mutex> lock(mtx[buffer_index]);
                T value = buffers[buffer_index].front();
                buffers[buffer_index].erase(buffers[buffer_index].begin());
                
                std::cout << "thread:" << std::this_thread::get_id() 
                          << " buffer:" << buffer_index 
                          << " consumed:" << value << std::endl;
                
                empty[buffer_index].release();
                std::this_thread::yield();
                return value;
            }
            
            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count() >= timeout_ms) {
                std::cout << "thread:" << std::this_thread::get_id() 
                          << " buffer:" << buffer_index 
                          << " consume timeout" << std::endl;
                return T();
            }
            
            std::this_thread::yield();
        }
    }
    
    int get_random_buffer() {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, buffers.size() - 1);
        return dis(gen);
    }
    
    void produce_random(T value, int timeout_ms) {
        int buffer_index = get_random_buffer();
        produce(value, buffer_index, timeout_ms);
    }
    
    T consume_random(int timeout_ms) {
        int buffer_index = get_random_buffer();
        return consume(buffer_index, timeout_ms);
    }
    
    size_t num_buffers() const {
        return buffers.size();
    }
    
    size_t buffer_size() const {
        return buffer_size;
    }
};