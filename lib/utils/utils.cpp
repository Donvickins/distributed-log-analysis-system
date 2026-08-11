#include "utils.hpp"

#include <cerrno>
#include <csignal>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <print>
#include <thread>

#ifdef _WIN32
#include <conio.h>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <poll.h>
#include <unistd.h>
#endif

#include "../network/listener.hpp"

std::string path_cat(std::string base, std::string_view path) {
  if (base.empty()) return std::string(path);
#ifdef BOOST_MSVC
  char constexpr path_separator = '\\';
  if (base.back() == path_separator) base.resize(base.size() - 1);
  base.append(path.data(), path.size());
  for (auto& c : base)
    if (c == '/') c = path_separator;
#else
  char constexpr path_separator = '/';
  if (base.back() == path_separator) base.resize(base.size() - 1);
  base.append(path.data(), path.size());
#endif
  return base;
}

std::string get_file(const std::filesystem::path& doc_root,
    const std::string& file_name,
    const std::string& clientId,
    const std::string& file_ext) {
  if (doc_root.empty() || file_name.empty()) {
    std::println("[ERROR] Document root is empty");
    return "";
  }

  if (!std::filesystem::is_directory(doc_root)) {
    std::println("[ERROR] Document root is not a directory");
    return "";
  }

  if (!std::filesystem::exists(doc_root)) {
    std::println("[ERROR] Document root does not exist");
    return "";
  }

  std::string file_path =
      path_cat(doc_root.string(), "/Client#" + clientId + "/" + file_name + "." + file_ext);

  if (!std::filesystem::exists(file_path)) {
    std::println("[ERROR] File does not exist: {}", file_path);
    return "";
  }

  std::ifstream file(file_path);
  if (!file.is_open()) {
    std::println("[ERROR] Failed to open file: {}", file_path);
    return "";
  }

  std::string file_content((std::istreambuf_iterator<char>(file)),
      std::istreambuf_iterator<char>());

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

void print_response(const boost::json::value& j) {
  int log_level_len = 25, message_len = 35, times_occurred_len = 15;

  std::println("{:<{}} {:<{}} {:<{}}",
      "LOG LEVEL",
      log_level_len,
      "MESSAGE",
      message_len,
      "TIMES OCCURRED",
      times_occurred_len);

  std::println("{:-<{}}", "", log_level_len + message_len + times_occurred_len);

  for (const auto& [log_level, content] : j.as_object()) {
    std::println("{:<{}}", log_level, log_level_len);
    bool is_first = true;

    for (const auto& [key, value] : content.as_object()) {
      if (is_first) {
        std::println("{:<{}}", key, message_len);
        std::println("{:<{}}", value.as_string(), times_occurred_len);
        is_first = false;
      } else {
        std::println("{:<{}} {:<{}}",
            "",
            log_level_len,
            key,
            message_len,
            value.as_string(),
            times_occurred_len);
      }
    }

    std::println("{:-<{}}", "", log_level_len + message_len + times_occurred_len);
  }
};

std::optional<std::filesystem::path> setup_public_dir() {
  std::filesystem::path doc_root_path("./public");

  if (!std::filesystem::exists(doc_root_path)) {
    std::println("[INFO] Document root directory does not exist, creating it...");

    try {
      if (!std::filesystem::create_directory(doc_root_path)) {
        std::println(stderr,
            "[FATAL] Failed to create document root directory at: {}",
            doc_root_path.string());
        return std::nullopt;
      }
      std::println("[INFO] Successfully created document root directory at: {}",
          doc_root_path.string());
    } catch (const std::filesystem::filesystem_error& e) {
      std::println(stderr, "[FATAL] Error creating document root directory: {}", e.what());
      return std::nullopt;
    }
  }

  if (!std::filesystem::is_directory(doc_root_path)) {
    std::println(stderr,
        "[FATAL] Document root exists but is not a directory: {}",
        doc_root_path.string());
    return std::nullopt;
  }

  return doc_root_path;
};

static std::atomic<bool> stop_requested{false};

#ifndef _WIN32
static void handle_terminate(int signal) {
  if (signal == SIGINT || signal == SIGTERM) stop_requested.store(true);
};
#else
static BOOL WINAPI console_ctrl_handler(DWORD ctrl_type) {
  if (ctrl_type == CTRL_C_EVENT || ctrl_type == CTRL_BREAK_EVENT || ctrl_type == CTRL_CLOSE_EVENT) {
    stop_requested.store(true);
    return TRUE;
  }
  return FALSE;
};
#endif

bool init_server(unsigned short port) {
#ifdef _WIN32
  SetConsoleCtrlHandler(console_ctrl_handler, TRUE);
#else
  std::signal(SIGTERM, handle_terminate);
  std::signal(SIGINT, handle_terminate);
#endif

  try {
    auto const address = asio::ip::make_address("0.0.0.0");

    std::optional<std::filesystem::path> const doc_root_path = setup_public_dir();
    if (!doc_root_path) return false;

    std::shared_ptr const doc_root = std::make_shared<std::string>(doc_root_path.value().string());

    // Calculate optimal thread count based on hardware
    unsigned int const thread_count =
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

#ifdef _WIN32
    std::thread input_thread([] {
      std::println("[INFO] /quit + Enter or CTRL + C to stop the server...");
      while (!stop_requested.load()) {
        if (_kbhit()) {
          std::string command{};
          if (!std::getline(std::cin, command)) break;
          if (command == "/quit") stop_requested.store(true);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
      }
    });
#else
    std::thread input_thread([] {
      std::println("[INFO] /quit + Enter or CTRL + C to stop the server...");
      struct pollfd stdin_poll{STDIN_FILENO, POLLIN, 0};
      while (!stop_requested.load()) {
        int ready = ::poll(&stdin_poll, 1, 200);
        if (ready < 0 && errno == EINTR) continue;
        if (ready > 0 && (stdin_poll.revents & (POLLIN | POLLHUP | POLLERR | POLLNVAL))) {
          std::string command{};
          if (!std::getline(std::cin, command)) break;
          if (command == "/quit") stop_requested.store(true);
        }
      }
    });
#endif

    // Start worker threads
    for (auto i = thread_count - 1; i > 0; --i) {
      threads.emplace_back([&ioc] {
        try {
          ioc.run();
        } catch (const std::exception& err) {
          std::println(stderr, "[ERROR] Worker thread exception: {}", err.what());
        }
      });
    }

    // Run io_context in this thread until stop is requested
    while (!stop_requested.load()) {
      try {
        ioc.poll();  // Non-blocking poll to process any pending handlers
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      } catch (const std::exception& e) {
        std::println(stderr, "[ERROR] Main thread exception: {}", e.what());
      }
    }

    // Signal all threads to stop by releasing the work guard
    std::println("\n[INFO] Stopping server...");
    work_guard.reset();
    ioc.stop();

    if (input_thread.joinable()) input_thread.join();

    // Wait for all worker threads to complete
    for (auto& thread : threads) {
      if (thread.joinable()) {
        thread.join();
      }
    }

    std::println("[INFO] Server stopped successfully");
  } catch (std::exception& error) {
    std::println(stderr, "[ERROR] Error processing request: {}", error.what());
  }

  return true;
}
