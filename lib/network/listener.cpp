#include "listener.hpp"

#include <boost/asio/strand.hpp>
#include <print>

#include "session.hpp"

//------------------------------------------------------------------------------

// Constructor implementation for the listener class
listener::listener(asio::io_context& ioc,
    tcp::endpoint endpoint,
    std::shared_ptr<std::string const> const& doc_root)
    : ioc_(ioc), acceptor_(asio::make_strand(ioc)), doc_root_(doc_root) {
  beast::error_code ec;

  // Open the acceptor
  boost::ignore_unused(acceptor_.open(endpoint.protocol(), ec));
  if (ec) {
    fail(ec, "open");
    return;
  }

  // Allow address reuse
  boost::ignore_unused(acceptor_.set_option(asio::socket_base::reuse_address(true), ec));
  if (ec) {
    fail(ec, "set_option");
    return;
  }

  // Bind to the server address
  boost::ignore_unused(acceptor_.bind(endpoint, ec));
  if (ec) {
    fail(ec, "bind");
    return;
  }
  std::println("[INFO] Waiting for connection...");
  // Start listening for connections
  boost::ignore_unused(acceptor_.listen(asio::socket_base::max_listen_connections, ec));

  if (ec) {
    fail(ec, "listen");
    return;
  }
}

// Start accepting incoming connections
void listener::run() { do_accept(); }

void listener::do_accept() {
  // The new connection gets its own strand
  acceptor_.async_accept(asio::make_strand(ioc_),
      beast::bind_front_handler(&listener::on_accept, shared_from_this()));
}

void listener::on_accept(beast::error_code ec, tcp::socket socket) {
  if (ec) {
    fail(ec, "accept");
    return;  // To avoid infinite loop
  } else {
    // Store the client IP address
    set_client_ip(get_socket_ip_and_port(socket)[0]);
    std::println("\n[INFO] Client connected. IP: {}, PORT: {}",
        get_socket_ip_and_port(socket)[0],
        get_socket_ip_and_port(socket)[1]);

    // Create the session and run it
    std::make_shared<session>(std::move(socket), doc_root_)->run();
  }

  // Accept another connection
  do_accept();
}

std::array<std::string, 2> listener::get_socket_ip_and_port(tcp::socket& socket) {
  // Get the remote endpoint
  auto remote_endpoint = socket.remote_endpoint();

  // Convert the IP address to a string
  std::string ip_address = remote_endpoint.address().to_string();
  std::string port = std::to_string(remote_endpoint.port());

  return {ip_address, port};
}

void listener::fail(beast::error_code ec, char const* what) {
  std::println("{}: {}", what, ec.message());
}