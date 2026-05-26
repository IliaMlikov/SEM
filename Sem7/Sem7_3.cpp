#include <iostream>
#include <vector>
#include <random>
#include <execution>
#include <algorithm>
#include <atomic>
#include <chrono>

int main() {
    const size_t SIZE = 2'000'000;
    std::vector<int> data(SIZE);

    std::mt19937 rng(42);
    std::uniform_int_distribution<int> dist(0, 2'000'000 - 1);
    
    for (auto& val : data) {
        val = dist(rng);
    }

    std::vector<int> even(SIZE);
    std::atomic<size_t> even_count{0};
    
    auto start = std::chrono::high_resolution_clock::now();
    
    std::for_each(std::execution::par_unseq, data.begin(), data.end(),
        [&](int x) {
            if (x % 2 == 0) {
                size_t idx = even_count.fetch_add(1, std::memory_order_relaxed);
                even[idx] = x;
            }
        });
    
    even.resize(even_count.load());
    
    auto filter_end = std::chrono::high_resolution_clock::now();

    std::sort(std::execution::par, even.begin(), even.end());
    
    auto sort_end = std::chrono::high_resolution_clock::now();

    auto large_count = std::count_if(std::execution::par, 
                                     even.begin(), even.end(),
                                     [](int x) { return x > 1'000'000; });
    
    auto end = std::chrono::high_resolution_clock::now();
    
    std::cout << "Всего чисел: " << SIZE << std::endl;
    std::cout << "Чётных чисел: " << even.size() << std::endl;
    std::cout << "Чётных чисел > 1'000'000: " << large_count << std::endl;
    std::cout << "Время фильтрации: " 
              << std::chrono::duration<double>(filter_end - start).count() << " сек" << std::endl;
    std::cout << "Время сортировки: " 
              << std::chrono::duration<double>(sort_end - filter_end).count() << " сек" << std::endl;
    
    return 0;
}