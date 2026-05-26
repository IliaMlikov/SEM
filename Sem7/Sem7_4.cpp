#include <iostream>
#include <vector>
#include <numeric>
#include <execution>
#include <cmath>
#include <chrono>

int main() {
    const size_t SIZE = 1'000'000;
    std::vector<int> data(SIZE, 1);
    std::vector<int> prefix_sum(SIZE);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    std::inclusive_scan(std::execution::par,
                        data.begin(), data.end(),
                        prefix_sum.begin(),
                        std::plus<>());
    
    auto scan_end = std::chrono::high_resolution_clock::now();
    
    std::vector<double> logs(SIZE);
    std::transform(std::execution::par_unseq,
                   prefix_sum.begin(), prefix_sum.end(),
                   logs.begin(),
                   [](int x) { return std::log(static_cast<double>(x)); });
    
    auto end = std::chrono::high_resolution_clock::now();
    
    std::cout << "Префиксная сумма последнего элемента: " << prefix_sum.back() << std::endl;
    std::cout << "Логарифм последнего элемента: " << logs.back() << std::endl;
    std::cout << "Первые 10 префиксных сумм: ";
    for (int i = 0; i < 10; ++i) {
        std::cout << prefix_sum[i] << " ";
    }
    std::cout << std::endl;
    
    std::cout << "Время сканирования: " 
              << std::chrono::duration<double>(scan_end - start).count() << " сек" << std::endl;
    std::cout << "Время логарифмирования: " 
              << std::chrono::duration<double>(end - scan_end).count() << " сек" << std::endl;
    
    return 0;
}