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
    size_t invalid_fields;
    boost::json::object message_stats;
};


// Return a response for the given request.
template <class Body, class Allocator>
http::message_generator handle_request(beast::string_view doc_root, 
    http::request<Body, http::basic_fields<Allocator>>&& req, tcp::endpoint client_endpoint)
{
    beast::error_code ec;
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
        unsigned short client_port(client_endpoint.port());
        size_t total_number_of_fields = 0, invalid_fields = 0;
        ClientResponseData response_data;
        response_data.analysis_type = "LOG LEVEL";
        std::vector<boost::json::value> json_objects;
        std::string client_id(req["Client-Id"]);
        std::map<std::string, int, std::less<std::string>> message_frequencies;

        if(req.find(http::field::content_type) == req.end()) {
            return ResponseHandler::bad_request(req, "Missing Content-Type header");
        }
        if(req.body().size() == 0) {
            return ResponseHandler::bad_request(req, "Empty request body");
        }

        std::string content_type = req[http::field::content_type];
        if(content_type.find("multipart/form-data") != std::string::npos) {
            // Parse boundary
             std::string boundary;
             std::string boundary_prefix = "boundary=";
             auto pos = content_type.find(boundary_prefix);
             if (pos != std::string::npos) {
                 boundary = "--" + content_type.substr(pos + boundary_prefix.size());
             } else {
                 return ResponseHandler::bad_request(req, "Missing boundary in multipart/form-data");
             }
            std::string body = beast::buffers_to_string(req.body().data());
            size_t start = 0;
            while ((start = body.find(boundary, start)) != std::string::npos) {
                size_t part_start = start + boundary.size();
                if (body.substr(part_start, 2) == "--") break; // End of multipart
                part_start += 2; // skip \r\n
                size_t header_end = body.find("\r\n\r\n", part_start);
                if (header_end == std::string::npos) break;
                std::string headers = body.substr(part_start, header_end - part_start);
                size_t data_start = header_end + 4;
                size_t data_end = body.find(boundary, data_start);
                if (data_end == std::string::npos) break;
                size_t data_len = data_end - data_start;
                std::string part_data = body.substr(data_start, data_len);
                // Remove trailing CRLF
                if (!part_data.empty() && part_data[part_data.size()-1] == '\n') part_data.pop_back();
                if (!part_data.empty() && part_data[part_data.size()-1] == '\r') part_data.pop_back();

                // Parse headers
                std::string disposition, filename, part_content_type;
                std::istringstream hstream(headers);
                std::string hline;
                while (std::getline(hstream, hline)) {
                    if (hline.find("Content-Disposition:") != std::string::npos) {
                        disposition = hline;
                        auto fname_pos = hline.find("filename=\"");
                        if (fname_pos != std::string::npos) {
                            size_t fname_start = fname_pos + 10;
                            size_t fname_end = hline.find('"', fname_start);
                            filename = hline.substr(fname_start, fname_end - fname_start);
                        }
                    } else if (hline.find("Content-Type:") != std::string::npos) {
                        auto ctype_pos = hline.find(":");
                        if (ctype_pos != std::string::npos) {
                            part_content_type = hline.substr(ctype_pos + 1);
                            part_content_type.erase(0, part_content_type.find_first_not_of(" \t\r\n"));
                        }
                    }
                }
                // Save and process each part
                if (part_content_type == "application/json") {
                    std::cout << "[INFO] Receiving and parsing JSON file: " << filename << std::endl;
                    try {
                        auto response = save_file(part_data, client_id, client_ip_address, ".json");
                    } catch(const std::exception& e) {
                        std::cerr << "[ERROR] Saving file: " << e.what() << '\n';
                        return ResponseHandler::server_error(req, e.what());
                    }
                    const computed_data& parsedJson = process_json_request(part_data);
                    if(parsedJson.error_message != "success") {
                        return ResponseHandler::bad_request(req, parsedJson.error_message);
                    } else {
                        total_number_of_fields += parsedJson.total_fields;
                        invalid_fields += parsedJson.invalid_fields;
                        json_objects.push_back(parsedJson.message_stats);
                    }
                } else if (part_content_type == "application/xml") {
                    std::cout << "[INFO] Receiving and parsing XML file: " << filename << std::endl;
                    try {
                        auto response = save_file(part_data, client_id, client_ip_address, ".xml");
                    } catch(const std::exception& e) {
                        std::cerr << "[ERROR] Saving file: " << e.what() << '\n';
                        return ResponseHandler::server_error(req, e.what());
                    }
                    const computed_data& parsedXml = parse_xml_file(part_data);
                    if(parsedXml.error_message != "success") {
                        return ResponseHandler::bad_request(req, parsedXml.error_message);
                    } else {
                        total_number_of_fields += parsedXml.total_fields;
                        invalid_fields += parsedXml.invalid_fields;
                        json_objects.push_back(parsedXml.message_stats);
                    }
                } else if (part_content_type == "text/plain") {
                    std::cout << "[INFO] Receiving and parsing text file: " << filename << std::endl;
                    try {
                        auto response = save_file(part_data, client_id, client_ip_address, ".txt");
                    } catch(const std::exception& e) {
                        std::cerr << "[ERROR] Saving file: " << e.what() << '\n';
                        return ResponseHandler::server_error(req, e.what());
                    }
                    const computed_data& parsedText = parse_text_file(part_data);
                    if(parsedText.error_message != "success") {
                        return ResponseHandler::bad_request(req, parsedText.error_message);
                    } else {
                        total_number_of_fields += parsedText.total_fields;
                        invalid_fields += parsedText.invalid_fields;
                        json_objects.push_back(parsedText.message_stats);
                    }
                }
                start = data_end;
            }
            std::cout << "[INFO] Making analysis and preparing a response..." << std::endl;
            response_data.client_ip = client_ip_address;
            response_data.client_port = std::to_string(client_port);
            response_data.message_stats = merge_json_objects(json_objects);
            response_data.total_number_of_fields = total_number_of_fields;
            response_data.invalid_fields = invalid_fields;
            std::cout << "[INFO] Response sent to client. ID: "<< client_id << "\n" << std::endl;
            return ResponseHandler::response(req, response_data);
        }

        if(!is_valid_content_type(content_type)) {
            return ResponseHandler::bad_request(req, "Invalid Content-Type header");
        }

        if(content_type == "application/json") {

            LOG("[INFO] Receiving and Parsing Json data");

            if(!req.count("Client-Id"))
                return ResponseHandler::bad_request(req, "Missing Client-Id header");
            
            std::string data = beast::buffers_to_string(req.body().data());

            try{
                auto response = save_file(data, client_id, client_ip_address, ".json");
            }catch(const std::exception& e){
                std::cerr << "[ERROR] Saving file: " << e.what() << '\n';
                return ResponseHandler::server_error(req, e.what());
            }
            const computed_data& parsedJson = process_json_request(data);
            
            if(parsedJson.error_message != "success") {
                return ResponseHandler::bad_request(req, parsedJson.error_message);
            }else{
                total_number_of_fields += parsedJson.total_fields;
                invalid_fields += parsedJson.invalid_fields;
                json_objects.push_back(parsedJson.message_stats);
            }
        }

        if(content_type == "text/plain"){

            LOG("[INFO] Receiving Text data");

            if(!req.count("Client-Id"))
                return ResponseHandler::bad_request(req, "Missing Client-Id header");
            
            std::string data = beast::buffers_to_string(req.body().data());

            try{
                auto response = save_file(data, client_id, client_ip_address, ".txt");
            }catch(const std::exception& e){
                std::cerr << "[ERROR] Saving file: " << e.what() << '\n';
                return ResponseHandler::server_error(req, e.what());
            }
            const computed_data& parsedText = parse_text_file(data);
            
            if(parsedText.error_message != "success") {
                return ResponseHandler::bad_request(req, parsedText.error_message);
            }else{
                total_number_of_fields += parsedText.total_fields;
                invalid_fields += parsedText.invalid_fields;
                json_objects.push_back(parsedText.message_stats);
            }
        }
        
        if(content_type == "application/xml"){
            LOG("[INFO] Receiving XML data");

            if(!req.count("Client-Id"))
                return ResponseHandler::bad_request(req, "Missing Client-Id header");
            
            std::string data = beast::buffers_to_string(req.body().data());

            try{
                auto response = save_file(data, client_id, client_ip_address, ".xml");
            }catch(const std::exception& e){
                std::cerr << "[ERROR] Saving file: " << e.what() << '\n';
                return ResponseHandler::server_error(req, e.what());
            }
            const computed_data& parsedXml = parse_xml_file(data);
            
            if(parsedXml.error_message != "success") {
                return ResponseHandler::bad_request(req, parsedXml.error_message);
            }else{
                total_number_of_fields += parsedXml.total_fields;
                invalid_fields += parsedXml.invalid_fields;
                json_objects.push_back(parsedXml.message_stats);
            }
        }
        std::cout << "[INFO] Making analysis and preparing a response..." << std::endl;
        response_data.client_ip = client_ip_address;
        response_data.client_port = std::to_string(client_port);
        response_data.message_stats = merge_json_objects(json_objects);
        response_data.total_number_of_fields = total_number_of_fields;
        response_data.invalid_fields = invalid_fields;
        LOG("[INFO] Response sent to client. ID: "<< client_id << "\n" << std::endl);
        return ResponseHandler::response(req, response_data);
    }

    // Build the path to the requested file
    std::string path = path_cat(doc_root, req.target());
    if (req.target().back() == '/'){
        path.append("index.html");
    }

    // Attempt to open the file
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
