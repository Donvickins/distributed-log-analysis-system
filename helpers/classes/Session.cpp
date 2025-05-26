#include "Session.hpp"
#include "../Helper.hpp"
#include "../http_handler.hpp"

// Take ownership of the stream
session::session(tcp::socket &&socket, std::shared_ptr<std::string const> const &doc_root)
    : stream_(std::move(socket)), doc_root_(doc_root)
{
}

// Start the asynchronous operation
void session::run()
{
    // We need to be executing within a strand to perform async operations
    // on the I/O objects in this session. Although not strictly necessary
    // for single-threaded contexts, this example code is written to be
    // thread-safe by default.
    asio::dispatch(stream_.get_executor(), beast::bind_front_handler(&session::do_read, shared_from_this()));
}

void session::do_read()
{
    // Make the request empty before reading,
    // otherwise the operation behavior is undefined.
    req_ = {};

    // Set the timeout dynamically based on the size of the data being transferred.
    stream_.expires_after(std::chrono::seconds(300));

    // Use a parser to set the body limit
    auto parser = std::make_shared<http::request_parser<http::dynamic_body>>();
    parser->body_limit(body_limit_); // Set the body limit to 1GB

    // Read a request
    http::async_read(
        stream_,
        buffer_,
        *parser,
        beast::bind_front_handler(
            [self = shared_from_this(), parser](beast::error_code ec, std::size_t bytes_transferred) {
                self->on_read_with_parser(ec, bytes_transferred, parser);
            }
        )
    );
}

// New handler to accept parser
void session::on_read_with_parser(beast::error_code ec, std::size_t bytes_transferred, std::shared_ptr<http::request_parser<http::dynamic_body>> parser)
{
    boost::ignore_unused(bytes_transferred);

    // This means they closed the connection
    if (ec == http::error::end_of_stream)
        return do_close();

    if(ec == beast::error::timeout)
        return do_close();

    if (ec)
        return fail(ec, "[INFO] Read");

    req_ = parser->get(); // Move the parsed request into req_

    //Get client endpoint from the socket
    tcp::endpoint client_endpoint = stream_.socket().remote_endpoint();
    // Send the response
    send_response(handle_request(*doc_root_, std::move(req_), client_endpoint));
}

void session::send_response(http::message_generator &&msg)
{
    bool keep_alive = msg.keep_alive();

    // Write the response
    beast::async_write(
        stream_,
        std::move(msg),
        beast::bind_front_handler(
            &session::on_write, shared_from_this(), keep_alive));
}

void session::on_write(bool keep_alive, beast::error_code ec, std::size_t bytes_transferred)
{
    boost::ignore_unused(bytes_transferred);

    if (ec){
        LOG("[ERROR] Write failed: " << ec.message());
        return fail(ec, "[ERROR] Write");
    }
    if (!keep_alive)
    {
        // This means we should close the connection, usually because
        // the response indicated the "Connection: close" semantic.
        return do_close();
    }
    buffer_.consume(buffer_.size());
    // Read another request
    do_read();
}

void session::do_close()
{
    // Send a TCP shutdown
    beast::error_code ec;
    stream_.socket().shutdown(tcp::socket::shutdown_send, ec);

    // At this point the connection is closed gracefully
}
