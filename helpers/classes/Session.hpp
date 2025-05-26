#pragma once

// Include standard headers first
#include <memory>
#include <string>

// Then include boost headers via our wrapper
#include "../boost.hpp"

class session : public std::enable_shared_from_this<session>
{
private:
    beast::tcp_stream stream_;
    beast::flat_buffer buffer_;
    std::shared_ptr<std::string const> doc_root_;
    http::request<http::dynamic_body> req_; 
    std::size_t body_limit_ = 1073741824;

public:
    session(tcp::socket &&socket, std::shared_ptr<std::string const> const &doc_root);
    void run();
    void do_read();
    void on_read_with_parser(beast::error_code ec, std::size_t bytes_transferred, std::shared_ptr<http::request_parser<http::dynamic_body>> parser);
    void send_response(http::message_generator &&msg);
    void on_write(bool keep_alive, beast::error_code ec, std::size_t bytes_transferred);
    void do_close();
};
