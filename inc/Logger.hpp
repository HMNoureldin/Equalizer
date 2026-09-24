/**
 * @file
 * @author H.Noureldin <heshamnoureldin2017@gmail.com>
 * @brief Log levels and convenience functions for
 * standard-error logging.
 */

#pragma once

#include <string_view>

/** @brief Simple logger for single-threaded application
 * use. */
namespace Logger
{
/** @brief Severity in increasing order; messages below the
 * threshold are hidden. */
enum class Level {
    Debug, ///< Development details.
    Info,  ///< Normal application status.
    Warn,  ///< Recoverable unexpected conditions.
    Error  ///< Operation failures.
};

/**
 * @brief Set the process-wide minimum severity (initially
 * Info).
 * @param level Lowest severity to emit.
 */
void setLevel(Level level);
/**
 * @brief Write an enabled message to standard error with a
 * timestamp, level, source location, and newline.
 * @param level Message severity.
 * @param message Text consumed immediately; no copy is
 * retained.
 * @param file Source file containing the log call.
 * @param line Source line containing the log call.
 * @note Timestamps use local time.
 * @note Logging and level changes are not synchronized
 * across threads.
 */
void log(Level level, std::string_view message, std::string_view file,
         int line);
} // namespace Logger

/**
 * @brief Log a DEBUG message if enabled.
 * @param message Text to log.
 */
#define LOG_DEBUG(message)                                                     \
    ::Logger::log(::Logger::Level::Debug, (message), __FILE__, __LINE__)

/**
 * @brief Log a INFO message if enabled.
 * @param message Text to log.
 */
#define LOG_INFO(message)                                                      \
    ::Logger::log(::Logger::Level::Info, (message), __FILE__, __LINE__)

/**
 * @brief Log a WARN message if enabled.
 * @param message Text to log.
 */
#define LOG_WARN(message)                                                      \
    ::Logger::log(::Logger::Level::Warn, (message), __FILE__, __LINE__)

/**
 * @brief Log a ERROR message if enabled.
 * @param message Text to log.
 */
#define LOG_ERROR(message)                                                     \
    ::Logger::log(::Logger::Level::Error, (message), __FILE__, __LINE__)
