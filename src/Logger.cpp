/**
 * @file
 * @author H.Noureldin <heshamnoureldin2017@gmail.com>
 * @brief Process-wide log filtering and labeled message
 * output.
 */

#include "Logger.hpp"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace Logger
{
namespace
{
/// Process-wide severity threshold.
Level minimumLevel = Level::Info;

/**
 * @brief Obtain a printable severity label.
 * @param level Severity to label.
 * @return Uppercase name, or UNKNOWN for an unrecognized
 * value.
 */
std::string_view levelName(Level level)
{
    switch (level) {
    case Level::Debug:
        return "DEBUG";
    case Level::Info:
        return "INFO";
    case Level::Warn:
        return "WARN";
    case Level::Error:
        return "ERROR";
    }
    return "UNKNOWN";
}
} // namespace

void setLevel(Level level)
{
    minimumLevel = level;
}

void log(Level level, std::string_view message, std::string_view file, int line)
{
    if (level < minimumLevel) {
        return;
    }
    const auto now = std::chrono::system_clock::now();
    const auto time = std::chrono::system_clock::to_time_t(now);
    const auto* localTime = std::localtime(&time);
    const auto separator = file.find_last_of("/\\");
    const auto filename =
        file.substr(separator == std::string_view::npos ? 0 : separator + 1);

    std::ostringstream output;
    output << '[';
    if (localTime) {
        output << std::put_time(localTime, "%Y-%m-%d %H:%M:%S");
    } else {
        output << "time unavailable";
    }
    output << "] [" << levelName(level) << "] [" << filename << ':' << line
           << "] " << message << '\n';
    std::cerr << output.str();
}
} // namespace Logger
