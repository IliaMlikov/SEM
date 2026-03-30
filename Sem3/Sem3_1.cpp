#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>

template<typename T>
class ParallelSum {
private:
    std::vector<T> data;
    size_t n_threads;
    T total_sum;
    std::mutex mtx;
    std::condition_variable cv;
    size_t completed_threads;
    
    inline T sum_segment(const std::vector<T>& vec, size_t start, size_t end) {
        T sum = T(0);
        for (size_t i = start; i < end; ++i) {
            sum += vec[i];
        }
        return sum;
    }
    
public:
    ParallelSum(const std::vector<T>& vec, size_t threads) 
        : data(vec), n_threads(threads), total_sum(T(0)), completed_threads(0) {}
    
    T compute_sum() {
        size_t size = data.size();
        size_t segment_size = size / n_threads;
        
        for (size_t i = 0; i < n_threads; ++i) {
            size_t start = i * segment_size;
            size_t end = (i == n_threads - 1) ? size : (i + 1) * segment_size;
            
            std::thread t([this, start, end]() {
                T segment_sum = sum_segment(this->data, start, end);
                
                std::cout << "thread:" << std::this_thread::get_id() 
                          << " sum = " << segment_sum << std::endl;
                
                {
                    std::lock_guard<std::mutex> lock(this->mtx);
                    this->total_sum += segment_sum;
                }
                
                std::this_thread::yield();
                
                {
                    std::lock_guard<std::mutex> lock(this->mtx);
                    ++(this->completed_threads);
                }
                this->cv.notify_one();
            });
            
            t.detach();
        }
        
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this]() { return completed_threads == n_threads; });
        
        std::cout << "Total sum = " << total_sum << std::endl;
        
        return total_sum;
    }
};