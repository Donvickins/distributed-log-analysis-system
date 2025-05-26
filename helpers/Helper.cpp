#include "Helper.hpp"


// When given a program path, it returns the program name, else it returns the given path
std::string programNameResolver(std::string programPath)
{
    std::string programName;

    size_t pos = programPath.find_last_of("/\\");
    if (pos != std::string::npos)
    {
        return programName = programPath.substr(pos + 1);
    }
    else
    {
        return programPath;
    }
}

// Returns a reasonable mime type based on the extension of a file
beast::string_view mime_type(beast::string_view path)
{
    using beast::iequals;
    auto const ext = [&path]
    {
        auto const pos = path.rfind(".");
        if (pos == beast::string_view::npos)
            return beast::string_view{};
        return path.substr(pos);
    }();
    if (iequals(ext, ".htm"))
        return "text/html";
    if (iequals(ext, ".html"))
        return "text/html";
    if (iequals(ext, ".php"))
        return "text/html";
    if (iequals(ext, ".css"))
        return "text/css";
    if (iequals(ext, ".txt"))
        return "text/plain";
    if (iequals(ext, ".js"))
        return "application/javascript";
    if (iequals(ext, ".json"))
        return "application/json";
    if (iequals(ext, ".xml"))
        return "application/xml";
    if (iequals(ext, ".swf"))
        return "application/x-shockwave-flash";
    if (iequals(ext, ".flv"))
        return "video/x-flv";
    if (iequals(ext, ".png"))
        return "image/png";
    if (iequals(ext, ".jpe"))
        return "image/jpeg";
    if (iequals(ext, ".jpeg"))
        return "image/jpeg";
    if (iequals(ext, ".jpg"))
        return "image/jpeg";
    if (iequals(ext, ".gif"))
        return "image/gif";
    if (iequals(ext, ".bmp"))
        return "image/bmp";
    if (iequals(ext, ".ico"))
        return "image/vnd.microsoft.icon";
    if (iequals(ext, ".tiff"))
        return "image/tiff";
    if (iequals(ext, ".tif"))
        return "image/tiff";
    if (iequals(ext, ".svg"))
        return "image/svg+xml";
    if (iequals(ext, ".svgz"))
        return "image/svg+xml";
    return "application/text";
}

// Append an HTTP rel-path to a local filesystem path.
// The returned path is normalized for the platform.
std::string path_cat(beast::string_view base, beast::string_view path)
{
    if (base.empty())
        return std::string(path);
    std::string result(base);
#ifdef BOOST_MSVC
    char constexpr path_separator = '\\';
    if (result.back() == path_separator)
        result.resize(result.size() - 1);
    result.append(path.data(), path.size());
    for (auto &c : result)
        if (c == '/')
            c = path_separator;
#else
    char constexpr path_separator = '/';
    if (result.back() == path_separator)
        result.resize(result.size() - 1);
    result.append(path.data(), path.size());
#endif
    return result;
}

// Report a failure
void fail(beast::error_code ec, char const *what)
{
    std::cerr << what << ": " << ec.message() << "\n";
}

std::string sanitize_ip(const std::string& ip) {
    std::string sanitized_ip = ip;
    std::replace(sanitized_ip.begin(), sanitized_ip.end(), '.', '_');
    return sanitized_ip;
}

std::string get_timestamp_str() {
    auto now = std::chrono::system_clock::now();
    auto itt = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&itt), "%Y-%m-%d_%H:%M:%S"); // Local time with normal date format
    return ss.str();
}


std::map<std::string, std::string> save_file(const std::string& content, const std::string& client_id, const std::string& ip, const std::string& ext) {
    std::string directory = "./storage/Client#" + client_id + "/";
    fs::path dir = directory;
    std::string construct_file_name = get_timestamp_str() + "_" + sanitize_ip(ip) + ext;
    fs::path file_name = construct_file_name;
    fs::path file_path = dir / file_name;

    if (!fs::exists(dir)) {
        if(!fs::create_directories(dir))
        throw std::runtime_error("Failed to create directory");
    }

    std::fstream file;
    file.open(file_path, std::ios::out);

    if (!file.is_open()) {
        throw std::runtime_error("[ERROR] Failed to open file: " + file_path.string());
    }

    file << content;
    file.close();
    return std::map<std::string, std::string>{{"status", "success"}, {"file_name", file_name.string()}};
}

bool is_valid_content_type(const std::string& content_type) {
        static const std::vector<std::string> valid_types = {
            "application/json",
            "application/xml",
            "text/plain"
        };
        return std::find(valid_types.begin(), valid_types.end(), content_type) != valid_types.end();
}

std::string trim(const std::string& data){
    size_t first = data.find_first_not_of(" \t\n\r\f\v");
    size_t last = data.find_last_not_of(" \t\n\r\f\v");

    if(first == std::string::npos)
        return "";

    return data.substr(first, last - first + 1);
}

bool is_log_level(const std::string& log_level){
    const std::vector<std::string> valid_log_level = {"INFO", "DEBUG", "WARN", "ERROR", "CRITICAL"};
    return std::find(valid_log_level.begin(), valid_log_level.end(), log_level) != valid_log_level.end();
}

std::string get_file(const fs::path& doc_root, const std::string& file_name, const std::string& clientId, const std::string& file_ext){

    if(doc_root.empty() || file_name.empty()){
        std::cerr << "[ERROR] Document root is empty" << std::endl;
        return "";
    }

    if(!fs::is_directory(doc_root)){
        std::cerr << "[ERROR] Document root is not a directory" << std::endl;
        return "";
    }

    if(!fs::exists(doc_root)){
        std::cerr << "[ERROR] Document root does not exist" << std::endl;
        return "";
    }
    
    std::string file_path = path_cat(doc_root.string(), "/Client#" + clientId + "/" +  file_name + "." + file_ext);

    if(!fs::exists(file_path)){
        std::cerr << "[ERROR] File does not exist: " << file_path << std::endl;
        return "";
    }

    std::ifstream file(file_path);
    if(!file.is_open()){
        std::cerr << "[ERROR] Failed to open file: " << file_path << std::endl;
        return "";
    }

    std::string file_content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    return file_content;
}

bool has_ext(const std::string& filename, const std::string& ext) {
    std::size_t idx = filename.rfind('.');
    if (idx != std::string::npos) {
        std::string extension = filename.substr(idx + 1);
        return extension == ext;
    }
    return false;
}

void print_response(const json& j){
    int log_level_len = 25, 
            message_len = 35, 
            times_occurred_len = 15;

    std::cout << std::left << std::setw(log_level_len) << "LOG LEVEL" <<
        std::setw(message_len) << "MESSAGE" <<
        std::setw(times_occurred_len) << "TIMES OCCURED" <<
        std::endl; 

    std::cout << std::setfill('-') << std::setw(log_level_len + message_len + times_occurred_len) << "" << std::endl;
    std::cout << std::setfill(' ');

    for(const auto& [log_level, content] : j["message_stats"].items()){
        std::cout << std::left << std::setw(log_level_len) << log_level;
        bool is_first = true;

        for(const auto& [key,value]: content.items()){
            if(is_first){
                std::cout << std::setw(message_len) << key
                    << std::setw(times_occurred_len) << value << std::endl;
                is_first = false;
            }else{
                std::cout << std::setw(log_level_len) << ""
                    << std::setw(message_len) << key
                    << std::setw(times_occurred_len) << value << std::endl;
            }
            
        }

        std::cout << std::setfill('-') << std::setw(log_level_len + message_len + times_occurred_len) << "" << std::endl;
        std::cout << std::setfill(' ');
    }
};