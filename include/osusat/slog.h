/**
 * @file slog.h
 * @brief OSUSat Structured Logging System.
 *
 * A lightweight logging framework designed for embedded systems with limited
 * resources. Stores log entries in a ring buffer for deferred transmission.
 *
 * Features:
 * - Multiple severity levels (DEBUG through CRITICAL)
 * - Automatic timestamp and source line capture
 * - Component-based organization
 * - Generic flush interface via callbacks (transport-agnostic)
 * - Ring buffer storage with overwrite support
 * - Printf-style formatting
 */

#ifndef OSUSAT_SLOG_H
#define OSUSAT_SLOG_H

#include "ring_buffer.h"
#include <stdarg.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup osusat_slog Logging System
 * @brief Structured logging with deferred transmission.
 *
 * @{
 */

/**
 * @defgroup osusat_slog_types Structures
 * @ingroup osusat_slog
 * @brief Structures and types used by the Logging System.
 *
 * @{
 */

/**
 * @brief Maximum length of a single log message (including null terminator).
 */
#define OSUSAT_SLOG_MAX_MESSAGE_LEN 128

/**
 * @brief Log severity levels.
 */
typedef enum {
    OSUSAT_SLOG_DEBUG = 0,   /**< Verbose debug information */
    OSUSAT_SLOG_INFO = 1,    /**< Informational messages */
    OSUSAT_SLOG_WARN = 2,    /**< Warning conditions */
    OSUSAT_SLOG_ERROR = 3,   /**< Error conditions */
    OSUSAT_SLOG_CRITICAL = 4 /**< Critical failures */
} osusat_slog_level_t;

/**
 * @struct osusat_slog_entry_t
 * @brief Log entry header stored in ring buffer.
 *
 * Each log entry consists of this header followed by a null-terminated
 * message string.
 */
typedef struct __attribute__((packed)) {
    uint32_t timestamp_ms; /**< Millisecond timestamp when log was created */
    uint8_t level;         /**< Severity level (osusat_slog_level_t) */
    uint8_t component_id;  /**< Subsystem/component identifier */
    uint16_t line;         /**< Source code line number */
    uint16_t message_len;  /**< Length of message string (excluding null) */
} osusat_slog_entry_t;

/**
 * @brief Timestamp provider function type.
 *
 * @return Current timestamp in milliseconds.
 */
typedef uint32_t (*osusat_slog_timestamp_fn_t)(void);

/**
 * @brief Flush callback function type.
 *
 * Called during osusat_slog_flush() for each log entry. Implementation should
 * handle framing, packetization, and transmission as needed.
 *
 * @param[in] entry    Pointer to log entry header.
 * @param[in] message  Pointer to null-terminated message string.
 * @param[in] ctx User context pointer passed during flush.
 */
typedef void (*osusat_slog_flush_fn_t)(const osusat_slog_entry_t *entry,
                                       const char *message, void *ctx);

/** @} */ // end osusat_slog_types

/**
 * @defgroup osusat_slog_api Public API
 * @ingroup osusat_slog
 * @brief External interface for interacting with the Logging System.
 *
 * @{
 */

/**
 * @brief Initialize the logging subsystem.
 *
 * Sets up the log storage buffer and configures timestamp and filtering.
 *
 * @param[in] log_buf      Ring buffer for storing log entries. Should be
 *                         initialized with overwrite mode enabled.
 * @param[in] timestamp_fn Function to get current timestamp in milliseconds
 *                         (e.g., HAL_GetTick). Can be NULL if timestamps
 *                         are not needed.
 * @param[in] min_level    Minimum log level to record. Logs below this level
 *                         are silently discarded.
 */
void osusat_slog_init(osusat_ring_buffer_t *log_buf,
                      osusat_slog_timestamp_fn_t timestamp_fn,
                      osusat_slog_level_t min_level);

/**
 * @brief Change the minimum logging level.
 *
 * Modifies the minimum log level to record during runtime. Useful for
 * maintenance mode
 *
 * @param[in] min_level    Minimum log level to record. Logs below this level
 *                         are silently discarded.
 */
void osusat_slog_change_min_log_level(osusat_slog_level_t min_level);

/**
 * @brief Write a log entry (internal function).
 *
 * Formats the log message and stores it in the ring buffer along with
 * metadata. Use the osusat_slog() macro instead of calling this directly.
 *
 * @param[in] level        Severity level of the log.
 * @param[in] component_id Component/subsystem identifier.
 * @param[in] line         Source code line number.
 * @param[in] fmt          Printf-style format string.
 * @param[in] ...          Variable arguments for format string.
 */
void osusat_slog_write_internal(osusat_slog_level_t level, uint8_t component_id,
                                uint16_t line, const char *fmt, ...);

/**
 * @brief Flush all pending log entries.
 *
 * Drains the ring buffer and calls the provided callback for each log entry.
 * The callback is responsible for transmission/packetization.
 *
 * @param[in] flush_fn Callback function to handle each log entry.
 * @param[in] ctx User context passed to callback. Can be NULL.
 *
 * @return Number of log entries flushed.
 */
size_t osusat_slog_flush(osusat_slog_flush_fn_t flush_fn, void *ctx);

/**
 * @brief Get number of log entries currently buffered.
 *
 * @return Approximate count of pending log entries.
 *
 * @note This is an estimate based on buffer usage and average entry size.
 */
size_t osusat_slog_pending_count(void);

/**
 * @brief Main logging macro.
 *
 * Automatically captures the source line number and formats the message.
 *
 * @param[in] level     Severity level (osusat_slog_level_t).
 * @param[in] component Component identifier (uint8_t).
 * @param[in] fmt       Printf-style format string.
 * @param[in] ...       Variable arguments for format string.
 *
 * Example:
 * @code
 * osusat_slog(osusat_slog_WARN, EPS_BATTERY, "Voltage low: %dmV", voltage);
 * @endcode
 */
#define OSUSAT_SLOG(level, component, fmt, ...)                                \
    osusat_slog_write_internal(level, component, __LINE__, fmt, ##__VA_ARGS__)

/**
 * @brief Log a DEBUG level message.
 *
 * @param[in] component Component identifier.
 * @param[in] fmt       Printf-style format string.
 * @param[in] ...       Variable arguments.
 */
#define LOG_DEBUG(component, fmt, ...)                                         \
    OSUSAT_SLOG(OSUSAT_SLOG_DEBUG, component, fmt, ##__VA_ARGS__)

/**
 * @brief Log an INFO level message.
 *
 * @param[in] component Component identifier.
 * @param[in] fmt       Printf-style format string.
 * @param[in] ...       Variable arguments.
 */
#define LOG_INFO(component, fmt, ...)                                          \
    OSUSAT_SLOG(OSUSAT_SLOG_INFO, component, fmt, ##__VA_ARGS__)

/**
 * @brief Log a WARN level message.
 *
 * @param[in] component Component identifier.
 * @param[in] fmt       Printf-style format string.
 * @param[in] ...       Variable arguments.
 */
#define LOG_WARN(component, fmt, ...)                                          \
    OSUSAT_SLOG(OSUSAT_SLOG_WARN, component, fmt, ##__VA_ARGS__)

/**
 * @brief Log an ERROR level message.
 *
 * @param[in] component Component identifier.
 * @param[in] fmt       Printf-style format string.
 * @param[in] ...       Variable arguments.
 */
#define LOG_ERROR(component, fmt, ...)                                         \
    OSUSAT_SLOG(OSUSAT_SLOG_ERROR, component, fmt, ##__VA_ARGS__)

/**
 * @brief Log a CRITICAL level message.
 *
 * @param[in] component Component identifier.
 * @param[in] fmt       Printf-style format string.
 * @param[in] ...       Variable arguments.
 */
#define LOG_CRITICAL(component, fmt, ...)                                      \
    OSUSAT_SLOG(OSUSAT_SLOG_CRITICAL, component, fmt, ##__VA_ARGS__)

/** @} */ // end osusat_slog_api

/** @} */ // end osusat_slog

#ifdef __cplusplus
}
#endif

#endif // OSUSAT_SLOG_H
