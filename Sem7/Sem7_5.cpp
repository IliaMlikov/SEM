#include <iostream>
#include <vector>
#include <execution>
#include <numeric>
#include <algorithm>
#include <cmath>
#include <chrono>

int main() {
    const size_t SIZE = 1'000'000;
    std::vector<int> A(SIZE, 10);
    std::vector<int> B(SIZE, 10);
    
    B[500'000] = 5;
    B[750'000] = 3;
    B[250'000] = 15;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    auto [it1, it2] = std::mismatch(std::execution::par,
                                    A.begin(), A.end(),
                                    B.begin());
    
    auto mismatch_end = std::chrono::high_resolution_clock::now();
    
    if (it1 != A.end()) {
        size_t index = std::distance(A.begin(), it1);
        std::cout << "Первое несовпадение на индексе: " << index << std::endl;
        std::cout << "A[" << index << "] = " << *it1 << ", B[" << index << "] = " << *it2 << std::endl;
    } else {
        std::cout << "Массивы полностью совпадают" << std::endl;
    }
    
    long long abs_diff_sum = std::transform_reduce(
        std::execution::par_unseq,
        A.begin(), A.end(),
        B.begin(),
        0LL,
        std::plus<>(),
        [](int a, int b) { return static_cast<long long>(std::abs(a - b)); }
    );
    
    auto end = std::chrono::high_resolution_clock::now();
    
    std::cout << "Сумма модулей разностей всех элементов: " << abs_diff_sum << std::endl;
    std::cout << "Время поиска mismatch: " 
              << std::chrono::duration<double>(mismatch_end - start).count() << " сек" << std::endl;
    std::cout << "Время transform_reduce: " 
              << std::chrono::duration<double>(end - mismatch_end).count() << " сек" << std::endl;
    
    return 0;
}