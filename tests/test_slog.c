#include "osusat/ring_buffer.h"
#include "osusat/slog.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

// test component IDs
#define TEST_COMPONENT_A 0x10
#define TEST_COMPONENT_B 0x20

// mock timestamp function
static uint32_t mock_timestamp = 0;
static uint32_t mock_get_timestamp(void) { return mock_timestamp; }

// flush callback context for testing
typedef struct {
    size_t call_count;
    osusat_slog_entry_t last_entry;
    char last_message[128];
} test_flush_context_t;

static void reset_test_state(void) { mock_timestamp = 0; }

static void test_flush_callback(const osusat_slog_entry_t *entry,
                                const char *message, void *user_ctx) {
    test_flush_context_t *ctx = (test_flush_context_t *)user_ctx;

    ctx->call_count++;
    ctx->last_entry = *entry;
    strncpy(ctx->last_message, message, sizeof(ctx->last_message) - 1);
    ctx->last_message[sizeof(ctx->last_message) - 1] = '\0';

    // validate entry before printing
    if (entry->level > OSUSAT_SLOG_CRITICAL) {
        printf("[CORRUPTED ENTRY - skipping display]\n");
        return;
    }

    const char *level_str;
    switch (entry->level) {
    case OSUSAT_SLOG_DEBUG:
        level_str = "DEBUG";
        break;
    case OSUSAT_SLOG_INFO:
        level_str = "INFO ";
        break;
    case OSUSAT_SLOG_WARN:
        level_str = "WARN ";
        break;
    case OSUSAT_SLOG_ERROR:
        level_str = "ERROR";
        break;
    case OSUSAT_SLOG_CRITICAL:
        level_str = "CRIT ";
        break;
    default:
        level_str = "?????";
        break;
    }

    printf("[%10u] [%s] [0x%02X:%04u] %s\n", entry->timestamp_ms, level_str,
           entry->component_id, entry->line, message);
}

static void test_init(void) {
    reset_test_state();

    uint8_t storage[256];
    osusat_ring_buffer_t rb;

    osusat_ring_buffer_init(&rb, storage, sizeof(storage), true);
    osusat_slog_init(&rb, mock_get_timestamp, OSUSAT_SLOG_INFO);

    // should be initialized and empty
    assert(osusat_slog_pending_count() == 0);
}

static void test_basic_logging(void) {
    reset_test_state();

    uint8_t storage[512];
    osusat_ring_buffer_t rb;

    osusat_ring_buffer_init(&rb, storage, sizeof(storage), true);
    osusat_slog_init(&rb, mock_get_timestamp, OSUSAT_SLOG_DEBUG);

    mock_timestamp = 1000;

    // write a simple log
    OSUSAT_SLOG(OSUSAT_SLOG_INFO, TEST_COMPONENT_A, "Test message");

    // should have one entry
    assert(osusat_slog_pending_count() > 0);

    // flush and verify
    test_flush_context_t ctx = {0};
    size_t flushed = osusat_slog_flush(test_flush_callback, &ctx);

    assert(flushed == 1);
    assert(ctx.call_count == 1);
    assert(ctx.last_entry.timestamp_ms == 1000);
    assert(ctx.last_entry.level == OSUSAT_SLOG_INFO);
    assert(ctx.last_entry.component_id == TEST_COMPONENT_A);
    assert(strcmp(ctx.last_message, "Test message") == 0);

    // buffer should now be empty
    assert(osusat_slog_pending_count() == 0);
}

static void test_formatted_logging(void) {
    reset_test_state();

    uint8_t storage[512];
    osusat_ring_buffer_t rb;

    osusat_ring_buffer_init(&rb, storage, sizeof(storage), true);
    osusat_slog_init(&rb, mock_get_timestamp, OSUSAT_SLOG_DEBUG);

    mock_timestamp = 2000;

    // write formatted log
    int voltage = 3300;
    OSUSAT_SLOG(OSUSAT_SLOG_WARN, TEST_COMPONENT_B, "Voltage: %dmV", voltage);

    // flush and verify
    test_flush_context_t ctx = {0};
    osusat_slog_flush(test_flush_callback, &ctx);

    assert(ctx.call_count == 1);
    assert(ctx.last_entry.timestamp_ms == 2000);
    assert(ctx.last_entry.level == OSUSAT_SLOG_WARN);
    assert(ctx.last_entry.component_id == TEST_COMPONENT_B);
    assert(strcmp(ctx.last_message, "Voltage: 3300mV") == 0);
}

static void test_level_filtering(void) {
    reset_test_state();

    uint8_t storage[512];
    osusat_ring_buffer_t rb;

    osusat_ring_buffer_init(&rb, storage, sizeof(storage), true);
    osusat_slog_init(&rb, mock_get_timestamp, OSUSAT_SLOG_WARN);

    // these should be filtered out
    LOG_DEBUG(TEST_COMPONENT_A, "Debug message");
    LOG_INFO(TEST_COMPONENT_A, "Info message");

    // these should be logged
    LOG_WARN(TEST_COMPONENT_A, "Warning message");
    LOG_ERROR(TEST_COMPONENT_A, "Error message");
    LOG_CRITICAL(TEST_COMPONENT_A, "Critical message");

    // should only have 3 entries
    test_flush_context_t ctx = {0};
    size_t flushed = osusat_slog_flush(test_flush_callback, &ctx);

    assert(flushed == 3);
}

static void test_multiple_entries(void) {
    reset_test_state();

    uint8_t storage[1024];
    osusat_ring_buffer_t rb;

    osusat_ring_buffer_init(&rb, storage, sizeof(storage), true);
    osusat_slog_init(&rb, mock_get_timestamp, OSUSAT_SLOG_DEBUG);

    // log multiple entries
    for (int i = 0; i < 5; i++) {
        mock_timestamp = 1000 + i * 100;
        OSUSAT_SLOG(OSUSAT_SLOG_INFO, TEST_COMPONENT_A, "Entry %d", i);
    }

    // flush and verify count
    test_flush_context_t ctx = {0};
    size_t flushed = osusat_slog_flush(test_flush_callback, &ctx);

    assert(flushed == 5);
    assert(ctx.call_count == 5);

    // last entry should be "Entry 4"
    assert(strcmp(ctx.last_message, "Entry 4") == 0);
    assert(ctx.last_entry.timestamp_ms == 1400);
}

static void test_convenience_macros(void) {
    reset_test_state();

    uint8_t storage[1024];
    osusat_ring_buffer_t rb;

    osusat_ring_buffer_init(&rb, storage, sizeof(storage), true);
    osusat_slog_init(&rb, mock_get_timestamp, OSUSAT_SLOG_DEBUG);

    // test all convenience macros
    LOG_DEBUG(TEST_COMPONENT_A, "Debug");
    LOG_INFO(TEST_COMPONENT_A, "Info");
    LOG_WARN(TEST_COMPONENT_A, "Warn");
    LOG_ERROR(TEST_COMPONENT_A, "Error");
    LOG_CRITICAL(TEST_COMPONENT_A, "Critical");

    // should have 5 entries
    test_flush_context_t ctx = {0};
    size_t flushed = osusat_slog_flush(test_flush_callback, &ctx);

    assert(flushed == 5);
}

static void test_message_truncation(void) {
    reset_test_state();

    uint8_t storage[1024];
    osusat_ring_buffer_t rb;

    osusat_ring_buffer_init(&rb, storage, sizeof(storage), true);
    osusat_slog_init(&rb, mock_get_timestamp, OSUSAT_SLOG_DEBUG);

    // create a very long message (> 128 chars)
    char long_msg[200];
    memset(long_msg, 'A', sizeof(long_msg) - 1);
    long_msg[sizeof(long_msg) - 1] = '\0';

    OSUSAT_SLOG(OSUSAT_SLOG_INFO, TEST_COMPONENT_A, "%s", long_msg);

    // flush and verify truncation
    test_flush_context_t ctx = {0};
    osusat_slog_flush(test_flush_callback, &ctx);

    assert(ctx.call_count == 1);
    // message should be truncated to 127 chars (128 - 1 for null terminator)
    assert(strlen(ctx.last_message) == 127);
}

static void test_change_min_level(void) {
    reset_test_state();

    uint8_t storage[512];
    osusat_ring_buffer_t rb;

    osusat_ring_buffer_init(&rb, storage, sizeof(storage), true);
    osusat_slog_init(&rb, mock_get_timestamp, OSUSAT_SLOG_INFO);

    // debug should be filtered
    LOG_DEBUG(TEST_COMPONENT_A, "Debug 1");
    LOG_INFO(TEST_COMPONENT_A, "Info 1");

    // change level to DEBUG
    osusat_slog_change_min_log_level(OSUSAT_SLOG_DEBUG);

    // now debug should work
    LOG_DEBUG(TEST_COMPONENT_A, "Debug 2");
    LOG_INFO(TEST_COMPONENT_A, "Info 2");

    // should have 3 entries (Info 1, Debug 2, Info 2)
    test_flush_context_t ctx = {0};
    size_t flushed = osusat_slog_flush(test_flush_callback, &ctx);

    assert(flushed == 3);
}

static void test_null_timestamp_function(void) {
    reset_test_state();

    uint8_t storage[512];
    osusat_ring_buffer_t rb;

    osusat_ring_buffer_init(&rb, storage, sizeof(storage), true);
    osusat_slog_init(&rb, NULL, OSUSAT_SLOG_DEBUG);

    OSUSAT_SLOG(OSUSAT_SLOG_INFO, TEST_COMPONENT_A, "No timestamp");

    // flush and verify timestamp is 0
    test_flush_context_t ctx = {0};
    osusat_slog_flush(test_flush_callback, &ctx);

    assert(ctx.call_count == 1);
    assert(ctx.last_entry.timestamp_ms == 0);
}

static void test_overwrite_mode(void) {
    reset_test_state();

    // small buffer to force overwrite
    uint8_t storage[128];
    osusat_ring_buffer_t rb;

    osusat_ring_buffer_init(&rb, storage, sizeof(storage), true);
    osusat_slog_init(&rb, mock_get_timestamp, OSUSAT_SLOG_DEBUG);

    // fill buffer with many entries
    for (int i = 0; i < 20; i++) {
        OSUSAT_SLOG(OSUSAT_SLOG_INFO, TEST_COMPONENT_A, "Entry %d", i);
    }

    // should have some entries (oldest ones overwritten)
    test_flush_context_t ctx = {0};
    size_t flushed = osusat_slog_flush(test_flush_callback, &ctx);

    // should have flushed some entries (not all 20 due to small buffer)
    assert(flushed > 0);
    assert(flushed < 20);

    // last message should be from recent entries
    assert(strstr(ctx.last_message, "Entry") != NULL);
}

static void test_empty_flush(void) {
    reset_test_state();

    uint8_t storage[512];
    osusat_ring_buffer_t rb;

    osusat_ring_buffer_init(&rb, storage, sizeof(storage), true);
    osusat_slog_init(&rb, mock_get_timestamp, OSUSAT_SLOG_DEBUG);

    // flush empty buffer
    test_flush_context_t ctx = {0};
    size_t flushed = osusat_slog_flush(test_flush_callback, &ctx);

    assert(flushed == 0);
    assert(ctx.call_count == 0);
}

static void test_null_flush_callback(void) {
    reset_test_state();

    uint8_t storage[512];
    osusat_ring_buffer_t rb;

    osusat_ring_buffer_init(&rb, storage, sizeof(storage), true);
    osusat_slog_init(&rb, mock_get_timestamp, OSUSAT_SLOG_DEBUG);

    OSUSAT_SLOG(OSUSAT_SLOG_INFO, TEST_COMPONENT_A, "Test");

    // flush with NULL callback should return 0
    size_t flushed = osusat_slog_flush(NULL, NULL);
    assert(flushed == 0);
}

int main(void) {
    test_init();
    test_basic_logging();
    test_formatted_logging();
    test_level_filtering();
    test_multiple_entries();
    test_convenience_macros();
    test_message_truncation();
    test_change_min_level();
    test_null_timestamp_function();
    test_overwrite_mode();
    test_empty_flush();
    test_null_flush_callback();

    printf("All slog tests passed.\n");
    return 0;
}
