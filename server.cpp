#include "helpers/classes/listener.hpp"
#include "helpers/helper.hpp"
#include <filesystem>
#include <iostream>
#include <print>
#include <thread>

int main(int argc, char *argv[]) {
  std::string programPath = argv[0];
  std::string programName = programNameResolver(programPath);
  int portNumber = -1;
  bool portProvidedWithFlag = false;

  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];
    if (arg == "-p" && i + 1 < argc) {
      try {
        portNumber = std::stoi(argv[i + 1]);
        if (portNumber < 1024 || portNumber > 65535) {
          std::println(stderr, "[ERROR] Port must be between 1024 and 65535");
          return 1;
        }
        portProvidedWithFlag = true;
        i++;
      } catch (const std::exception &) {
        std::println(stderr, "[ERROR] Port must be a valid integer");
        return 1;
      }
    } else if (i == 1 && !portProvidedWithFlag) {
      try {
        portNumber = std::stoi(arg);
        if (portNumber < 1024 || portNumber > 65535) {
          std::println(stderr, "[ERROR] Port must be between 1024 and 65535");
          return 1;
        }
      } catch (const std::exception &) {
        std::println(stderr, "[ERROR] Port must be a valid integer");
        return 1;
      }
    }
  }

  if (portNumber == -1) {
    portNumber = 9000;
    std::println("[INFO] No port provided, Using default port: {}", portNumber);
  }

  try {
    auto const address = asio::ip::make_address("127.0.0.1");
    auto const port = static_cast<short unsigned int>(portNumber);

    // Create and verify Document Root
    std::filesystem::path doc_root_path("./public");

    // Check if directory exists, create it if it doesn't
    if (!std::filesystem::exists(doc_root_path)) {
      std::println(
          "[INFO] Document root directory does not exist, creating it...");

      try {
        if (!std::filesystem::create_directory(doc_root_path)) {
          std::println(
              stderr, "[FATAL] Failed to create document root directory at: {}",
              doc_root_path.string());
          return 1;
        }
        std::println(
            "[INFO] Successfully created document root directory at: {}",
            doc_root_path.string());
      } catch (const std::filesystem::filesystem_error &e) {
        std::println(stderr,
                     "[FATAL] Error creating document root directory: {}",
                     e.what());
        return 1;
      }
    }

    // Verify it's actually a directory (might be a file with the same name)
    if (!std::filesystem::is_directory(doc_root_path)) {
      std::println(stderr,
                   "[FATAL] Document root exists but is not a directory: {}",
                   doc_root_path.string());
      return 1;
    }

    auto const doc_root = std::make_shared<std::string>(doc_root_path.string());

    // Calculate optimal thread count based on hardware
    auto const thread_count =
        std::max<unsigned int>(1, std::thread::hardware_concurrency());

    // The io_context is required for all I/O
    asio::io_context ioc{static_cast<int>(thread_count)};

    // Create a work guard to keep the io_context alive
    auto work_guard = asio::make_work_guard(ioc);

    // Create and launch a listening port
    std::shared_ptr<listener> http_listener =
        std::make_shared<listener>(ioc, tcp::endpoint{address, port}, doc_root);
    http_listener->run();

    // Start the worker threads
    std::vector<std::thread> threads;
    threads.reserve(thread_count - 1);

    // Create a thread to handle user input without blocking server operation
    std::atomic<bool> stop_requested{false};
    std::thread input_thread([&stop_requested]() {
      std::println("[INFO] Press Enter or CTRL + C to stop the server...");
      std::cin.get();
      stop_requested.store(true);
      std::println("[INFO] Shutdown requested by user.");
    });

    // Start worker threads
    for (auto i = thread_count - 1; i > 0; --i) {
      threads.emplace_back([&ioc] {
        try {
          ioc.run();
        } catch (const std::exception &e) {
          std::println(stderr, "[ERROR] Worker thread exception: {}", e.what());
        }
      });
    }

    // Run io_context in this thread until stop is requested
    while (!stop_requested.load()) {
      try {
        ioc.poll(); // Non-blocking poll to process any pending handlers
        std::this_thread::sleep_for(
            std::chrono::milliseconds(100)); // Small sleep to avoid CPU spin
      } catch (const std::exception &e) {
        std::println(stderr, "[ERROR] Main thread exception: {}", e.what());
      }
    }

    // Signal all threads to stop by releasing the work guard
    std::println("[INFO] Stopping server...");
    work_guard.reset();
    ioc.stop();

    // Join the input thread
    if (input_thread.joinable()) {
      input_thread.join();
    }

    // Wait for all worker threads to complete
    for (auto &thread : threads) {
      if (thread.joinable()) {
        thread.join();
      }
    }

    std::println("[INFO] Server stopped successfully");
  } catch (std::exception &error) {
    std::println(stderr, "[ERROR] Error processing request: {}", error.what());
  }

  return 0;
}