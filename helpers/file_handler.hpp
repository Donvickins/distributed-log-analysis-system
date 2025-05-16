#pragma once

// Boost includes - keep these after standard library
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/filesystem.hpp>

#include "Helper.hpp"
#include "classes/ResponseHandler.hpp"

// Namespace declarations
namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace fs = boost::filesystem;

using tcp = boost::asio::ip::tcp;

struct computed_data {
    size_t total_number_of_fields;
    size_t invalid_json_objects;
    json message_stats = json::object();
    std::string error_message;
};

computed_data process_json_request(std::string& body)
{
    computed_data response_data;
    try
    {
        auto json_data = json::parse(body);
        std::map<std::string, int> message_frequencies;
        size_t valid_objects = 0, invalid_objects = 0;

        if(!json_data.is_array()) {
            throw std::runtime_error("Invalid JSON format: Expected an array");
        }

        for(const auto& [key, value] : json_data.items()){

            if(!value.is_object() || !value.contains("message")) {
                invalid_objects++;
                continue;
            }

            valid_objects++;
            message_frequencies[value["message"]]++;

        }

        response_data.total_number_of_fields = valid_objects + invalid_objects;
        response_data.invalid_json_objects = invalid_objects;
        response_data.message_stats = json(message_frequencies);
        response_data.error_message = "success";
    }
    catch(const std::exception& e)
    {
        std::cerr << "[ERROR] Processing Json: " << e.what() << '\n';
        response_data.total_number_of_fields = 0;
        response_data.invalid_json_objects = 0;
        response_data.message_stats = json::object();
        response_data.error_message = e.what();
        return response_data;
    }
    
    return response_data;
}