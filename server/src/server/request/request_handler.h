#pragma once

#include <boost/beast.hpp>

namespace http = boost::beast::http;

http::response<http::string_body> handle_request(const http::request<http::string_body>& req);
