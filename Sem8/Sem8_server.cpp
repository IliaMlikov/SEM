#include <boost/asio.hpp>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>
#include <atomic>

using boost::asio::ip::tcp;

std::atomic<int> global_message_count{0};

class Session : public std::enable_shared_from_this<Session> {
public:
    Session(tcp::socket socket,
            std::shared_ptr<boost::asio::strand<boost::asio::io_context::executor_type>> strand)
        : socket_(std::move(socket)), strand_(strand) {}

    void start() { do_read(); }

private:
    void do_read() {
        auto self = shared_from_this();
        socket_.async_read_some(
            boost::asio::buffer(data_, max_length),
            boost::asio::bind_executor(*strand_,
                [this, self](boost::system::error_code ec, std::size_t length) {
                    if (!ec) {
                        std::cout << "[СЕРВЕР] Получено: " << std::string(data_, length) << std::endl;
                        boost::asio::post(*strand_, [this]() {
                            ++global_message_count;
                            std::cout << "[СЕРВЕР] Всего сообщений: " 
                                      << global_message_count.load() << std::endl;
                        });
                        do_write(length);
                    }
                })
        );
    }

    void do_write(std::size_t length) {
        auto self = shared_from_this();
        boost::asio::async_write(
            socket_,
            boost::asio::buffer(data_, length),
            boost::asio::bind_executor(*strand_,
                [this, self](boost::system::error_code ec, std::size_t) {
                    if (!ec) do_read();
                })
        );
    }

    tcp::socket socket_;
    std::shared_ptr<boost::asio::strand<boost::asio::io_context::executor_type>> strand_;
    enum { max_length = 1024 };
    char data_[max_length];
};

class Server {
public:
    Server(boost::asio::io_context& io_context, short port)
        : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)),
          strand_(std::make_shared<boost::asio::strand<boost::asio::io_context::executor_type>>(
              io_context.get_executor())) {
        std::cout << "[СЕРВЕР] Запущен на порту " << port << std::endl;
        do_accept();
    }

private:
    void do_accept() {
        acceptor_.async_accept(
            [this](boost::system::error_code ec, tcp::socket socket) {
                if (!ec) {
                    std::cout << "[СЕРВЕР] Беспилотник подключился!" << std::endl;
                    std::make_shared<Session>(std::move(socket), strand_)->start();
                }
                do_accept();
            }
        );
    }

    tcp::acceptor acceptor_;
    std::shared_ptr<boost::asio::strand<boost::asio::io_context::executor_type>> strand_;
};

int main() {
    try {
        boost::asio::io_context io_context;
        const short PORT = 12345;
        const int NUM_THREADS = 4;

        Server server(io_context, PORT);

        std::vector<std::thread> threads;
        for (int i = 0; i < NUM_THREADS; ++i) {
            threads.emplace_back([&io_context]() { io_context.run(); });
        }

        for (auto& t : threads) t.join();
    } catch (std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << std::endl;
    }
    return 0;
}