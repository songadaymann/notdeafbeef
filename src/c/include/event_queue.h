#ifndef EVENT_QUEUE_H
#define EVENT_QUEUE_H

#include <stdint.h>

/* Event types for the segment composer */
typedef enum {
    EVT_KICK = 0,
    EVT_SNARE,
    EVT_HAT,
    EVT_MELODY,
    EVT_MID,     /* generic mid-range note with 7 possible waveforms */
    EVT_FM_BASS,
    EVT_COUNT
} event_type_t;

typedef struct {
    uint32_t time;   /* sample index at which to trigger */
    uint8_t  type;   /* event_type_t */
    uint8_t  aux;    /* optional small parameter (e.g. preset/freq index) */
} event_t;

#define MAX_EVENTS 512  /* adjust if needed */

typedef struct {
    event_t events[MAX_EVENTS];
    uint32_t count;
} event_queue_t;

static inline void eq_init(event_queue_t *q){ q->count = 0; }
static inline void eq_push(event_queue_t *q, uint32_t time, uint8_t type, uint8_t aux){
    if(q->count < MAX_EVENTS){
        q->events[q->count++] = (event_t){time, type, aux};
    }
}

#endif /* EVENT_QUEUE_H */ 