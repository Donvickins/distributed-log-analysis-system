#pragma once

#include <string>
#include <sstream>
#include <istream>

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
    size_t total_fields;
    size_t invalid_fields;
    json message_stats = json::object();
    std::string error_message;
};

computed_data process_json_request(std::string& body)
{
    computed_data response_data;
    try
    {
        auto json_data = json::parse(body);
        std::map<std::string, std::map<std::string, int>> parsedData;
        size_t valid_objects = 0, invalid_objects = 0;

        if(!json_data.is_array()) {
            throw std::runtime_error("Invalid JSON format: Expected an array");
        }

        for(const auto& [key, value] : json_data.items()){

            if(!value.is_object() || !value.contains("message") || !value.contains("log_level")) {
                invalid_objects++;
                continue;
            }

            valid_objects++;
            
            parsedData[value["log_level"]][value["message"]]++;

        }

        response_data.total_fields = valid_objects + invalid_objects;
        response_data.invalid_fields = invalid_objects;
        response_data.message_stats = json(parsedData);
        response_data.error_message = "success";
    }
    catch(const std::exception& e)
    {
        std::cerr << "[ERROR] Processing Json: " << e.what() << '\n';
        response_data.total_fields = 0;
        response_data.invalid_fields = 0;
        response_data.message_stats = json::object();
        response_data.error_message = e.what();
        return response_data;
    }
    
    return response_data;
}

computed_data parse_text_file(const std::string& body)
{
    computed_data response_data;

    std::unordered_map<std::string, std::unordered_map<std::string, int>> parsedData;
    size_t valid_objects = 0, invalid_objects = 0;

    std::stringstream iss(body);
    std::string line;
    try{
    while (std::getline(iss, line)) {
        std::stringstream iss_line(line);
        size_t field = 0;
        std::string line_data,log_level, message;

        while(std::getline(iss_line, line_data, '|')){
            ++field;
            if(field == 2){
                log_level = trim(line_data);
            }
            if(field == 3){
                message = trim(line_data);
                break;
            }
        }

        if(log_level.empty() || message.empty()) {
            invalid_objects++;
            continue;
        }
        parsedData[log_level][message]++;
        ++valid_objects;

    }}catch(std::exception& e){
        std::cerr << "[ERROR] Processing Text: " << e.what() << '\n';
        response_data.total_fields = 0;
        response_data.invalid_fields = 0;
        response_data.message_stats = json::object();
        response_data.error_message = e.what();
        return response_data;
    }

    response_data.total_fields = valid_objects + invalid_objects;
    response_data.invalid_fields = invalid_objects;
    response_data.message_stats = json(parsedData);
    response_data.error_message = "success";

    return response_data;
}

json merge_json_objects(const std::vector<json>& json_objects) {
    std::unordered_map<std::string, std::unordered_map<std::string, int>> merged_data;

    for (const auto& obj : json_objects) {
        if(!obj.is_object())
            throw std::runtime_error("Invalid JSON format: Expected an object");

        for (const auto& [key, value] : obj.items()) {
            if(!value.is_object())
                throw std::runtime_error("Invalid JSON format: Expected an object");
            
            for(const auto& [message, count] : value.items()) {
                merged_data[key][message] += count.get<int>();
            }
        }
    }

    return json(merged_data);
}