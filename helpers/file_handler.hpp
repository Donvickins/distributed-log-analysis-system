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
#include <boost/asio/buffer.hpp>

#include "Helper.hpp"
#include "classes/ResponseHandler.hpp"
#include <pugixml.hpp>
#include "classes/simdjson.h"

// Namespace declarations
namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace fs = boost::filesystem;

using tcp = boost::asio::ip::tcp;

struct computed_data {
    size_t total_fields;
    size_t invalid_fields;
    boost::json::object message_stats;
    std::string error_message;
};

computed_data process_json_request(const std::string& body)
{
    computed_data response_data;
    simdjson::dom::parser parser;
    try
    {
        simdjson::dom::element logs_array = parser.parse(body);

        std::unordered_map<std::string, std::map<std::string, int>> parsedData;
        size_t valid_objects = 0, invalid_objects = 0;

        if(!logs_array.is_array()) {
            throw std::runtime_error("Invalid JSON format: Expected an array");
        }

        for(const auto& log : logs_array){

            if(!log.is_object() || log["message"].is_null() || log["log_level"].is_null()) {
                invalid_objects++;
                continue;
            }

            valid_objects++;
            std::string_view log_level = log["log_level"].get_string();
            std::string_view message = log["message"].get_string();

            std::string log_level_str(log_level);
            std::string message_str(message);
            
            parsedData[log_level_str][message_str]++;

        }
        boost::json::value jv = boost::json::value_from(parsedData);
        
        if(jv.is_object()){
            response_data.total_fields = valid_objects + invalid_objects;
            response_data.invalid_fields = invalid_objects;
            response_data.message_stats = jv.as_object();
            response_data.error_message = "success";
        }else{
            throw std::runtime_error("Parsed JSON is not an object");
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << "[ERROR] Processing Json: " << e.what() << '\n';
        response_data.total_fields = 0;
        response_data.invalid_fields = 0;
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

        }

        boost::json::value jv = boost::json::value_from(parsedData);

        if(jv.is_object()){
            response_data.total_fields = valid_objects + invalid_objects;
            response_data.invalid_fields = invalid_objects;
            response_data.message_stats = jv.as_object();
            response_data.error_message = "success";
        }else{
            throw std::runtime_error("Parsed JSON is not an object");
        }
    }catch(std::exception& e){
        std::cerr << "[ERROR] Processing Text: " << e.what() << '\n';
        response_data.total_fields = 0;
        response_data.invalid_fields = 0;
        response_data.error_message = e.what();
        return response_data;
    }

    return response_data;
}

computed_data parse_xml_file(const std::string& body)
{
    computed_data response_data;

    std::unordered_map<std::string, std::unordered_map<std::string, int>> parsedData;
    size_t valid_objects = 0, invalid_objects = 0;

    try {
        pugi::xml_document doc;
        doc.load_string(body.c_str());

        pugi::xml_node logs = doc.child("logs");

        for(const auto& log: logs){
            const std::string currentLog = trim(log.child("log_level").text().as_string());
            const std::string currentLogMsg = trim(log.child("message").text().as_string());
            if(!is_log_level(currentLog) || currentLogMsg.empty() ){
                ++invalid_objects;
                continue;
            }
            
            parsedData[currentLog][currentLogMsg]++;
            ++valid_objects;
        }
        boost::json::value jv = boost::json::value_from(parsedData);

        if(jv.is_object()){
            response_data.total_fields = valid_objects + invalid_objects;
            response_data.invalid_fields = invalid_objects;
            response_data.message_stats = jv.as_object();
            response_data.error_message = "success";
        }else{
            throw std::runtime_error("Parsed JSON is not an object");
        }
    
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Processing XML: " << e.what() << '\n';
        response_data.total_fields = 0;
        response_data.invalid_fields = 0;
        response_data.error_message = e.what();
        return response_data;
    }

    return response_data;
}

boost::json::object merge_json_objects(const std::vector<boost::json::value>& json_array) {
    std::unordered_map<std::string, std::unordered_map<std::string, int>> merged_data;

    for (const auto& obj : json_array) {
        if(!obj.is_object())
            throw std::runtime_error("Invalid JSON format: Expected an object");

        for (const auto& [key, value] : obj.as_object()) {
            if(!value.is_object())
                throw std::runtime_error("Invalid JSON format: Expected an object");
            
            for(const auto& [message, count] : value.as_object()) {
                merged_data[key][message] += static_cast<int>(count.as_int64());
            }
        }
    }

    return boost::json::value_from(merged_data).as_object();
}