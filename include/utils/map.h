#ifndef _UTILS_MAP_H
#define _UTILS_MAP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <memory.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include "hash.h"

#define map_calloc calloc
#define map_free free

#define map_dec_strkey(name, K, V)                                          \
	struct map_item_##name {                                            \
		K key;                                                         \
		V value;                                                       \
		uint32_t hash;                                                 \
	};                                                                     \
                                                                               \
	map_of(name, K, V)

#define map_dec_scalar(name, K, V)                                          \
	struct map_item_##name {                                            \
		K key;                                                         \
		V value;                                                       \
	};                                                                     \
                                                                               \
	map_of(name, K, V)

#define map_of(name, K, V)                                                  \
	struct map_##name {                                                 \
		struct map_item_##name *mem;                                \
		uint32_t cap;                                                  \
		uint32_t size;                                                 \
		uint32_t load_fac;                                             \
		uint32_t remap;                                                \
		bool used;                                                     \
		bool oom;                                                      \
		bool found;                                                    \
	};                                                                     \
                                                                               \
	/**                                                                    \
	 * Create map                                                          \
	 *                                                                     \
	 * @param map map                                                      \
	 * @param cap initial capacity, zero is accepted                       \
	 * @param load_factor must be >25 and <95. Pass 0 for default value.   \
	 * @return 'true' on success,                                          \
	 *         'false' on out of memory or if 'load_factor' value is       \
	 *          invalid.                                                   \
	 */                                                                    \
	bool map_init_##name(struct map_##name *map, uint32_t cap,       \
				uint32_t load_factor);                         \
                                                                               \
	/**                                                                    \
	 * Destroy map.                                                        \
	 *                                                                     \
	 * @param map map                                                      \
	 */                                                                    \
	void map_term_##name(struct map_##name *map);                    \
                                                                               \
	/**                                                                    \
	 * Get map element count                                               \
	 *                                                                     \
	 * @param map map                                                      \
	 * @return element count                                               \
	 */                                                                    \
	uint32_t map_size_##name(struct map_##name *map);                \
                                                                               \
	/**                                                                    \
	 * Clear map                                                           \
	 *                                                                     \
	 * @param map map                                                      \
	 */                                                                    \
	void map_clear_##name(struct map_##name *map);                   \
                                                                               \
	/**                                                                    \
	 * Put element to the map                                              \
	 *                                                                     \
	 * struct map_str map;                                              \
	 * map_put_str(&map, "key", "value");                               \
	 *                                                                     \
	 * @param map map                                                      \
	 * @param K key                                                        \
	 * @param V value                                                      \
	 * @return previous value if exists                                    \
	 *         call map_found() to see if the returned value is valid.  \
	 */                                                                    \
	V map_put_##name(struct map_##name *map, K key, V val);          \
                                                                               \
	/**                                                                    \
	 * Get element                                                         \
	 *                                                                     \
	 * @param map map                                                      \
	 * @param K key                                                        \
	 * @return current value if exists.                                    \
	 *         call map_found() to see if the returned value is valid.  \
	 */                                                                    \
	/** NOLINTNEXTLINE */                                                  \
	V map_get_##name(struct map_##name *map, K key);                 \
                                                                               \
	/**                                                                    \
	 * Delete element                                                      \
	 *                                                                     \
	 * @param map map                                                      \
	 * @param K key                                                        \
	 * @return current value if exists.                                    \
	 *         call map_found() to see if the returned value is valid.  \
	 */                                                                    \
	/** NOLINTNEXTLINE */                                                  \
	V map_del_##name(struct map_##name *map, K key);

/**
 * @param map map
 * @return    - if put operation overrides a value, returns true
 *            - if get operation finds the key, returns true
 *            - if del operation deletes a key, returns true
 */
#define map_found(map) ((map)->found)

/**
 * @param map map
 * @return    true if put operation failed with out of memory
 */
#define map_oom(map) ((map)->oom)

// clang-format off

/**
 * Foreach loop
 *
 * char *key, *value;
 * struct map_str map;
 *
 * map_foreach(&map, key, value) {
 *      printf("key = %s, value = %s \n");
 * }
 */
#define map_foreach(map, K, V)                                                  \
	for (int64_t _i = -1, _b = 0; !_b && _i < (map)->cap; _i++)                \
		for ((V) = (map)->mem[_i].value, (K) = (map)->mem[_i].key, _b = 1; \
		     _b && ((_i == -1 && (map)->used) || (K) != 0) ? 1 : (_b = 0); \
		     _b = 0)

/**
 * Foreach loop for keys
 *
 * char *key;
 * struct map_str map;
 *
 * map_foreach_key(&map, key) {
 *      printf("key = %s \n");
 * }
 */
#define map_foreach_key(map, K)                                                 \
	for (int64_t _i = -1, _b = 0; !_b && _i < (map)->cap; _i++)                \
		for ((K) = (map)->mem[_i].key, _b = 1;                             \
		     _b && ((_i == -1 && (map)->used) || (K) != 0) ? 1 : (_b = 0); \
		     _b = 0)

/**
 * Foreach loop for values
 *
 * char *value;
 * struct map_str map;
 *
 * map_foreach_value(&map, value) {
 *      printf("value = %s \n");
 * }
 */
#define map_foreach_value(map, V)                                                              \
	for (int64_t _i = -1, _b = 0; !_b && _i < (map)->cap; _i++)                               \
		for ((V) = (map)->mem[_i].value, _b = 1;                                          \
		     _b && ((_i == -1 && (map)->used) || (map)->mem[_i].key != 0) ? 1 : (_b = 0); \
		     _b = 0)

//              name  key type      value type
map_dec_scalar(32,  uint32_t,     uint32_t)
map_dec_scalar(64,  uint64_t,     uint64_t)
map_dec_scalar(64v, uint64_t,     void *)
map_dec_scalar(64s, uint64_t,     const char *)
map_dec_strkey(str, const char *, const char *)
map_dec_strkey(sv,  const char *, void*)
map_dec_strkey(s64, const char *, uint64_t)

#ifdef __cplusplus
}
#endif

#if defined(_UTILS_IMPL) || defined(_UTILS_MAP_IMPL)

#include <string.h>

#ifndef MAP_MAX
#define MAP_MAX UINT32_MAX
#endif

#define map_def_strkey(name, K, V, cmp, hash_fn)                            \
	bool map_cmp_##name(struct map_item_##name *t, K key,            \
			       uint32_t hash)                                  \
	{                                                                      \
		return t->hash == hash && cmp(t->key, key);                    \
	}                                                                      \
                                                                               \
	void map_assign_##name(struct map_item_##name *t, K key,         \
				  V value, uint32_t hash)                      \
	{                                                                      \
		t->key = key;                                                  \
		t->value = value;                                              \
		t->hash = hash;                                                \
	}                                                                      \
                                                                               \
	uint32_t map_hashof_##name(struct map_item_##name *t)            \
	{                                                                      \
		return t->hash;                                                \
	}                                                                      \
                                                                               \
	map_def(name, K, V, cmp, hash_fn)

#define map_def_scalar(name, K, V, cmp, hash_fn)                            \
	bool map_cmp_##name(struct map_item_##name *t, K key,            \
			       uint32_t hash)                                  \
	{                                                                      \
		(void) hash;                                                   \
		return cmp(t->key, key);                                       \
	}                                                                      \
                                                                               \
	void map_assign_##name(struct map_item_##name *t, K key,         \
				  V value, uint32_t hash)                      \
	{                                                                      \
		(void) hash;                                                   \
		t->key = key;                                                  \
		t->value = value;                                              \
	}                                                                      \
                                                                               \
	uint32_t map_hashof_##name(struct map_item_##name *t)            \
	{                                                                      \
		return hash_fn(t->key);                                        \
	}                                                                      \
                                                                               \
	map_def(name, K, V, cmp, hash_fn)

#define map_def(name, K, V, cmp, hash_fn)                                   \
                                                                               \
	static const struct map_item_##name empty_items_##name[2];          \
                                                                               \
	static const struct map_##name map_empty_##name = {              \
		.cap = 1,                                                      \
		.mem = (struct map_item_##name *) &empty_items_##name[1]};  \
                                                                               \
	static void *map_alloc_##name(uint32_t *cap, uint32_t factor)       \
	{                                                                      \
		uint32_t v = *cap;                                             \
		struct map_item_##name *t;                                  \
                                                                               \
		if (*cap > MAP_MAX / factor) {                              \
			return NULL;                                           	   \
		}                                                              \
                                                                               \
		/* Find next power of two */                                   \
		v = v < 8 ? 8 : (v * factor);                                  \
		v--;                                                           \
		for (uint32_t i = 1; i < sizeof(v) * 8; i *= 2) {              \
			v |= v >> i;                                           \
		}                                                              \
		v++;                                                           \
                                                                               \
		*cap = v;                                                      \
		t = map_calloc(sizeof(*t), v + 1);                          \
		return t ? &t[1] : NULL;                                       \
	}                                                                      \
                                                                               \
	bool map_init_##name(struct map_##name *m, uint32_t cap,         \
				uint32_t load_fac)                             \
	{                                                                      \
		void *t;                                                       \
		uint32_t f = (load_fac == 0) ? 75 : load_fac;                  \
                                                                               \
		if (f > 95 || f < 25) {                                        \
			return false;                                          \
		}                                                              \
                                                                               \
		if (cap == 0) {                                                \
			*m = map_empty_##name;                              \
			m->load_fac = f;                                       \
			return true;                                           \
		}                                                              	\
                                                                     	\
		t = map_alloc_##name(&cap, 1);                             	\
		if (t == NULL) {                                               	\
			return false;                                          		\
		}                                                              	\
                                                                               \
		m->mem = t;                                                    \
		m->size = 0;                                                   \
		m->used = false;                                               \
		m->cap = cap;                                                  \
		m->load_fac = f;                                               \
		m->remap = (uint32_t) (m->cap * ((double) m->load_fac / 100)); \
                                                                               \
		return true;                                                   \
	}                                                                      \
                                                                               \
	void map_term_##name(struct map_##name *m)                       \
	{                                                                      \
		if (m->mem != map_empty_##name.mem) {                       \
			map_free(&m->mem[-1]);                              \
			*m = map_empty_##name;                              \
		}                                                              \
	}                                                                      \
                                                                               \
	uint32_t map_size_##name(struct map_##name *m)                   \
	{                                                                      \
		return m->size;                                                \
	}                                                                      \
                                                                               \
	void map_clear_##name(struct map_##name *m)                      \
	{                                                                      \
		if (m->size > 0) {                                             \
			for (uint32_t i = 0; i < m->cap; i++) {                \
				m->mem[i].key = 0;                             \
			}                                                      \
                                                                               \
			m->used = false;                                       \
			m->size = 0;                                           \
		}                                                              \
	}                                                                      \
                                                                               \
	static bool map_remap_##name(struct map_##name *m)               \
	{                                                                      \
		uint32_t pos, cap, mod;                                        \
		struct map_item_##name *new;                                \
                                                                               \
		if (m->size < m->remap) {                                      \
			return true;                                           \
		}                                                              \
                                                                               \
		cap = m->cap;                                                  \
		new = map_alloc_##name(&cap, 2);                            \
		if (new == NULL) {                                             \
			return false;                                          \
		}                                                              \
                                                                               \
		mod = cap - 1;                                                 \
                                                                               \
		for (uint32_t i = 0; i < m->cap; i++) {                        \
			if (m->mem[i].key != 0) {                              \
				pos = map_hashof_##name(&m->mem[i]) & mod;  \
                                                                               \
				while (true) {                                 \
					if (new[pos].key == 0) {               \
						new[pos] = m->mem[i];          \
						break;                         \
					}                                      \
                                                                               \
					pos = (pos + 1) & (mod);               \
				}                                              \
			}                                                      \
		}                                                              \
                                                                               \
		if (m->mem != map_empty_##name.mem) {                       \
			new[-1] = m->mem[-1];                                  \
			map_free(&m->mem[-1]);                              \
		}                                                              \
                                                                               \
		m->mem = new;                                                  \
		m->cap = cap;                                                  \
		m->remap = (uint32_t) (m->cap * ((double) m->load_fac / 100)); \
                                                                               \
		return true;                                                   \
	}                                                                      \
                                                                               \
	V map_put_##name(struct map_##name *m, K key, V value)           \
	{                                                                      \
		V ret;                                                         \
		uint32_t pos, mod, h;                                          \
                                                                               \
		m->oom = false;                                                \
                                                                               \
		if (!map_remap_##name(m)) {                                 \
			m->oom = true;                                         \
			return 0;                                              \
		}                                                              \
                                                                               \
		if (key == 0) {                                                \
			ret = (m->used) ? m->mem[-1].value : 0;                \
			m->found = m->used;                                    \
			m->size += !m->used;                                   \
			m->used = true;                                        \
			m->mem[-1].value = value;                              \
                                                                               \
			return ret;                                            \
		}                                                              \
                                                                               \
		mod = m->cap - 1;                                              \
		h = hash_fn(key);                                              \
		pos = h & (mod);                                               \
                                                                               \
		while (true) {                                                 \
			if (m->mem[pos].key == 0) {                            \
				m->size++;                                     \
			} else if (!map_cmp_##name(&m->mem[pos], key, h)) { \
				pos = (pos + 1) & (mod);                       \
				continue;                                      \
			}                                                      \
                                                                               \
			m->found = m->mem[pos].key != 0;                       \
			ret = m->found ? m->mem[pos].value : 0;                \
			map_assign_##name(&m->mem[pos], key, value, h);     \
                                                                               \
			return ret;                                            \
		}                                                              \
	}                                                                      \
                                                                               \
	/** NOLINTNEXTLINE */                                                  \
	V map_get_##name(struct map_##name *m, K key)                    \
	{                                                                      \
		const uint32_t mod = m->cap - 1;                               \
		uint32_t h, pos;                                               \
                                                                               \
		if (key == 0) {                                                \
			m->found = m->used;                                    \
			return m->used ? m->mem[-1].value : 0;                 \
		}                                                              \
                                                                               \
		h = hash_fn(key);                                              \
		pos = h & mod;                                                 \
                                                                               \
		while (true) {                                                 \
			if (m->mem[pos].key == 0) {                            \
				m->found = false;                              \
				return 0;                                      \
			} else if (!map_cmp_##name(&m->mem[pos], key, h)) { \
				pos = (pos + 1) & (mod);                       \
				continue;                                      \
			}                                                      \
                                                                               \
			m->found = true;                                       \
			return m->mem[pos].value;                              \
		}                                                              \
	}                                                                      \
                                                                               \
	/** NOLINTNEXTLINE */                                                  \
	V map_del_##name(struct map_##name *m, K key)                    \
	{                                                                      \
		const uint32_t mod = m->cap - 1;                               \
		uint32_t pos, prev, it, p, h;                                  \
		V ret;                                                         \
                                                                               \
		if (key == 0) {                                                \
			m->found = m->used;                                    \
			m->size -= m->used;                                    \
			m->used = false;                                       \
                                                                               \
			return m->found ? m->mem[-1].value : 0;                \
		}                                                              \
                                                                               \
		h = hash_fn(key);                                              \
		pos = h & (mod);                                               \
                                                                               \
		while (true) {                                                 \
			if (m->mem[pos].key == 0) {                            \
				m->found = false;                              \
				return 0;                                      \
			} else if (!map_cmp_##name(&m->mem[pos], key, h)) { \
				pos = (pos + 1) & (mod);                       \
				continue;                                      \
			}                                                      \
                                                                               \
			m->found = true;                                       \
			ret = m->mem[pos].value;                               \
                                                                               \
			m->size--;                                             \
			m->mem[pos].key = 0;                                   \
			prev = pos;                                            \
			it = pos;                                              \
                                                                               \
			while (true) {                                         \
				it = (it + 1) & (mod);                         \
				if (m->mem[it].key == 0) {                     \
					break;                                 \
				}                                              \
                                                                               \
				p = map_hashof_##name(&m->mem[it]) & (mod); \
                                                                               \
				if ((p > it && (p <= prev || it >= prev)) ||   \
				    (p <= prev && it >= prev)) {               \
                                                                               \
					m->mem[prev] = m->mem[it];             \
					m->mem[it].key = 0;                    \
					prev = it;                             \
				}                                              \
			}                                                      \
                                                                               \
			return ret;                                            \
		}                                                              \
	}

//              name,  key type,   value type,     cmp           hash
map_def_scalar(32,  uint32_t,     uint32_t,     cmp_int,    hash_32)
map_def_scalar(64,  uint64_t,     uint64_t,     cmp_int,    hash_64)
map_def_scalar(64v, uint64_t,     void *,       cmp_int,    hash_64)
map_def_scalar(64s, uint64_t,     const char *, cmp_int,    hash_64)
map_def_strkey(str, const char *, const char *, cmp_str, murmurhash)
map_def_strkey(sv,  const char *, void *,       cmp_str, murmurhash)
map_def_strkey(s64, const char *, uint64_t,     cmp_str, murmurhash)

#endif
#endif