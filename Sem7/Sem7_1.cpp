#include <iostream>
#include <vector>
#include <execution>
#include <numeric>
#include <atomic>
#include <algorithm>

int main() {
    std::vector<int> data(1'000'000);
    std::iota(data.begin(), data.end(), -500'000);

    std::vector<int> positives(data.size());
    std::atomic<size_t> pos_count{0};

    std::for_each(std::execution::par_unseq, data.begin(), data.end(),
        [&](int x) {
            if (x > 0) {
                size_t idx = pos_count.fetch_add(1, std::memory_order_relaxed);
                positives[idx] = x;
            }
        });

    positives.resize(pos_count.load());

    long long norm = std::transform_reduce(
        std::execution::par,
        positives.begin(), positives.end(),
        0LL,
        std::plus<>(),
        [](int x) { return 1LL * x * x; }
    );

    std::cout << "Положительных чисел: " << positives.size() << std::endl;
    std::cout << "Сумма квадратов (L2 норма без корня): " << norm << std::endl;

    return 0;
}