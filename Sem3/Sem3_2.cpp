#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>

template<typename T>
class Account {
private:
    T balance;
    std::mutex mtx;
    
public:
    Account(T initial_balance = 0) : balance(initial_balance) {}
    
    T get_balance() {
        std::lock_guard<std::mutex> lock(mtx);
        return balance;
    }
    
    void deposit(T amount) {
        std::lock_guard<std::mutex> lock(mtx);
        balance += amount;
    }
    
    void withdraw(T amount) {
        std::lock_guard<std::mutex> lock(mtx);
        balance -= amount;
    }
    
    std::mutex& get_mutex() {
        return mtx;
    }
};

template<typename T>
class Bank {
private:
    std::vector<Account<T>> accounts;
    std::condition_variable cv;
    
    inline void transfer_impl(int from, int to, T amount) {
        if (from == to) return;
        
        int first = from;
        int second = to;
        if (first > second) {
            std::swap(first, second);
        }
        
        std::unique_lock<std::mutex> lock1(accounts[first].get_mutex(), std::defer_lock);
        std::unique_lock<std::mutex> lock2(accounts[second].get_mutex(), std::defer_lock);
        std::lock(lock1, lock2);
        
        cv.wait(lock1, [this, from, amount]() {
            return accounts[from].get_balance() >= amount;
        });
        
        accounts[from].withdraw(amount);
        accounts[to].deposit(amount);
        
        cv.notify_all();
    }
    
public:
    Bank(int num_accounts, T initial_balance = 0) {
        for (int i = 0; i < num_accounts; ++i) {
            accounts.emplace_back(initial_balance);
        }
    }
    
    void transfer(int from, int to, T amount) {
        transfer_impl(from, to, amount);
    }
    
    T total_balance() {
        T total = T(0);
        for (auto& account : accounts) {
            total += account.get_balance();
        }
        return total;
    }
    
    size_t size() const {
        return accounts.size();
    }
};