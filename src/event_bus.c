/**
 * @file osusat_event_bus.c
 * @brief Event Bus Implementation (Ring Buffer + Dispatcher)
 */

#include "osusat/event_bus.h"
#include <stdint.h>
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

// === internal types ===

typedef struct {
    osusat_event_id_t id;
    osusat_event_handler_t handler;
    void *ctx;
} subscriber_entry_t;

static struct {
    // subscriber registry
    subscriber_entry_t subs[OSUSAT_EVENT_MAX_SUBSCRIBERS];
    size_t sub_count;

    // event queue (ring buffer)
    osusat_event_t *queue;
    size_t capacity;

    volatile size_t head;
    volatile size_t tail;
} bus;

// === api Implementation ===

void osusat_event_bus_init(osusat_event_t *queue_storage,
                           size_t queue_capacity) {
    memset(&bus, 0, sizeof(bus));

    if (queue_storage != NULL && queue_capacity > 0) {
        bus.queue = queue_storage;
        bus.capacity = queue_capacity;
    }
}

bool osusat_event_bus_subscribe(osusat_event_id_t event_id,
                                osusat_event_handler_t handler, void *ctx) {
    if (bus.sub_count >= OSUSAT_EVENT_MAX_SUBSCRIBERS) {
        return false;
    }

    if (handler == NULL) {
        return false;
    }

    bus.subs[bus.sub_count].id = event_id;
    bus.subs[bus.sub_count].handler = handler;
    bus.subs[bus.sub_count].ctx = ctx;

    bus.sub_count++;

    return true;
}

bool osusat_event_bus_publish(osusat_event_id_t event_id, const void *payload,
                              size_t len) {
    if (bus.queue == NULL) {
        return false;
    }

    bool success = false;

    if (len > OSUSAT_EVENT_MAX_PAYLOAD) { // truncate
        len = OSUSAT_EVENT_MAX_PAYLOAD;
    }

    // prevent ISR from interrupting here while we move the head pointer
    ENTER_CRITICAL();

    size_t next_head = (bus.head + 1) % bus.capacity;

    if (next_head != bus.tail) {
        osusat_event_t *event = &bus.queue[bus.head];

        event->id = event_id;
        event->payload_len = (uint8_t)len;

        if (payload && len > 0) {
            memcpy(event->payload, payload, len);
        } else {
            memset(event->payload, 0, OSUSAT_EVENT_MAX_PAYLOAD);
        }

        bus.head = next_head;
        success = true;
    }

    EXIT_CRITICAL();

    return success;
}

void osusat_event_bus_process(void) {
    if (bus.queue == NULL) {
        return;
    }

    while (bus.tail != bus.head) { // process entire queue until empty
        const osusat_event_t *current_event = &bus.queue[bus.tail];

        for (size_t i = 0; i < bus.sub_count; i++) {
            if (bus.subs[i].id == current_event->id) {
                bus.subs[i].handler(current_event, bus.subs[i].ctx);
            }
        }

        bus.tail = (bus.tail + 1) % bus.capacity;
    }
}
