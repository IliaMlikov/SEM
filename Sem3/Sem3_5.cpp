#include <iostream>
#include <map>
#include <thread>
#include <mutex>
#include <condition_variable>

template<typename Key, typename Value>
class Cache {
private:
    std::map<Key, Value> data;
    std::mutex mtx;
    std::condition_variable cv;
    
public:
    inline void set(const Key& key, const Value& value) {
        std::lock_guard<std::mutex> lock(mtx);
        data[key] = value;
        std::cout << "thread:" << std::this_thread::get_id() 
                  << " set: " << key << " = " << value << std::endl;
        cv.notify_all();
    }
    
    inline Value get(const Key& key) {
        std::unique_lock<std::mutex> lock(mtx);
        
        cv.wait(lock, [this, &key]() {
            return data.find(key) != data.end();
        });
        
        std::cout << "thread:" << std::this_thread::get_id() 
                  << " get: " << key << " = " << data[key] << std::endl;
        
        std::this_thread::yield();
        
        return data[key];
    }
    
    void print_all() {
        std::lock_guard<std::mutex> lock(mtx);
        std::cout << "thread:" << std::this_thread::get_id() << " cache contents: ";
        for (const auto& pair : data) {
            std::cout << pair.first << ":" << pair.second << " ";
        }
        std::cout << std::endl;
    }
};