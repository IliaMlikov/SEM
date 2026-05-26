#include <boost/asio.hpp>
#include <iostream>
#include <atomic>
#include <random>
#include <memory>
#include <vector>
#include <thread>

namespace asio = boost::asio;

class BankAccount : public std::enable_shared_from_this<BankAccount> {
public:
    using executor_type = asio::io_context::executor_type;
    
    explicit BankAccount(asio::io_context& io_context, int64_t initial_balance = 0)
        : strand_(asio::make_strand(io_context)), balance_(initial_balance) {}
    
    asio::awaitable<void> async_deposit(int64_t amount) {
        if (amount < 0) {
            throw std::invalid_argument("Negative deposit amount");
        }
        co_await asio::post(strand_, asio::use_awaitable);
        balance_ += amount;
    }
    
    asio::awaitable<void> async_withdraw(int64_t amount) {
        if (amount < 0) {
            throw std::invalid_argument("Negative withdrawal amount");
        }
        co_await asio::post(strand_, asio::use_awaitable);
        if (balance_ < amount) {
            throw std::invalid_argument("Insufficient funds");
        }
        balance_ -= amount;
    }
    
    asio::awaitable<int64_t> async_get_balance() {
        co_await asio::post(strand_, asio::use_awaitable);
        co_return balance_;
    }
    
private:
    asio::strand<executor_type> strand_;
    int64_t balance_;
};

asio::awaitable<void> run_transactions(
    std::shared_ptr<BankAccount> account,
    int id,
    int num_transactions,
    int max_amount,
    std::atomic<int>& pending_count,
    std::atomic<int64_t>& expected_balance_delta
) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int64_t> dist(1, max_amount);
    std::uniform_int_distribution<int> type_dist(0, 1);
    
    for (int i = 0; i < num_transactions; ++i) {
        try {
            int64_t amount = dist(gen);
            bool is_deposit = (type_dist(gen) == 0);
            if (is_deposit) {
                co_await account->async_deposit(amount);
                expected_balance_delta += amount;
            } else {
                co_await account->async_withdraw(amount);
                expected_balance_delta -= amount;
            }
        } catch (const std::invalid_argument&) {
        }
    }
    pending_count--;
}

int main() {
    try {
        const int NUM_THREADS = 4;
        const int NUM_COROUTINES = 100;
        const int TRANSACTIONS_PER_CORO = 20;
        const int MAX_AMOUNT = 100;
        const int64_t INITIAL_BALANCE = 10000;
        
        asio::io_context io_context(NUM_THREADS);
        auto account = std::make_shared<BankAccount>(io_context, INITIAL_BALANCE);
        std::atomic<int> pending_count{NUM_COROUTINES};
        std::atomic<int64_t> expected_balance_delta{0};
        
        for (int i = 0; i < NUM_COROUTINES; ++i) {
            asio::co_spawn(
                io_context,
                run_transactions(account, i, TRANSACTIONS_PER_CORO, MAX_AMOUNT,
                                 pending_count, expected_balance_delta),
                asio::detached
            );
        }
        
        std::vector<std::thread> threads;
        for (int i = 0; i < NUM_THREADS; ++i) {
            threads.emplace_back([&io_context]() { io_context.run(); });
        }
        
        while (pending_count.load() > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        io_context.stop();
        for (auto& t : threads) t.join();
        
        int64_t final_balance = 0;
        asio::io_context temp_io;
        asio::co_spawn(temp_io, [&account, &final_balance]() -> asio::awaitable<void> {
            final_balance = co_await account->async_get_balance();
        }, asio::use_future).get();
        
        int64_t expected_balance = INITIAL_BALANCE + expected_balance_delta.load();
        
        std::cout << "Expected balance: " << expected_balance << std::endl;
        std::cout << "Final balance: " << final_balance << std::endl;
        
        if (expected_balance == final_balance) {
            std::cout << "SUCCESS: No data races" << std::endl;
        } else {
            std::cout << "FAILURE: Race condition detected" << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    
    return 0;
}