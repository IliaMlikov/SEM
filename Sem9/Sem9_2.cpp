#include <boost/asio.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <iostream>
#include <array>

namespace asio = boost::asio;
using asio::ip::tcp;
using namespace asio::experimental::awaitable_operators;

asio::awaitable<std::pair<std::string, std::string>> read_from(tcp::socket& socket, std::string name) {
    std::array<char, 1024> buffer;
    try {
        auto [ec, n] = co_await socket.async_read_some(
            asio::buffer(buffer),
            asio::as_tuple(asio::use_awaitable)
        );
        if (ec == asio::error::eof) {
            co_return {name, "[DISCONNECTED]"};
        }
        if (ec) {
            throw boost::system::system_error(ec);
        }
        co_return {name, std::string(buffer.data(), n)};
    } catch (const std::exception& e) {
        co_return {name, "[ERROR: " + std::string(e.what()) + "]"};
    }
}

asio::awaitable<void> send_data(tcp::socket& socket, const std::string& message) {
    co_await asio::async_write(socket, asio::buffer(message), asio::use_awaitable);
}

asio::awaitable<void> multiplexer(tcp::socket sock1, tcp::socket sock2) {
    for (int i = 0; i < 5; ++i) {
        auto result = co_await (read_from(sock1, "SOCK1") || read_from(sock2, "SOCK2"));
        std::visit([](auto& val) {
            auto& [source, data] = val;
            std::cout << "[" << source << "] " << data << std::endl;
        }, result);
    }
}

asio::awaitable<void> test_echo_server(asio::io_context& io, short port) {
    tcp::acceptor acceptor(io, tcp::endpoint(tcp::v4(), port));
    while (true) {
        tcp::socket socket = co_await acceptor.async_accept(asio::use_awaitable);
        asio::co_spawn(io, [sock = std::move(socket)]() -> asio::awaitable<void> {
            std::array<char, 1024> buf;
            try {
                for (;;) {
                    auto [ec, n] = co_await sock.async_read_some(
                        asio::buffer(buf),
                        asio::as_tuple(asio::use_awaitable)
                    );
                    if (ec) break;
                    co_await asio::async_write(sock, asio::buffer(buf.data(), n), asio::use_awaitable);
                }
            } catch (...) {}
        }, asio::detached);
    }
}

int main() {
    try {
        asio::io_context io_context;
        asio::co_spawn(io_context, test_echo_server(io_context, 12346), asio::detached);
        asio::co_spawn(io_context, test_echo_server(io_context, 12347), asio::detached);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        tcp::socket sock1(io_context);
        tcp::socket sock2(io_context);
        tcp::resolver resolver(io_context);
        auto endpoints1 = resolver.resolve("127.0.0.1", "12346");
        auto endpoints2 = resolver.resolve("127.0.0.1", "12347");
        asio::connect(sock1, endpoints1);
        asio::connect(sock2, endpoints2);
        std::thread sender([&io_context, &sock1, &sock2]() {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            asio::post(io_context, [&]() {
                asio::co_spawn(io_context, send_data(sock1, "Hello from client 1!"), asio::detached);
                asio::co_spawn(io_context, send_data(sock2, "Hello from client 2!"), asio::detached);
            });
        });
        sender.detach();
        asio::co_spawn(io_context, multiplexer(std::move(sock1), std::move(sock2)), asio::detached);
        io_context.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return 0;
}