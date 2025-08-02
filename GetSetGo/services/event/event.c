
#include "event.h"

static event_t event_buffer[EVENT_BUFFER_SIZE];
static uint32_t event_count = 0;

static event_t event_queue[];
