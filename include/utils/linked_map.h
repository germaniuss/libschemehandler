#ifndef _UTILS_LINKED_MAP_H
#define _UTILS_LINKED_MAP_H

#define LINKED_MAP_MIN_LOAD 0.2f
#define LINKED_MAP_MAX_LOAD 0.95f
#define LINKED_MAP_DEFAULT_LOAD 0.75f
#define LINKED_MAP_MIN_CAPACITY 16

#include <memory.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "hash.h"

typedef struct linked_map_entry {
	void *key;
	void *value;
	struct linked_map_entry* chain_next;
	struct linked_map_entry* chain_prev;
	struct linked_map_entry* next;
	struct linked_map_entry* prev;
} linked_map_entry;

typedef struct linked_map {
	linked_map_entry **mem;
	linked_map_entry *head;
	linked_map_entry *tail;
	uint32_t cap;
	// uint32_t max_cap;
	float load_fac;
	uint32_t size;
	uint32_t max_size;
	bool oom;
	bool found;
	bool reload;
	bool circular;
	uint32_t (*hash_fn)(void *);
	bool (*cmp)(void *, void *);
} linked_map;

void linked_map_entry_ommit(linked_map *m, linked_map_entry* entry) {
	uint32_t index = m->hash_fn(entry->key) & (m->cap - 1);

	if (entry->chain_prev) entry->chain_prev->chain_next = entry->chain_next;
	else m->mem[index] = entry->chain_next;
	if (entry->chain_next) entry->chain_next->chain_prev = entry->chain_prev;

	if (entry->prev) entry->prev->next = entry->next;
	else m->head = entry->next;
	if (entry->next) entry->next->prev = entry->prev;
	else m->tail = entry->prev;

	m->size--;
}

void linked_map_entry_add(linked_map *m, linked_map_entry *entry) {
	uint32_t index = m->hash_fn(entry->key) & (m->cap - 1);
	entry->chain_next = m->mem[index];
	if (m->mem[index]) m->mem[index]->chain_prev = entry;
	m->mem[index] = entry;
	if (!m->tail) {
        m->tail = m->head = entry;
    } else {
		entry->prev = m->tail;
		entry->next = NULL;
        m->tail = m->tail->next = entry;
    } m->size++;
}

void linked_map_entry_reload(linked_map* m, linked_map_entry* entry) {
	if (m->reload && (entry->prev || entry->next)) {
		linked_map_entry_ommit(m, entry);
		linked_map_entry_add(m, entry);
	}
}

bool linked_map_assign(linked_map_entry **t, void *key, void *value)
{
	if (!(*t) && !(*t = malloc(sizeof(linked_map_entry)))) {
		return false;
	}	

	(*t)->key = key;
	(*t)->value = value;
	(*t)->prev = 0;
	(*t)->next = 0;
	(*t)->chain_next = 0;
	(*t)->chain_prev = 0;
}

bool linked_map_remap(linked_map *m)
{
	if (m->size == m->max_size) {
		return false;
	}
	
	if (m->size < (uint32_t) (m->cap * m->load_fac)) {
		return true;
	}

	linked_map_entry **new_table;
	uint32_t cap = m->cap * 2;

	if (!(new_table = calloc(cap, sizeof(linked_map_entry*)))) {
		m->oom = true;
		return false;
	}

	uint32_t mod = cap - 1;
	for (linked_map_entry* entry = m->head; entry; entry = entry->next) {
		uint32_t index = m->hash_fn(entry->key) & mod;
        entry->chain_next = new_table[index];
        new_table[index] = entry;
	}

	free(m->mem);

	m->mem = new_table;
	m->cap = cap;
	// m->max_cap = (uint32_t) (m->cap * m->load_fac);
	
	return true;
}

static inline bool linked_map_init(
	linked_map *m, uint32_t max_size, uint32_t cap, float load, bool reload, bool cicular,
	struct {
		uint32_t (*hash_fn)(void *);
		bool (*cmp)(linked_map_entry *, void *, uint32_t);
	}* funcs)
{
	float f = (load < LINKED_MAP_MIN_LOAD || load > LINKED_MAP_MAX_LOAD) ? LINKED_MAP_DEFAULT_LOAD : load;
	cap = cap < 16 ? 16 : cap;
	cap--;
	cap |= cap >> 1;
	cap |= cap >> 2;
	cap |= cap >> 4;
	cap |= cap >> 8;
	cap |= cap >> 16;
	cap++;

	if (max_size > LINKED_MAP_MIN_CAPACITY * f) {
		max_size--;
		max_size |= max_size >> 1;
		max_size |= max_size >> 2;
		max_size |= max_size >> 4;
		max_size |= max_size >> 8;
		max_size |= max_size >> 16;
		max_size++;
		max_size *= f;
	} else if (!max_size) max_size = UINT32_MAX;

	if (!m && !(m = malloc(sizeof(linked_map)))) {
		return false;
	}

	if (!(m->mem = calloc(cap, sizeof(linked_map_entry*)))) {
		m->oom = true;
		return false;
	}

	m->found = false;
	m->reload = reload;
	m->circular = cicular;
	m->head = NULL;
	m->tail = NULL;
	m->size = 0;
	m->max_size = max_size;
	m->cap = cap;
	// m->max_cap = (uint32_t) (cap * f);
	m->load_fac = f;
	m->hash_fn = funcs->hash_fn;
	m->cmp = funcs->cmp;
	return true;
}

static inline void linked_map_clear(linked_map *m)
{
	if (!m) {
		return;
	}

	for (linked_map_entry* entry = m->head; entry; entry = entry->next) {
		free(entry);
	}
}

static inline void linked_map_term(linked_map *m) 
{
	if (!m) {
		return;
	}

	linked_map_clear(m);
	free(m->mem);
}

static inline void *linked_map_put(linked_map *m, void *K, void *V)
{
	if (!m) {
		return NULL;
	}

	linked_map_entry* entry;

	for (entry = m->mem[m->hash_fn(K) & (m->cap - 1)]; entry; entry = entry->chain_next) {
		if (m->cmp(entry->key, K)) {
			m->found = true;
			void* value = entry->value;
			entry->value = V;
			linked_map_entry_reload(m, entry);
			return value;
		}
	}

	if (!linked_map_remap(m)) {
		if (m->circular) {
			entry = m->head;
			linked_map_entry_ommit(m, entry);
		} else {
			m->oom = true;
			return NULL;
		}
	}

	linked_map_assign(&entry, K, V);
	linked_map_entry_add(m, entry);
}

static inline void *linked_map_get(linked_map *m, void *key)
{
	uint32_t index;
    linked_map_entry* entry;

    if (!m) 
    {
        return NULL;
    }
    
    index = m->hash_fn(key) & (m->cap - 1);

    for (entry = m->mem[index]; entry; entry = entry->chain_next)
    {
        if (m->cmp(key, entry->key))
        {
			m->found = true;
			linked_map_entry_reload(m, entry);
            return entry->value;
        }
    }

	m->found = false;
    return NULL;
}

static inline void *linked_map_del(linked_map *m, void *key)
{
	uint32_t index;
	linked_map_entry* entry;

	if (!m) {
		return NULL;
	}

	index = m->hash_fn(key) & (m->cap - 1);
	for (entry = m->mem[index]; entry; entry = entry->chain_next) {
		if (m->cmp(entry->key, key)) {
			linked_map_entry_ommit(m, entry);
			void* value = entry->value;
			m->size--;
			free(entry);
			return value;
		}
	}
}

#define linked_map_def(name, h, c)                                  \
	struct {                                                           \
		uint32_t (*hash_fn)(void *);                                   \
		bool (*cmp)(struct linked_map_entry *, void *, uint32_t);    \
	} linked_map_config_##name = {h, c};

linked_map_def(32, hash_32, cmp_int);
linked_map_def(64, hash_64, cmp_int);
linked_map_def(str, murmurhash, cmp_str);

#endif