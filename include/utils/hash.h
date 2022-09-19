#ifndef _UTILS_HASH_H
#define _UTILS_HASH_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

uint32_t hash_32(uint32_t a);

uint32_t hash_64(uint64_t a);

uint32_t murmurhash(const char *key);

bool cmp_int(uint64_t old, uint64_t key);

bool cmp_str(const char* old, const char *key);

#ifdef __cplusplus
}
#endif

#if defined(_UTILS_IMPL) || defined(_UTILS_HASH_IMPL)

uint32_t hash_32(uint32_t a)
{
	return a;
}

uint32_t hash_64(uint64_t a)
{
	return ((uint32_t) a) ^ (uint32_t) (a >> 32u);
}

uint32_t murmurhash(const char *key)
{
	const uint64_t m = UINT64_C(0xc6a4a7935bd1e995);
	const size_t len = strlen(key);
	const unsigned char *p = (const unsigned char *) key;
	const unsigned char *end = p + (len & ~(uint64_t) 0x7);
	uint64_t h = (len * m);

	while (p != end) {
		uint64_t k;
		memcpy(&k, p, sizeof(k));

		k *= m;
		k ^= k >> 47u;
		k *= m;

		h ^= k;
		h *= m;
		p += 8;
	}

	switch (len & 7u) {
	case 7:
		h ^= (uint64_t) p[6] << 48ul; // fall through
	case 6:
		h ^= (uint64_t) p[5] << 40ul; // fall through
	case 5:
		h ^= (uint64_t) p[4] << 32ul; // fall through
	case 4:
		h ^= (uint64_t) p[3] << 24ul; // fall through
	case 3:
		h ^= (uint64_t) p[2] << 16ul; // fall through
	case 2:
		h ^= (uint64_t) p[1] << 8ul; // fall through
	case 1:
		h ^= (uint64_t) p[0]; // fall through
		h *= m;
	default:
		break;
	};

	h ^= h >> 47u;
	h *= m;
	h ^= h >> 47u;

	return (uint32_t) h;
}

bool cmp_int(uint64_t old, uint64_t key)
{
	return (old == key);
}

bool cmp_str(const char* old, const char *key)
{
	return !strcmp(old, key);
}

#endif
#endif