#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <cstdlib>
#include <fstream>
#include <print>

#include "lib/utils.hpp"
#include "simdjson.h"

namespace beast = boost::beast;
namespace http = beast::http;
namespace asio = boost::asio;
using tcp = asio::ip::tcp;

int main(int argc, char* argv[]) {
  auto cfg = parse_cli_args_client(argc, argv);

  if (!cfg) return 1;

  const ClientConfig& config = *cfg;

  try {
    std::string host = "0.0.0.0";
    asio::io_context ioc;
    tcp::resolver resolver(ioc);
    beast::error_code ec;
    beast::tcp_stream stream(ioc);

    auto const result = resolver.resolve(host, std::to_string(config.port), ec);
    auto output = stream.connect(result, ec);
    boost::ignore_unused(output);
    if (ec) {
      std::cerr << "[ERROR] Connecting to Server: " << ec.message() << std::endl;
      return 1;
    }

    stream.socket().set_option(tcp::no_delay(true));  // Speed up small chunk sending
    tcp::endpoint server_ = stream.socket().remote_endpoint();
    auto server_ip = server_.address().to_string();
    auto server_port = server_.port();

    std::cout << "[INFO] Connected to server on IP: " << server_ip << " , PORT: " << server_port
              << std::endl;
    // Prepare file paths
    std::string dir = "./logs";
    std::string json_path = path_cat(dir, "/log_file.json");
    std::string xml_path = path_cat(dir, "/log_file.xml");
    std::string txt_path = path_cat(dir, "/log_file.txt");

    // Read files into strings
    auto read_file = [](const std::string& path) -> std::string {
      std::ifstream file(path, std::ios::binary);
      if (!file) return "";
      std::ostringstream ss;
      ss << file.rdbuf();
      return ss.str();
    };

    auto get_file_name = [](const std::string& path) -> std::string {
      fs::path p(path);
      return p.filename().string();
    };
    // simdjson::dom::parser parser;
    // simdjson::dom::element json_data = parser.load(json_path);

    simdjson::ondemand::parser parser;
    auto json_file = simdjson::padded_string::load(json_path);
    simdjson::ondemand::document json_data = parser.iterate(json_file);
    std::string xml_data = read_file(xml_path);
    std::string txt_data = read_file(txt_path);

    if (json_data.is_null() || xml_data.empty() || txt_data.empty()) {
      std::cerr << "[ERROR] One or more files are missing or empty." << std::endl;
      return 1;
    }
    // Create multipart/form-data body
    std::string boundary = "----boundary1234567890";
    std::ostringstream body_stream;
    std::cout << "[INFO] Processing JSON file: " << get_file_name(json_path) << std::endl;
    body_stream << "--" << boundary << "\r\n"
                << "Content-Disposition: form-data; name=\"file_json\"; "
                   "filename=\"log_file.json\"\r\n"
                << "Content-Type: application/json\r\n\r\n"
                << json_data << "\r\n";

    std::cout << "[INFO] Processing XML file: " << get_file_name(xml_path) << std::endl;
    body_stream << "--" << boundary << "\r\n"
                << "Content-Disposition: form-data; name=\"file_xml\"; "
                   "filename=\"log_file.xml\"\r\n"
                << "Content-Type: application/xml\r\n\r\n"
                << xml_data << "\r\n";

    std::cout << "[INFO] Processing Text file: " << get_file_name(txt_path) << std::endl;
    body_stream << "--" << boundary << "\r\n"
                << "Content-Disposition: form-data; name=\"file_txt\"; "
                   "filename=\"log_file.txt\"\r\n"
                << "Content-Type: text/plain\r\n\r\n"
                << txt_data << "\r\n"
                << "--" << boundary << "--\r\n";

    std::string body_str = body_stream.str();
    size_t total_size = body_str.size();
    double size_mb = static_cast<double>(total_size) / (1024 * 1024);
    std::cout << "[INFO] Total upload size: " << std::fixed << std::setprecision(2) << size_mb
              << " MB\n"
              << std::endl;
    // Prepare HTTP request headers only (no body yet)
    http::request<http::dynamic_body> req{http::verb::post, "/", 11};
    req.set(http::field::host, host);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    req.set(http::field::connection, "keep-alive");
    req.set("Client-Id", config.clientId);
    req.set(http::field::content_length, std::to_string(total_size));
    req.set(http::field::content_type, "multipart/form-data; boundary=" + boundary);

    auto body_buffer = req.body().prepare(total_size);
    boost::ignore_unused(asio::buffer_copy(body_buffer, asio::buffer(body_str)));
    req.body().commit(total_size);

    // Send to Server
    std::println("[INFO] Uploading log files...\n");
    beast::error_code write_ec;
    http::write(stream, req, write_ec);
    if (write_ec) {
      std::cerr << "[ERROR] Sending to server: " << write_ec.message() << std::endl;
      return 1;
    }

    beast::flat_buffer buffer;
    http::response<http::string_body> res;

    std::println("[INFO] Request sent successfully");
    std::println("[INFO] Awaiting server response...\n");

    http::read(stream, buffer, res, ec);
    if (ec) {
      std::cerr << "[ERROR] Reading response: " << ec.message() << std::endl;
      return 1;
    }

    boost::json::value data = boost::json::parse(res.body());

    std::println("ANALYSIS: LOG LEVEL");
    std::println("SERVER IP: {}", server_ip);
    std::println("SERVER PORT: {}", server_port);
    std::println("INVALID DATA: {}", static_cast<int>(data.at("invalid_data").as_int64()));
    std::println("TOTAL ENTRIES: {}", static_cast<int>(data.at("total_entries").as_int64()));

    print_response(data.at("message_stats"));

    if (res.need_eof() || res.find(http::field::connection) == res.end() ||
        res[http::field::connection] != "keep-alive") {
      std::println("[INFO] Connection closed by server");
    }
    auto shutdown_result = stream.socket().shutdown(tcp::socket::shutdown_both, ec);
    boost::ignore_unused(shutdown_result);
  } catch (std::exception& error) {
    std::println(stderr, "[ERROR] Reason: {}", error.what());
  }
  return 0;
}
