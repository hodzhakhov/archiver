#pragma once

#include <boost/asio.hpp>
#include <boost/beast.hpp>

namespace ip = boost::asio::ip;
namespace http = boost::beast::http;

void start_server(int port = 8080, int thread_count = 4);

void onReadAsync(std::shared_ptr<ip::tcp::socket> sock,
                 std::shared_ptr<boost::beast::flat_buffer> buf,
                 std::shared_ptr<http::request<http::string_body>> req,
                 boost::beast::error_code ec, std::size_t bytes_transferred);

void onWriteAsync(std::shared_ptr<ip::tcp::socket> sock,
                  std::shared_ptr<http::response<http::string_body>> resp,
                  bool keepAlive, boost::beast::error_code ec,
                  std::size_t bytes_transferred);

void onAcceptAsync(ip::tcp::acceptor &acceptor,
                   boost::asio::io_context &service,
                   std::shared_ptr<ip::tcp::socket> sock,
                   const boost::system::error_code &ec);
