#include <boost/asio.hpp>
#include <iostream>
#include <array>

namespace asio = boost::asio;
using asio::ip::tcp;

asio::awaitable<void> echo_session(tcp::socket sock) {
    char data[1024];
    try {
        for (;;) {
            size_t length = co_await sock.async_read_some(
                asio::buffer(data),
                asio::use_awaitable
            );
            co_await asio::async_write(
                sock,
                asio::buffer(data, length),
                asio::use_awaitable
            );
        }
    } catch (const std::exception& e) {
        std::cerr << "Session error: " << e.what() << std::endl;
    }
}

asio::awaitable<void> accept_connections(asio::io_context& io_context, short port) {
    tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), port));
    while (true) {
        tcp::socket socket = co_await acceptor.async_accept(asio::use_awaitable);
        asio::co_spawn(io_context, echo_session(std::move(socket)), asio::detached);
    }
}

int main() {
    try {
        asio::io_context io_context;
        asio::co_spawn(io_context, accept_connections(io_context, 8080), asio::detached);
        io_context.run();
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
    return 0;
}