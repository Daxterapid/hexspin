#ifndef EVENT_H
#define EVENT_H

#include "hexspin/common.h"

typedef void (*event_emitter_callback_pfn)(void *, void *);

struct event_emitter;

struct event_emitter *event_emitter_create(void);
void event_emitter_destroy(struct event_emitter *emitter);

void event_emitter_emit(struct event_emitter *emitter, uint32_t event,
		void *params);

void event_emitter_on(struct event_emitter *emitter, uint32_t event,
		event_emitter_callback_pfn callback, void *data);

#endif
