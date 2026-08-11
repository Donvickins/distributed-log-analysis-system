#pragma once

#include <array>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <memory>
#include <string>

namespace asio = boost::asio;
using tcp = boost::asio::ip::tcp;
namespace beast = boost::beast;

class listener : public std::enable_shared_from_this<listener> {
  asio::io_context& ioc_;
  tcp::acceptor acceptor_;
  std::shared_ptr<std::string const> doc_root_;
  std::string client_ip_;  // Add this line to store the client IP

 public:
  listener(asio::io_context& ioc,
      tcp::endpoint endpoint,
      std::shared_ptr<std::string const> const& doc_root);

  void run();
  std::string get_client_ip() const { return client_ip_; }        // Updated method
  void set_client_ip(const std::string& ip) { client_ip_ = ip; }  // New method
  std::array<std::string, 2> get_socket_ip_and_port(
      tcp::socket& socket);  // Renamed original method

 private:
  void do_accept();
  void on_accept(beast::error_code ec, tcp::socket socket);
  void fail(beast::error_code ec, char const* what);
};
