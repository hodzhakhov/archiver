#pragma once

#include <boost/beast.hpp>
#include <string>

namespace http = boost::beast::http;

std::string extract_boundary(const std::string &content_type);
std::string generate_boundary();
http::response<http::string_body> handle_request(const http::request<http::string_body> &req);
