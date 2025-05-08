#include "helpers/Helper.hpp"
#include <cstdlib>

int main(int argc, char *argv[])
{
    std::string programPath = argv[0];
    std::string programName = programNameResolver(programPath);
    int portNumber = -1;
    bool portProvidedWithFlag = false;

    // Parse command line arguments manually since we're on Windows
    for (int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];
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
                i++; // Skip the next argument since we used it
            }
            catch (const std::exception &)
            {
                std::cerr << "[ERROR] Port must be a valid integer" << std::endl;
                return 1;
            }
        }
        else if (i == 1 && !portProvidedWithFlag)
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
                std::cerr << "[ERROR] Port must be a valid integer" << std::endl;
                return 1;
            }
        }
    }

    if (portNumber == -1)
    {
        portNumber = 9000;
        std::cout << "[INFO] No port provided, Using default port: " << portNumber << std::endl;
    }

    try
    {
        // Asio context
        asio::io_context context;

        // Get the address
        tcp::resolver resolver(context);
        auto const results = resolver.resolve("127.0.0.1", std::to_string(portNumber));

        // Create and connect the socket
        tcp::socket socket(context);
        asio::connect(socket, results.begin(), results.end());

        // Prepare the buffer for receiving data
        beast::flat_buffer buffer;
        std::string response;
        response.resize(1024);

        // Receive the response
        size_t bytes = socket.read_some(asio::buffer(&response[0], response.size()));
        response.resize(bytes);

        std::cout << "Received: " << response << std::endl;
    }
    catch (std::exception &error)
    {
        LOG("Error: " << error.what());
    }

    std::cin.get();
    return 0;
}
