#pragma once

// Core C++ standard library includes - keep these first
#include <string>
#include <memory>
#include <vector>
#include <map>
#include <utility>
#include <functional>
#include <algorithm>
#include <cstddef>
#include <type_traits>
#include <iostream>


// Local project includes
#include "file_handler.hpp"
#include "Helper.hpp"

#include "classes/ResponseHandler.hpp"

struct ClientResponseData {
    size_t total_number_of_fields;
    std::string client_ip;
    std::string client_port;
    std::string analysis_type;
    size_t invalid_json_objects;
    json message_stats = json::object();
};


// Return a response for the given request.
template <class Body, class Allocator>
http::message_generator handle_request(beast::string_view doc_root, 
    http::request<Body, http::basic_fields<Allocator>>&& req, tcp::endpoint client_endpoint)
{
    // Make sure we can handle the method
    if (req.method() != http::verb::get && req.method() != http::verb::post &&
        req.method() != http::verb::head)
        return ResponseHandler::bad_request(req, "Unknown HTTP-method");

    // Request path must be absolute and not contain "..".
    if (req.target().empty() ||
        req.target()[0] != '/' ||
        req.target().find("..") != beast::string_view::npos)
        return ResponseHandler::bad_request(req, "Illegal request-target");

    // Handle POST request first
    if(req.target() == "/" && req.method() == http::verb::post) {

        std::string client_ip_address(client_endpoint.address().to_string());
        unsigned short client_port = client_endpoint.port();
        ClientResponseData response_data;
        //size_t valid_json_objects = 0, invalid_json_objects = 0;
        auto content_type = req[http::field::content_type];
        response_data.analysis_type = "LOG LEVEL";

        std::map<std::string, int, std::less<std::string>> message_frequencies;

        if(req.find(http::field::content_type) == req.end()) {
            return ResponseHandler::bad_request(req, "Missing Content-Type header");
        }

        if(req.body().empty()) {
            return ResponseHandler::bad_request(req, "Empty request body");
        }

        if(!is_valid_content_type(content_type)) {
            return ResponseHandler::bad_request(req, "Invalid Content-Type header");
        }
        
        if(content_type == "application/json") {
            const computed_data& parsedJson = process_json_request(req.body());
            
            if(parsedJson.error_message != "success") {
                return ResponseHandler::bad_request(req, parsedJson.error_message);
            }else{
                response_data.total_number_of_fields = parsedJson.total_number_of_fields;
                response_data.invalid_json_objects = parsedJson.invalid_json_objects;
                response_data.message_stats = parsedJson.message_stats;
            }
        }
        
        //response_data.total_number_of_fields = invalid_json_objects + valid_json_objects;
        //response_data.invalid_json_objects = num_of_invalid_json_objects;
        response_data.client_ip = client_ip_address;
        response_data.client_port = std::to_string(client_port);

        return ResponseHandler::response(req, response_data);
    }

    // Build the path to the requested file
    std::string path = path_cat(doc_root, req.target());
    if (req.target().back() == '/'){
        path.append("index.html");
    }

    // Attempt to open the file
    beast::error_code ec;
    http::file_body::value_type body;
    body.open(path.c_str(), beast::file_mode::scan, ec);

    // Handle the case where the file doesn't exist
    if (ec == beast::errc::no_such_file_or_directory)
        return ResponseHandler::not_found(req, req.target());

    // Handle an unknown error
    if (ec)
        return ResponseHandler::server_error(req, ec.message());

    // Cache the size since we need it after the move
    auto const size = body.size();

    // Respond to HEAD request
    if (req.method() == http::verb::head)
    {
        http::response<http::empty_body> res{http::status::ok, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, mime_type(path));
        res.content_length(size);
        res.keep_alive(req.keep_alive());
        return res;
    }

    // Respond to GET request
    http::response<http::file_body> res{
        std::piecewise_construct,
        std::make_tuple(std::move(body)),
        std::make_tuple(http::status::ok, req.version())};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, mime_type(path));
    res.content_length(size);
    res.keep_alive(req.keep_alive());
    return res;
}
