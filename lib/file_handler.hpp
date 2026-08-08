#pragma once

#include <string>

#include <boost/asio/ip/tcp.hpp>
#include <boost/json.hpp>

using tcp = boost::asio::ip::tcp;

struct computed_data {
  size_t total_fields;
  size_t invalid_fields;
  boost::json::object message_stats;
  std::string error_message;
};

computed_data process_json_request(const std::string &body);
computed_data parse_text_file(const std::string &body);
computed_data parse_xml_file(const std::string &body);
boost::json::object
merge_json_objects(const std::vector<boost::json::value> &json_array);