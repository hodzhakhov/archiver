#include "server.h"
#include "request/request_handler.h"
#include <iostream>

void onReadAsync(
    std::shared_ptr<ip::tcp::socket> sock,
    std::shared_ptr<boost::beast::flat_buffer> buf,
    std::shared_ptr<http::request<http::string_body>> req,
    boost::beast::error_code ec,
    std::size_t) {
    if (ec) {
        if (ec != boost::asio::error::operation_aborted && ec != http::error::end_of_stream) {
            std::cerr << ec.message() << std::endl;
        }
        return;
    }

    auto resp = std::make_shared<http::response<http::string_body>>(handle_request(*req));
    http::async_write(*sock, *resp, std::bind(&onWriteAsync, sock, resp, req->keep_alive(),
                                             std::placeholders::_1, std::placeholders::_2));
}

void onWriteAsync(
    std::shared_ptr<ip::tcp::socket> sock,
    std::shared_ptr<http::response<http::string_body>>,
    bool keepAlive,
    boost::beast::error_code ec,
    std::size_t) {
    if (ec) {
        if (ec != boost::asio::error::operation_aborted) {
            std::cerr << ec.message() << std::endl;
        }
        return;
    }

    if (keepAlive) {
        auto buf = std::make_shared<boost::beast::flat_buffer>();
        buf->max_size(1024 * 1024 * 100);
        auto req = std::make_shared<http::request<http::string_body>>();
        http::async_read(*sock, *buf, *req, std::bind(&onReadAsync, sock, buf, req,
                                                     std::placeholders::_1, std::placeholders::_2));
    } else {
        sock->shutdown(ip::tcp::socket::shutdown_both);
    }
}

void onAcceptAsync(
    ip::tcp::acceptor& acceptor,
    boost::asio::io_context& service,
    std::shared_ptr<ip::tcp::socket> sock,
    const boost::system::error_code& ec) {
    if (ec) {
        if (ec != boost::asio::error::operation_aborted) {
            std::cerr << ec.message() << std::endl;
        }
        return;
    }

    auto buf = std::make_shared<boost::beast::flat_buffer>();
    buf->max_size(1024 * 1024 * 100);
    auto req = std::make_shared<http::request<http::string_body>>();
    http::async_read(*sock, *buf, *req, std::bind(&onReadAsync, sock, buf, req,
                                                 std::placeholders::_1, std::placeholders::_2));

    auto acceptSock = std::make_shared<ip::tcp::socket>(service);
    acceptor.async_accept(*acceptSock, std::bind(&onAcceptAsync, std::ref(acceptor),
                                                std::ref(service), acceptSock, std::placeholders::_1));
}

void start_server(int port, int thread_count) {
    boost::asio::io_context service;
    boost::asio::thread_pool tp(thread_count);

    for (int i = 0; i < thread_count; ++i) {
        post(tp, [&service]() {
            auto work = make_work_guard(service);
            service.run();
        });
    }

    ip::tcp::acceptor acceptor(service);
    ip::tcp::endpoint endpoint(ip::tcp::v4(), port);
    acceptor.open(endpoint.protocol());
    acceptor.set_option(ip::tcp::acceptor::reuse_address(true));
    acceptor.bind(endpoint);
    acceptor.listen();

    for(int i = 0; i < thread_count; ++i) {
        auto sock = std::make_shared<ip::tcp::socket>(service);
        acceptor.async_accept(*sock, std::bind(&onAcceptAsync, std::ref(acceptor),
                                              std::ref(service), sock, std::placeholders::_1));
    }

    tp.join();
}