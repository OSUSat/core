/**
 * @file event_bus.h
 * @brief Generic Publish/Subscribe Mechanism.
 */

#ifndef OSUSAT_EVENT_BUS_H
#define OSUSAT_EVENT_BUS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup osusat_event_bus Event Bus
 * @brief Generic Publish/Subscribe Mechanism.
 *
 * @{
 */

/**
 * @defgroup osusat_event_bus_types Structures
 * @ingroup osusat_event_bus
 * @brief Structures used by the Event Bus.
 *
 * @{
 */

/**
 * @brief Max payload size in bytes.
 * Kept small (8 bytes) to fit primitives (double, uint64_t) or pointers.
 */
#define OSUSAT_EVENT_MAX_PAYLOAD 32

/**
 * @brief Maximum number of active subscriptions allowed system-wide.
 * Adjust based on RAM constraints.
 */
#define OSUSAT_EVENT_MAX_SUBSCRIBERS 128

/**
 * @brief Event Identifier Type (32-bit).
 *
 * Constructed using OSUSAT_BUILD_EVENT_ID().
 */
typedef uint32_t osusat_event_id_t;

/**
 * @brief Helper to build a unique ID from a Service UID and Local Code.
 *
 * @param svc_uid  Unique 16-bit Service Identifier (e.g. 0xBA77 for Batt).
 * @param code     Local enum value (0-65535).
 */
#define OSUSAT_BUILD_EVENT_ID(svc_uid, code)                                   \
    (((uint32_t)(svc_uid) << 16) | ((uint32_t)(code) & 0xFFFF))

/**
 * @brief Helper to extract the Service UID from an Event ID.
 */
#define OSUSAT_GET_SERVICE_UID(event_id) ((uint16_t)((event_id) >> 16))

/**
 * @brief Helper to extract the Local Code from an Event ID.
 */
#define OSUSAT_GET_LOCAL_CODE(event_id) ((uint16_t)((event_id) & 0xFFFF))

/**
 * @brief Reserved UID for Core System Events.
 */
#define OSUSAT_SERVICE_UID_SYSTEM 0x0000

/**
 * @brief System Event Codes.
 */
typedef enum {
    SYSTEM_SYSTICK = 1, /**< Periodic heartbeat (e.g. 100Hz) */
    SYSTEM_INIT_DONE,   /**< All services initialized */
    SYSTEM_HEARTBEAT    /**< Heartbeat event for health monitoring */
} osusat_system_code_t;

#define EVENT_SYSTICK                                                          \
    OSUSAT_BUILD_EVENT_ID(OSUSAT_SERVICE_UID_SYSTEM, SYSTEM_SYSTICK)
#define EVENT_SYSTEM_INIT                                                      \
    OSUSAT_BUILD_EVENT_ID(OSUSAT_SERVICE_UID_SYSTEM, SYSTEM_INIT_DONE)

/**
 * @struct osusat_event_t
 * @brief The event object stored in the queue.
 */
typedef struct {
    osusat_event_id_t id;                      /**< Composite Event ID */
    uint8_t payload[OSUSAT_EVENT_MAX_PAYLOAD]; /**< Data copy */
    uint8_t payload_len;                       /**< Valid bytes in payload */
} osusat_event_t;

/**
 * @brief Event Handler Callback definition.
 *
 * @param[in] event Pointer to the event data.
 * @param[in] ctx   User context pointer registered during subscription.
 */
typedef void (*osusat_event_handler_t)(const osusat_event_t *event, void *ctx);

/** @} */ // end osusat_event_bus_types

/**
 * @defgroup osusat_event_bus_api Public API
 * @ingroup osusat_event_bus
 * @brief External interface for interacting with the Event Bus.
 *
 * @{
 */

/**
 * @brief Initialize the Event Bus.
 *
 * Configures the internal ring buffer and clears subscribers.
 *
 * @param[in] queue_storage  Pointer to allocated array of event structs.
 * @param[in] queue_capacity Number of elements in the storage array.
 */
void osusat_event_bus_init(osusat_event_t *queue_storage,
                           size_t queue_capacity);

/**
 * @brief Subscribe to an event.
 *
 * Registers a callback to be invoked when the specific Event ID occurs.
 *
 * @param[in] event_id The Composite ID to listen for.
 * @param[in] handler  The function to call.
 * @param[in] ctx      Optional context pointer passed to the handler.
 *
 * @retval true  Subscription added successfully.
 * @retval false Subscriber table full (increase OSUSAT_EVENT_MAX_SUBSCRIBERS).
 */
bool osusat_event_bus_subscribe(osusat_event_id_t event_id,
                                osusat_event_handler_t handler, void *ctx);

/**
 * @brief Publish an event to the bus.
 *
 * Copies the event data into the queue. Safe to call from ISRs.
 *
 * @param[in] event_id The Composite ID of the event.
 * @param[in] payload  Pointer to data to copy (can be NULL).
 * @param[in] len      Length of data (must be <= OSUSAT_EVENT_MAX_PAYLOAD).
 *
 * @retval true  Event queued successfully.
 * @retval false Queue full (Event Dropped!).
 */
bool osusat_event_bus_publish(osusat_event_id_t event_id, const void *payload,
                              size_t len);

/**
 * @brief Process the Event Queue.
 *
 * Pops all pending events and executes their subscribers.
 * @warning Must be called from the main loop (Thread Mode).
 */
void osusat_event_bus_process(void);

/** @} */ // end osusat_event_bus_api

#ifdef __cplusplus
}
#endif

#endif // OSUSAT_EVENT_BUS_H
