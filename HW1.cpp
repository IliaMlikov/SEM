#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <boost/thread.hpp>

class FileProcessor {
private:
    std::queue<std::string> task_queue;
    std::vector<std::string> filenames;
    std::atomic<int> processed_count;
    std::mutex queue_mtx;
    std::mutex console_mtx;
    std::condition_variable cv;
    std::condition_variable queue_cv;
    int total_files;
    bool all_tasks_added;
    std::chrono::steady_clock::time_point start_time;
    int num_threads;

    void process_file(const std::string& filename) {
        std::ifstream infile(filename);
        std::string content;
        std::string line;
        
        if (infile.is_open()) {
            while (std::getline(infile, line)) {
                content += line + "\n";
            }
            infile.close();
        }
        
        int word_count = 0;
        int char_count = 0;
        bool in_word = false;
        
        for (char c : content) {
            char_count++;
            if (std::isalnum(c)) {
                if (!in_word) {
                    in_word = true;
                    word_count++;
                }
            } else {
                in_word = false;
            }
        }
        
        std::string output_filename = "processed_" + filename;
        std::ofstream outfile(output_filename);
        if (outfile.is_open()) {
            outfile << "File: " << filename << std::endl;
            outfile << "Words: " << word_count << std::endl;
            outfile << "Characters: " << char_count << std::endl;
            outfile << "Content:" << std::endl;
            outfile << content;
            outfile.close();
        }
        
        {
            std::lock_guard<std::mutex> lock(console_mtx);
            std::cout << "Processed: " << filename 
                      << " (words: " << word_count 
                      << ", chars: " << char_count << ")" << std::endl;
        }
        
        processed_count++;
        cv.notify_one();
    }
    
    void worker() {
        while (true) {
            std::string filename;
            {
                std::unique_lock<std::mutex> lock(queue_mtx);
                queue_cv.wait(lock, [this]() { 
                    return !task_queue.empty() || all_tasks_added; 
                });
                
                if (task_queue.empty() && all_tasks_added) {
                    return;
                }
                
                filename = task_queue.front();
                task_queue.pop();
            }
            
            process_file(filename);
        }
    }

public:
    FileProcessor(const std::vector<std::string>& files, int threads = 2) 
        : filenames(files), processed_count(0), total_files(files.size()), 
          all_tasks_added(false), num_threads(threads) {}
    
    void run() {
        start_time = std::chrono::steady_clock::now();
        
        for (const auto& filename : filenames) {
            task_queue.push(filename);
        }
        all_tasks_added = true;
        queue_cv.notify_all();
        
        std::vector<boost::thread> workers;
        
        for (int i = 0; i < num_threads; ++i) {
            workers.emplace_back([this]() {
                worker();
            });
        }
        
        {
            std::unique_lock<std::mutex> lock(console_mtx);
            cv.wait(lock, [this]() { 
                return processed_count.load() == total_files; 
            });
        }
        
        for (auto& t : workers) {
            if (t.joinable()) {
                t.join();
            }
        }
        
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - start_time);
        
        std::cout << "\n=== STATISTICS ===" << std::endl;
        std::cout << "Processed files: " << processed_count.load() << std::endl;
        std::cout << "Total time: " << duration.count() << " ms" << std::endl;
    }
};

int main() {
    std::vector<std::string> files = {"data1.txt", "data2.txt", "data3.txt"};
    
    FileProcessor processor(files, 2);
    processor.run();
    
    return 0;
}