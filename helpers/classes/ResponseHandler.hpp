#pragma once

#include <string>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <nlohmann/json.hpp>

namespace http = boost::beast::http;
namespace beast = boost::beast;
using json = nlohmann::json;

class ResponseHandler {
public:
    template<class Request>
    static http::response<http::string_body> bad_request(const Request& req, beast::string_view why) {
        http::response<http::string_body> res{http::status::bad_request, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = std::string(why);
        res.prepare_payload();
        return res;
    }

    template<class Request>
    static http::response<http::string_body> not_found(const Request& req, beast::string_view target) {
        http::response<http::string_body> res{http::status::not_found, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = "The resource '" + std::string(target) + "' was not found.";
        res.prepare_payload();
        return res;
    }

    template<class Request>
    static http::response<http::string_body> server_error(const Request& req, beast::string_view what) {
        http::response<http::string_body> res{http::status::internal_server_error, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = "An error occurred: '" + std::string(what) + "'";
        res.prepare_payload();
        return res;
    }

    template<class Request, class computed_data>
    static http::response<http::string_body> response(const Request& req, computed_data& data)
    {
        json response_object;

        response_object["status"] = "success";
        response_object["total_entries"] = std::to_string(data.total_number_of_fields);
        response_object["client_ip"] = data.client_ip;
        response_object["client_port"] = data.client_port;
        response_object["analysis_type"] = data.analysis_type;
        response_object["message_stats"] = data.message_stats;
        response_object["invalid_json_objects"] = std::to_string(data.invalid_json_objects);

        http::response<http::string_body> res{http::status::ok, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "application/json");
        res.keep_alive(req.keep_alive());
        res.body() = response_object.dump();
        res.prepare_payload();
        return res;
    }
};