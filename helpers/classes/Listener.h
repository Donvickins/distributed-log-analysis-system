#pragma once

#include "../boost.hpp"
#include <memory>
#include <string>

class listener : public std::enable_shared_from_this<listener>
{
    asio::io_context &ioc_;
    tcp::acceptor acceptor_;
    std::shared_ptr<std::string const> doc_root_;

public:
    listener(asio::io_context &ioc, tcp::endpoint endpoint, std::shared_ptr<std::string const> const &doc_root);

    void run();

private:
    void do_accept();

    void on_accept(beast::error_code ec, tcp::socket socket);
};
