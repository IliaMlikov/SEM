#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>

template<typename T>
class MatrixProcessor {
private:
    std::vector<std::vector<T>> matrix;
    size_t n_threads;
    std::mutex mtx;
    std::condition_variable cv;
    size_t completed_threads;
    
public:
    MatrixProcessor(const std::vector<std::vector<T>>& mat, size_t threads) 
        : matrix(mat), n_threads(threads), completed_threads(0) {}
    
    void apply(std::function<T(T)> func) {
        if (matrix.empty() || matrix[0].empty()) return;
        
        size_t rows = matrix.size();
        size_t cols = matrix[0].size();
        size_t total_elements = rows * cols;
        size_t segment_size = total_elements / n_threads;
        
        for (size_t i = 0; i < n_threads; ++i) {
            size_t start = i * segment_size;
            size_t end = (i == n_threads - 1) ? total_elements : (i + 1) * segment_size;
            
            std::thread t([this, start, end, rows, cols, func]() {
                std::cout << "thread:" << std::this_thread::get_id() 
                          << " start segment [" << start << ", " << end << ")" << std::endl;
                
                for (size_t idx = start; idx < end; ++idx) {
                    size_t row = idx / cols;
                    size_t col = idx % cols;
                    matrix[row][col] = func(matrix[row][col]);
                }
                
                std::this_thread::yield();
                
                std::cout << "thread:" << std::this_thread::get_id() 
                          << " end segment" << std::endl;
                
                {
                    std::lock_guard<std::mutex> lock(mtx);
                    ++completed_threads;
                }
                cv.notify_one();
            });
            
            t.detach();
        }
        
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this]() { return completed_threads == n_threads; });
    }
    
    const std::vector<std::vector<T>>& get_matrix() const {
        return matrix;
    }
};