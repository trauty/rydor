#pragma once
#include <string>
#include <fmt/format.h>

namespace rydor::log 
{
    enum level_e
    {
        TRACE,
        DEBUG,
        INFO,
        WARN,
        ERROR,
        FATAL
    };

    void set_level(level_e level);
    void set_max_file_size(size_t size);
    bool to_file(const std::string& path);
    void start();
    void shutdown();
    void write(level_e level, const char* category, std::string_view msg);
} // namespace rydor::log

#ifndef RYDOR_LOG_LEVEL
#define RYDOR_LOG_LEVEL rydor::log::DEBUG
#endif

#define RYDOR_LOG_TRACE(cat, fmtstr, ...) \
    if constexpr (rydor::log::TRACE >= RYDOR_LOG_LEVEL) \
        rydor::log::write(rydor::log::TRACE, cat, fmt::format(fmtstr __VA_OPT__(,) __VA_ARGS__))

#define RYDOR_LOG_DEBUG(cat, fmtstr, ...) \
    if constexpr (rydor::log::DEBUG >= RYDOR_LOG_LEVEL) \
        rydor::log::write(rydor::log::DEBUG, cat, fmt::format(fmtstr __VA_OPT__(,) __VA_ARGS__))

#define RYDOR_LOG_INFO(cat, fmtstr, ...) \
    if constexpr (rydor::log::INFO >= RYDOR_LOG_LEVEL) \
        rydor::log::write(rydor::log::INFO, cat, fmt::format(fmtstr __VA_OPT__(,) __VA_ARGS__))

#define RYDOR_LOG_WARN(cat, fmtstr, ...) \
    if constexpr (rydor::log::WARN >= RYDOR_LOG_LEVEL) \
        rydor::log::write(rydor::log::WARN, cat, fmt::format(fmtstr __VA_OPT__(,) __VA_ARGS__))

#define RYDOR_LOG_ERROR(cat, fmtstr, ...) \
    if constexpr (rydor::log::ERROR >= RYDOR_LOG_LEVEL) \
        rydor::log::write(rydor::log::ERROR, cat, fmt::format(fmtstr __VA_OPT__(,) __VA_ARGS__))

#define RYDOR_LOG_FATAL(cat, fmtstr, ...) \
    if constexpr (rydor::log::FATAL >= RYDOR_LOG_LEVEL) \
        rydor::log::write(rydor::log::FATAL, cat, fmt::format(fmtstr __VA_OPT__(,) __VA_ARGS__))