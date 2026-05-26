#include <boost/asio.hpp>
#include <iostream>
#include <memory>
#include <thread>
#include <chrono>

using boost::asio::ip::tcp;

class DroneClient : public std::enable_shared_from_this<DroneClient> {
public:
    DroneClient(boost::asio::io_context& io_context,
                const tcp::resolver::results_type& endpoints)
        : socket_(io_context) {
        do_connect(endpoints);
    }

    void send_message(const std::string& message) {
        auto self = shared_from_this();
        boost::asio::async_write(
            socket_,
            boost::asio::buffer(message),
            [this, self, message](boost::system::error_code ec, std::size_t) {
                if (!ec) {
                    std::cout << "[БПЛА] Отправлено: " << message << std::endl;
                    do_read();
                }
            }
        );
    }

private:
    void do_connect(const tcp::resolver::results_type& endpoints) {
        auto self = shared_from_this();
        boost::asio::async_connect(
            socket_, endpoints,
            [this, self](boost::system::error_code ec, tcp::endpoint) {
                if (!ec) std::cout << "[БПЛА] Подключён к серверу!" << std::endl;
            }
        );
    }

    void do_read() {
        auto self = shared_from_this();
        socket_.async_read_some(
            boost::asio::buffer(data_, max_length),
            [this, self](boost::system::error_code ec, std::size_t length) {
                if (!ec) {
                    std::cout << "[БПЛА] Эхо-ответ: " << std::string(data_, length) << std::endl;
                }
            }
        );
    }

    tcp::socket socket_;
    enum { max_length = 1024 };
    char data_[max_length];
};

int main() {
    try {
        boost::asio::io_context io_context;
        tcp::resolver resolver(io_context);
        auto endpoints = resolver.resolve("127.0.0.1", "12345");

        auto client = std::make_shared<DroneClient>(io_context, endpoints);

        std::thread io_thread([&io_context]() { io_context.run(); });

        std::string input;
        std::cout << "[БПЛА] Введите координаты (или 'exit'):" << std::endl;
        while (true) {
            std::cout << "> ";
            std::getline(std::cin, input);
            if (input == "exit" || input.empty()) break;
            client->send_message(input);
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }

        io_context.stop();
        io_thread.join();
    } catch (std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << std::endl;
    }
    return 0;
}