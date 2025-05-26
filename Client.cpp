#include "boost.hpp"
#include "Helper.hpp"
#include <cstdlib>
#include <functional>
#include <thread>
#include <chrono>

int main(int argc, char *argv[])
{
    std::string programPath = argv[0];
    std::string programName = programNameResolver(programPath);
    size_t portNumber = 0, clientId = 0;
    bool portProvidedWithFlag = false, clientIdProvidedWithFlag = false;

    // Parse command line arguments 
    for (int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];
        if (arg == "-id" && i + 1 < argc)
        {
            try
            {
                clientId = std::stoi(argv[i + 1]);
                if (clientId <= 0)
                {
                    std::cerr << "[ERROR] Client Id must be a number" << std::endl;
                    return 1;
                }
                clientIdProvidedWithFlag = true;
                i++;
            }
            catch (const std::exception &)
            {
                std::cerr << "[ERROR] Client must be a valid integer" << std::endl;
                return 1;
            }
        }
        else if (i == 1 && !clientIdProvidedWithFlag)
        {
            try
            {
                clientId = std::stoi(arg);
                if (clientId <= 0)
                {
                    std::cerr << "[ERROR] Client Id must be a number" << std::endl;
                    return 1;
                }
            }
            catch (const std::exception &)
            {
                std::cerr << "[ERROR] Usage: Program -id <client-id> -p <port>"  << std::endl;
                return 1;
            }
        }

        if (arg == "-p" && i + 1 < argc)
        {
            try
            {
                portNumber = std::stoi(argv[i + 1]);
                if (portNumber < 1024 || portNumber > 65535)
                {
                    std::cerr << "[ERROR] Port must be between 1024 and 65535" << std::endl;
                    return 1;
                }
                portProvidedWithFlag = true;
                i++;
            }
            catch (const std::exception &)
            {
                std::cerr << "[ERROR] Usage: Program -id <client-id> -p <port>"  << std::endl;
                return 1;
            }
        }
        else if (i == 2 && !portProvidedWithFlag)
        {
            try
            {
                portNumber = std::stoi(arg);
                if (portNumber < 1024 || portNumber > 65535)
                {
                    std::cerr << "[ERROR] Port must be between 1024 and 65535" << std::endl;
                    return 1;
                }
            }
            catch (const std::exception &)
            {
                std::cerr << "[ERROR] Usage: Program -id <client-id> -p <port>"  << std::endl;
                return 1;
            }
        }
    }
    
    if(clientId == 0)
    {
        std::cerr << "[ERROR] Usage: Program -id <client-id> -p <port>" << std::endl;
        return 1;
    }

    if (portNumber == 0)
    {
        portNumber = 9000;
        std::cout << "[INFO] No port provided, Using default port: " << portNumber << "\n" << std::endl;
    }

    try
    {
        std::string host = "127.0.0.1";
        asio::io_context ioc;
        tcp::resolver resolver(ioc);
        beast::error_code ec;
        beast::tcp_stream stream(ioc);

        auto const result = resolver.resolve(host, std::to_string(portNumber), ec);
        stream.connect(result, ec);

        if (ec)
        {
            std::cerr << "[ERROR] Connecting to Server: " << ec.message() << std::endl;
            return 1;
        }

        stream.socket().set_option(tcp::no_delay(true)); // Speed up small chunk sending
        tcp::endpoint server_ = stream.socket().remote_endpoint();
        auto server_ip = server_.address().to_string();
        auto server_port = server_.port();

        std::cout << "[INFO] Connected to server on IP: " << server_ip << " , PORT: " << server_port << std::endl;
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

        std::string json_data = read_file(json_path);
        std::string xml_data = read_file(xml_path);
        std::string txt_data = read_file(txt_path);

        if (json_data.empty() || xml_data.empty() || txt_data.empty()) {
            std::cerr << "[ERROR] One or more files are missing or empty." << std::endl;
            return 1;
        }
        //Create multipart/form-data body
        std::string boundary = "----boundary1234567890";
        std::ostringstream body_stream;
        std::cout << "[INFO] Processing JSON file: " << get_file_name(json_path) << std::endl;
        body_stream << "--" << boundary << "\r\n"
             << "Content-Disposition: form-data; name=\"file_json\"; filename=\"log_file.json\"\r\n"
             << "Content-Type: application/json\r\n\r\n"
             << json_data << "\r\n";
        
        std::cout << "[INFO] Processing XML file: " << get_file_name(xml_path) << std::endl;
        body_stream << "--" << boundary << "\r\n"
             << "Content-Disposition: form-data; name=\"file_xml\"; filename=\"log_file.xml\"\r\n"
             << "Content-Type: application/xml\r\n\r\n"
             << xml_data << "\r\n";
        
        std::cout << "[INFO] Processing Text file: " << get_file_name(txt_path) << std::endl;
        body_stream << "--" << boundary << "\r\n"
             << "Content-Disposition: form-data; name=\"file_txt\"; filename=\"log_file.txt\"\r\n"
             << "Content-Type: text/plain\r\n\r\n"
             << txt_data << "\r\n"
             << "--" << boundary << "--\r\n";

        std::string body_str = body_stream.str();
        size_t total_size = body_str.size();
        double size_mb = static_cast<double>(total_size) / (1024 * 1024);
        std::cout << "[INFO] Total upload size: " << std::fixed << std::setprecision(2) << size_mb << " MB\n" << std::endl;   
             // Prepare HTTP request headers only (no body yet)
        http::request<http::dynamic_body> req{http::verb::post, "/", 11};
        req.set(http::field::host, host);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        req.set(http::field::connection, "keep-alive");
        req.set("Client-Id", std::to_string(clientId));
        req.set(http::field::content_length, std::to_string(total_size));
        req.set(http::field::content_type, "multipart/form-data; boundary=" + boundary);

        auto body_buffer = req.body().prepare(total_size);
        asio::buffer_copy(body_buffer, asio::buffer(body_str));
        req.body().commit(total_size);

        // Send to Server
        LOG("[INFO] Uploading log files...\n");
        beast::error_code write_ec;
        http::write(stream, req, write_ec);
        if (write_ec) {
            std::cerr << "[ERROR] Sending to server: " << write_ec.message() << std::endl;
            return 1;
        }

        beast::flat_buffer buffer;
        http::response<http::string_body> res;

        std::cout << "[INFO] Request sent successfully" << std::endl;
        std::cout << "[INFO] Awaiting server response...\n" << std::endl;

        http::read(stream, buffer, res, ec);
        if (ec) {
            std::cerr << "[ERROR] Reading response: " << ec.message() << std::endl;
            return 1;
        }

        json data = nlohmann::json::parse(res.body());

        LOG("ANALYSIS: LOG LEVEL");
        LOG("SERVER IP: " << server_ip);
        LOG("SERVER PORT: " << server_port);
        LOG("INVALID DATA: " << std::stoi(data["invalid_data"].get<std::string>()));
        LOG("TOTAL ENTRIES: " << std::stoi(data["total_entries"].get<std::string>()) << "\n");
        
        print_response(data);

        if (res.need_eof() || res.find(http::field::connection) == res.end() ||
            res[http::field::connection] != "keep-alive") {
            std::cout << "\n" << "[INFO] Connection closed by server" << std::endl;
        }
        stream.socket().shutdown(tcp::socket::shutdown_both, ec);
    }
    catch (std::exception &error)
    {
        LOG("[ERROR] Reason:  " << error.what());
    }
    return 0;
}
