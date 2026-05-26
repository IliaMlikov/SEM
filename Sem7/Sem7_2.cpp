#include <iostream>
#include <vector>
#include <execution>
#include <numeric>
#include <atomic>
#include <algorithm>

int main() {
    const size_t SIZE = 1'000'000;
    std::vector<double> A(SIZE, 100.0);
    std::vector<double> B(SIZE, 99.5);

    A[500'000] = 200.0;
    A[600'000] = 50.0;

    std::vector<double> diffs(SIZE);
    std::atomic<size_t> diff_count{0};

    std::vector<size_t> indices(SIZE);
    std::iota(indices.begin(), indices.end(), 0);
    
    std::for_each(std::execution::par_unseq, indices.begin(), indices.end(),
        [&](size_t i) {
            double diff = A[i] - B[i];
            if (diff > 0) {
                size_t idx = diff_count.fetch_add(1, std::memory_order_relaxed);
                diffs[idx] = diff;
            }
        });
    
    diffs.resize(diff_count.load());

    if (diffs.empty()) {
        std::cout << "Нет положительных разностей" << std::endl;
        return 0;
    }

    double total = std::reduce(std::execution::par, diffs.begin(), diffs.end());

    std::transform(std::execution::par_unseq, diffs.begin(), diffs.end(),
                   diffs.begin(),
                   [total](double x) { return x / total; });

    std::cout << "Количество положительных разностей: " << diffs.size() << std::endl;
    std::cout << "Сумма всех разностей: " << total << std::endl;
    std::cout << "Первые 5 нормализованных значений: ";
    for (size_t i = 0; i < std::min(size_t(5), diffs.size()); ++i) {
        std::cout << diffs[i] << " ";
    }
    std::cout << std::endl;

    return 0;
}