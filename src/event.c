#include "hexspin/common.h"
#include "hexspin/event.h"

#define EVENT_EMITTER_BUCKETS 30

struct event_callback {
	event_emitter_callback_pfn callback;
	void *data;
};

struct event_key {
	uint32_t event;
	struct event_callback *callbacks;
	uint32_t n_callbacks;
	uint32_t capacity;
};

void key_grow(struct event_key *key)
{
	key->capacity *= 2;
	key->callbacks = realloc(key->callbacks, sizeof(*key->callbacks) * key->capacity);
	assert(key->callbacks);
}

void key_push_callback(struct event_key *key,
		event_emitter_callback_pfn callback, void *data)
{
	if (key->n_callbacks >= key->capacity) {
		key_grow(key);
	}
	
	key->callbacks[key->n_callbacks].callback = callback;
	key->callbacks[key->n_callbacks].data = data;
	key->n_callbacks++;
}

struct event_bucket {
	struct event_key *keys;
	uint32_t n_keys;
	uint32_t capacity;
};

void bucket_grow(struct event_bucket *bucket)
{
	bucket->capacity *= 2;
	bucket->keys = realloc(bucket->keys, sizeof(*bucket->keys) * bucket->capacity);
	assert(bucket->keys);
}

void bucket_push_event(struct event_bucket *bucket, uint32_t event)
{
	if (bucket->n_keys >= bucket->capacity) {
		bucket_grow(bucket);
	}
	
	struct event_key *key = &bucket->keys[bucket->n_keys];
	
	key->event = event;
	key->callbacks = malloc(sizeof(*key->callbacks));
	assert(key->callbacks);
	key->n_callbacks = 0;
	key->capacity = 1;
	
	bucket->n_keys++;
}

struct event_emitter {
	struct event_bucket buckets[EVENT_EMITTER_BUCKETS];
};

struct event_emitter *event_emitter_create(void)
{
	struct event_emitter *emitter = malloc(sizeof(*emitter));
	assert(emitter);
	
	for (uint32_t i = 0; i < EVENT_EMITTER_BUCKETS; i++) {
		struct event_bucket *bucket = &emitter->buckets[i];
		
		bucket->n_keys = 0;
		bucket->capacity = 1;
		bucket->keys = malloc(sizeof(*bucket->keys) * bucket->capacity);
		assert(bucket->keys);
	}
	
	return emitter;
}

void event_emitter_destroy(struct event_emitter *emitter)
{
	for (uint32_t i = 0; i < EVENT_EMITTER_BUCKETS; i++) {
		struct event_bucket *bucket = &emitter->buckets[i];
		
		for (uint32_t j = 0; j < bucket->n_keys; j++) {
			free(bucket->keys[j].callbacks);
		}
		
		free(bucket->keys);
	}
	
	free(emitter);
}

void event_emitter_emit(struct event_emitter *emitter, uint32_t event,
		void *params)
{
	uint32_t index = event % EVENT_EMITTER_BUCKETS;
	struct event_bucket *bucket = &emitter->buckets[index];
	
	bool found = false;
	uint32_t event_key_index;
	
	for (uint32_t i = 0; i < bucket->n_keys; i++) {
		if (bucket->keys[i].event == event) {
			event_key_index = i;
			found = true;
			break;
		}
	}
	
	if (!found) {
		return;
	}
	
	struct event_key *key = &bucket->keys[event_key_index];
	
	for (uint32_t i = 0; i < key->n_callbacks; i++) {
		key->callbacks[i].callback(params, key->callbacks[i].data);
	}
}

void event_emitter_on(struct event_emitter *emitter, uint32_t event,
		event_emitter_callback_pfn callback, void *data)
{
	uint32_t index = event % EVENT_EMITTER_BUCKETS;
	struct event_bucket *bucket = &emitter->buckets[index];
	
	bool found = false;
	uint32_t event_key_index;
	
	for (uint32_t i = 0; i < bucket->n_keys; i++) {
		if (bucket->keys[i].event == event) {
			event_key_index = i;
			found = true;
			break;
		}
	}
	
	if (!found) {
		event_key_index = bucket->n_keys;
		bucket_push_event(bucket, event);
	}
	
	struct event_key *key = &bucket->keys[event_key_index];
	
	key_push_callback(key, callback, data);
}
