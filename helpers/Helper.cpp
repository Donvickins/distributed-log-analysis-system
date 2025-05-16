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
    ss << std::put_time(std::localtime(&itt), "%Y-%m-%d %H:%M:%S"); // Local time with normal date format
    return ss.str();
}


std::string save_file(const std::string& clientId, const std::string& ip, 
    const std::string& content, const std::string& extension) {
    std::string timestamp = get_timestamp_str();
    std::string sanitized_ip = sanitize_ip(ip);
    std::string file_name = clientId + "_" + sanitized_ip + "_" + timestamp + extension;
    
    fs::path dir = "./logs/" + clientId + "/";
    fs::path file_path = dir / file_name;
    
    if (!fs::exists(dir)) {
        fs::create_directories(dir);
    }
    
    std::ofstream file(file_path, std::ios::app);
    file << content;
    return file_name;
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