#include "log.h"
#include "defines.h"

#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>
#include <filesystem>
#include <fstream>
#include <string>
#include <iostream>
#include <regex>

#ifdef RYDOR_PLATFORM_WIN
#include <windows.h>
#include <wtypes.h>
#endif

namespace rydor::log
{
    struct log_msg_t
    {
        std::string text;
        bool to_console;
    };

    static level_e crt_level = INFO;

    static std::ofstream log_file;
    static std::string base_log_path;
    static u32_t crt_file_index = 0;
    static size_t crt_file_size = 0;
    static size_t max_file_size = 5 * 1024 * 1024;

    static std::mutex queue_mutex;
    static std::condition_variable queue_vc;
    static std::queue<log_msg_t> msg_queue;
    static bool is_running = false;
    static std::thread worker;

    static const char* level_names[] = {
        "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"
    };
    static const char* ansi_level_colors[] = {
        "\033[90m", // TRACE - Gray
        "\033[36m", // DEBUG - Cyan
        "\033[37m", // INFO - White
        "\033[33m", // WARN - Yellow
        "\033[31m", // ERROR - Red
        "\033[91m"  // FATAL - Bright Red
    };
    static const char* ansi_color_reset = "\033[0m";

    void set_level(level_e level)
    {
        crt_level = level;
    }

    void set_max_file_size(size_t new_max_file_size)
    {
        max_file_size = new_max_file_size;
    }

    static std::string rotated_log_path(u32_t index)
    {
        std::filesystem::path path(base_log_path);

        std::string stem = path.stem().string();
        std::string ext = path.extension().string();
        std::filesystem::path parent = path.parent_path();

        std::ostringstream oss;
        oss << stem << "_" << std::setw(3) << std::setfill('0') << index << ext;
        return (parent / oss.str()).string();
    }

    static bool open_new_log_file()
    {
        if (log_file.is_open()) log_file.close();

        std::string path_to_open = crt_file_index == 0 ? base_log_path : rotated_log_path(crt_file_index);
        
        std::filesystem::create_directories(std::filesystem::path(path_to_open).parent_path());

        log_file.open(path_to_open, std::ios::out | std::ios::app);
        if (!log_file.is_open())
        {
            return false;
        }

        log_file.seekp(0, std::ios::end);
        crt_file_size = (size_t)log_file.tellp();

        return true;
    }

    bool to_file(const std::string& path)
    {
        std::filesystem::path fs_path(path);

        if (!std::filesystem::is_directory(fs_path))
        {
            std::filesystem::create_directories(fs_path);
        }

        if (path.back() == '/' || path.back() == '\\' || !fs_path.has_extension()) {
            std::time_t now = std::time(nullptr);
            char buf[32];
            std::strftime(buf, sizeof(buf), "%Y-%m-%d_log.txt", std::localtime(&now));
            fs_path /= buf;
        }

        base_log_path = fs_path.string();
        return open_new_log_file();
    }

    static std::string format_line(level_e level, const char* category, std::string_view msg, bool include_date)
    {
        std::time_t now = std::time(nullptr);
        char time_string_buf[32];
        
        if (include_date)
        {
            std::strftime(time_string_buf, sizeof(time_string_buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
        }
        else
        {
            std::strftime(time_string_buf, sizeof(time_string_buf), "%H:%M:%S", std::localtime(&now));
        }

        std::ostringstream out;
        out << "[" << time_string_buf << "] [" << level_names[level] << "] [" << category << "] " << msg;
        return out.str();
    }

    void write(level_e level, const char* category, std::string_view msg)
    {
        if (level < crt_level) return;

        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            msg_queue.push({ansi_level_colors[level] + format_line(level, category, msg, false) + ansi_color_reset, true});
            if (log_file.is_open())
            {
                msg_queue.push({format_line(level, category, msg, true), false});
            }
        }

        queue_vc.notify_one();
    }

#if RYDOR_PLATFORM_WIN
    // Need to manually enable the processing of escape sequences in Windows in console
    static void setup_ansi_console_colors_windows() 
    {
        HANDLE h_out = GetStdHandle(STD_OUTPUT_HANDLE);
        if (h_out == INVALID_HANDLE_VALUE) return;

        DWORD dw_mode = 0;
        if (!GetConsoleMode(h_out, &dw_mode)) return;

        dw_mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(h_out, dw_mode);
    }
#endif

    void start()
    {
        is_running = true;
#if RYDOR_PLATFORM_WIN
        setup_ansi_console_colors_windows();
#endif
        worker = std::thread([] 
        {
            while (is_running || !msg_queue.empty())
            {
                std::unique_lock<std::mutex> lock(queue_mutex);
                queue_vc.wait(lock, [] { return !msg_queue.empty() || !is_running; });

                while (!msg_queue.empty())
                {
                    log_msg_t msg = msg_queue.front();
                    msg_queue.pop();

                    if (msg.to_console)
                    {
                        std::cout << msg.text << "\n";
                        continue;
                    }

                    if (log_file.is_open())
                    {
                        log_file << msg.text << "\n";
                        crt_file_size += msg.text.size() + 1;
                        log_file.flush();

                        if (crt_file_size >= max_file_size)
                        {
                            crt_file_index++;
                            open_new_log_file();
                        }
                    }
                }
            }
        });
    }

    void shutdown()
    {
        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            is_running = false;
        }

        queue_vc.notify_all();
        if (worker.joinable()) worker.join();
    }

} // namespace rydor::log
