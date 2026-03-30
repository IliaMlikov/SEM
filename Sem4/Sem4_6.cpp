#include <iostream>
#include <queue>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <semaphore>
#include <chrono>

struct FileChunk {
    int chunk_id;
    int file_id;
    size_t size;
    
    void download() {
        std::this_thread::sleep_for(std::chrono::milliseconds(size / 100));
    }
};

class FileDownload {
private:
    int file_id;
    std::vector<FileChunk> chunks;
    std::atomic<int> downloaded_chunks;
    
public:
    FileDownload(int id, int num_chunks, size_t chunk_size) 
        : file_id(id), downloaded_chunks(0) {
        for (int i = 0; i < num_chunks; ++i) {
            chunks.push_back({i, id, chunk_size});
        }
    }
    
    bool is_complete() const {
        return downloaded_chunks.load() == chunks.size();
    }
    
    void mark_chunk_downloaded() {
        downloaded_chunks++;
    }
    
    int get_file_id() const {
        return file_id;
    }
    
    const std::vector<FileChunk>& get_chunks() const {
        return chunks;
    }
};

class DownloadManager {
private:
    std::queue<FileChunk> queue;
    std::counting_semaphore<> active_downloads;
    std::counting_semaphore<> chunk_downloads;
    std::mutex queue_mutex;
    std::mutex console_mutex;
    std::atomic<int> completed_files;
    std::vector<FileDownload> files;
    std::mutex files_mutex;
    
    inline void process_chunk(FileChunk chunk) {
        {
            std::lock_guard<std::mutex> lock(console_mutex);
            std::cout << "thread:" << std::this_thread::get_id() 
                      << " file_id:" << chunk.file_id 
                      << " chunk_id:" << chunk.chunk_id 
                      << " downloading" << std::endl;
        }
        
        chunk.download();
        
        {
            std::lock_guard<std::mutex> lock(console_mutex);
            std::cout << "thread:" << std::this_thread::get_id() 
                      << " file_id:" << chunk.file_id 
                      << " chunk_id:" << chunk.chunk_id 
                      << " completed" << std::endl;
        }
        
        for (auto& file : files) {
            if (file.get_file_id() == chunk.file_id) {
                file.mark_chunk_downloaded();
                if (file.is_complete()) {
                    completed_files++;
                }
                break;
            }
        }
    }
    
public:
    DownloadManager(int max_active_files, int max_active_chunks) 
        : active_downloads(max_active_files), chunk_downloads(max_active_chunks), completed_files(0) {}
    
    void add_file(FileDownload file) {
        {
            std::lock_guard<std::mutex> lock(files_mutex);
            files.push_back(file);
        }
        
        for (const auto& chunk : file.get_chunks()) {
            std::lock_guard<std::mutex> lock(queue_mutex);
            queue.push(chunk);
        }
        
        std::lock_guard<std::mutex> lock(console_mutex);
        std::cout << "file_id:" << file.get_file_id() 
                  << " added, chunks:" << file.get_chunks().size() << std::endl;
    }
    
    void download_worker() {
        while (true) {
            FileChunk chunk;
            bool has_chunk = false;
            
            {
                std::lock_guard<std::mutex> lock(queue_mutex);
                if (!queue.empty()) {
                    chunk = queue.front();
                    queue.pop();
                    has_chunk = true;
                }
            }
            
            if (!has_chunk) {
                if (completed_files.load() == files.size()) {
                    return;
                }
                std::this_thread::yield();
                continue;
            }
            
            active_downloads.acquire();
            chunk_downloads.acquire();
            
            process_chunk(chunk);
            
            chunk_downloads.release();
            active_downloads.release();
            
            std::this_thread::yield();
        }
    }
    
    int get_completed_files() const {
        return completed_files.load();
    }
};