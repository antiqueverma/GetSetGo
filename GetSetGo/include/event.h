
#ifndef EVENT_H_
#define EVENT_H_

#include "gsg_base.h"

#define EVENT_BUFFER_SIZE  1024 // Maximum number of events that can be buffered
#define EVENT_MAX_DATA_LENGTH 256



typedef enum
{
    EVENT_ID_BUTTON_PRESS,
    EVENT_ID_TIMER_EXPIRE,
    EVENT_ID_DATA_RECEIVED,
    EVENT_ID_COUNT
} event_id_t;

typedef struct
{
    event_id_t id;
    
    uint32_t timestamp;
    uint32_t life; // Life of the event in ticks

    uint8_t flags; // Bitmask for event flags
    uint32_t source; // Source of the event, e.g., a specific module or component

    uint8_t *data;
    uint32_t length;
} event_t;

void event_publish(event_t *event);
void event_subscribe(event_id_t id);    // Subscribe to an event type
void event_unsubscribe(event_id_t id);  // Unsubscribe from an event type

event_t *event_get(event_id_t id);      // Get the latest event of a specific type
void event_wait(event_id_t id, uint32_t timeout); // Wait for an event of a specific type with a timeout

#endif // EVENT_H_
