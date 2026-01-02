#include "osusat/slog.h"
#include "osusat/ring_buffer.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

// === critical section helpers ===

#if defined(__arm__)

static inline uint32_t __get_PRIMASK(void) {
    uint32_t result;
    __asm volatile("MRS %0, primask" : "=r"(result)::"memory");
    return result;
}

static inline void __set_PRIMASK(uint32_t priMask) {
    __asm volatile("MSR primask, %0" : : "r"(priMask) : "memory");
    __asm volatile("" ::: "memory");
}

// disable interrupts and save state
#define ENTER_CRITICAL()                                                       \
    uint32_t primask = __get_PRIMASK();                                        \
    __asm volatile("cpsid i" : : : "memory")

// restore interrupts
#define EXIT_CRITICAL() __set_PRIMASK(primask)

#else

#define ENTER_CRITICAL()
#define EXIT_CRITICAL()

#endif

#define MAX_LOG_MESSAGE_LEN 128

static osusat_ring_buffer_t *g_log_buffer = NULL;
static osusat_slog_timestamp_fn_t g_timestamp_fn = NULL;
static osusat_slog_level_t g_min_level = OSUSAT_SLOG_DEBUG;

void osusat_slog_init(osusat_ring_buffer_t *log_buf,
                      osusat_slog_timestamp_fn_t timestamp_fn,
                      osusat_slog_level_t min_level) {
    g_log_buffer = log_buf;
    g_timestamp_fn = timestamp_fn;
    g_min_level = min_level;
}

void osusat_slog_write_internal(osusat_slog_level_t level, uint8_t component_id,
                                uint16_t line, const char *fmt, ...) {
    // filter by level
    if (level < g_min_level || g_log_buffer == NULL) {
        return;
    }

    // format message
    char message[MAX_LOG_MESSAGE_LEN];

    va_list args;

    va_start(args, fmt);
    int len = vsnprintf(message, sizeof(message), fmt, args);
    va_end(args);

    if (len < 0) {
        return; // formatting error
    }

    if ((size_t)len >= sizeof(message)) {
        len = sizeof(message) - 1; // truncated
    }

    // build entry header
    osusat_slog_entry_t entry = {.timestamp_ms =
                                     g_timestamp_fn ? g_timestamp_fn() : 0,
                                 .level = level,
                                 .component_id = component_id,
                                 .line = line,
                                 .message_len = (uint16_t)len};

    ENTER_CRITICAL(); // prevent ISR corruption

    // write to ring buffer: header + message + null terminator
    const uint8_t *header_bytes = (const uint8_t *)&entry;

    for (size_t i = 0; i < sizeof(entry); i++) {
        if (!osusat_ring_buffer_push(g_log_buffer, header_bytes[i])) {
            // ring buffer full, should not happen with overwrite mode
            EXIT_CRITICAL();
            return;
        }
    }

    // write message including null terminator
    for (int i = 0; i <= len; i++) {
        if (!osusat_ring_buffer_push(g_log_buffer, (uint8_t)message[i])) {
            EXIT_CRITICAL();
            return;
        }
    }

    EXIT_CRITICAL();
}

void osusat_slog_change_min_log_level(osusat_slog_level_t min_level) {
    g_min_level = min_level;
}

size_t osusat_slog_flush(osusat_slog_flush_fn_t flush_fn, void *ctx) {
    if (g_log_buffer == NULL || flush_fn == NULL) {
        return 0;
    }

    size_t count = 0;

    while (true) {
        ENTER_CRITICAL();

        bool empty = osusat_ring_buffer_empty(g_log_buffer);

        if (empty) {
            EXIT_CRITICAL();
            break;
        }

        // read header
        osusat_slog_entry_t entry;
        uint8_t *header_bytes = (uint8_t *)&entry;

        for (size_t i = 0; i < sizeof(entry); i++) {
            if (!osusat_ring_buffer_pop(g_log_buffer, &header_bytes[i])) {
                EXIT_CRITICAL();
                return count;
            }
        }

        // read message
        char message[MAX_LOG_MESSAGE_LEN];
        size_t msg_idx = 0;

        for (msg_idx = 0; msg_idx <= entry.message_len; msg_idx++) {
            if (msg_idx >= sizeof(message)) {
                break;
            }

            if (!osusat_ring_buffer_pop(g_log_buffer,
                                        (uint8_t *)&message[msg_idx])) {
                message[msg_idx] = '\0';
                break;
            }

            if (message[msg_idx] == '\0') {
                break;
            }
        }

        EXIT_CRITICAL();

        message[sizeof(message) - 1] = '\0';

        // call user callback
        flush_fn(&entry, message, ctx);
        count++;
    }

    return count;
}

size_t osusat_slog_pending_count(void) {
    if (g_log_buffer == NULL) {
        return 0;
    }

    // approximate count (header + message + null)
    size_t bytes = osusat_ring_buffer_size(g_log_buffer);

    // very rough estimate: assume average entry is ~40 bytes
    return bytes / 40;
}
