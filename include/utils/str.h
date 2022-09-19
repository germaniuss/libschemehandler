#ifndef _UTILS_STR_H
#define _UTILS_STR_H

#ifdef __cplusplus
extern "C" {
#endif

#define str_malloc malloc
#define str_realloc realloc
#define str_free free

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

/**
 * length prefixed C strings, length is at the start of the allocated memory
 *  e.g :
 *  -----------------------------------------------
 *  | 0 | 0 | 0 | 4 | 'T' | 'E' | 'S' | 'T' | '\0'|
 *  -----------------------------------------------
 *                    ^
 *                  return
 *  User can keep pointer to first character, so it's like C style strings with
 *  additional functionality when it's used with these functions here.
 */

/**
 * @param str '\0' terminated C string, can be NULL.
 * @return    length prefixed string. NULL on out of memory or if 'str' is NULL.
 */
char *str_create(const char *str);

/**
 * @param str string bytes, no need for '\0' termination.
 * @param len length of the 'str'.
 * @return    length prefixed string. NULL on out of memory or if 'str' is NULL.
 */
char *str_create_len(const char *str, uint32_t len);

/**
 * @param fmt format
 * @param ... arguments
 * @return    length prefixed string. NULL on out of memory.
 */
char *str_create_fmt(const char *fmt, ...);

/**
 * @param fmt format
 * @param va  va_list
 * @return    length prefixed string. NULL on out of memory.
 */
char *str_create_va(const char *fmt, va_list va);

/**
 * @param len desired length of the string
 * @return    randomly generated string
 */
char *str_create_random(uint32_t len);

/**
 * @param str string to be encoded
 * @return    base 64 encoded str.
 */
char *str_encode_base64(const char *str);

/**
 * Deallocate length prefixed string.
 * @param str length prefixed string. str may be NULL.
 */
void str_destroy(char **str);

/**
 * @param str length prefixed string. NULL values are accepted.
 * @return    length of the string. If NULL, returns -1.
 */
int64_t str_len(const char *str);

/**
 * @param str length prefixed string. NULL values are accepted.
 * @return    duplicate string. NULL on out of memory or if 'str' is NULL.
 */
char *str_dup(const char *str);

/**
 * @param str    Pointer to length prefixed string, '*str' may change.
 * @param param  New value to set.
 * @return      'true' on success, 'false' on out of memory or if '*str' is NULL
 */
bool str_set(char **str, const char *param);

/**
 * @param str pointer to length prefixed string, '*str' may change.
 * @param fmt format
 * @param ... arguments
 * @return    'true' on success, 'false' on out of memory
 */
bool str_set_fmt(char **str, const char *fmt, ...);

/**
 * @param str  pointer to length prefixed string, '*str' may change.
 * @param text text to append.
 * @return    'true' on success, 'false' on out of memory or if '*str' is NULL.
 */
bool str_append(char **str, const char *text);

/**
 * @param str pointer to length prefixed string. (char**).'*str' may change.
 * @param fmt format
 * @param ... arguments
 * @return    'true' on success, 'false' on out of memory or if '*str' is NULL.
 */
#define str_append_fmt(str, fmt, ...)                                       \
	((*str) ? (str_set_fmt(str, "%s" fmt, *str, __VA_ARGS__)) :         \
			(str_set_fmt(str, fmt, __VA_ARGS__)))

/**
 * Compare two length prefixed strings. To compare with C string, use strcmp().
 *
 * @param str   length prefixed string, must not be NULL.
 * @param other length prefixed string, must not be NULL.
 * @return      'true' if equals.
 */
bool str_cmp(const char *str, const char *other);

/**
 * @param str  length prefixed string, '*str' may change
 * @param list character list to trim.
 * @return    'true' on success or if '*str' is NULL. 'false' on out of memory
 */
bool str_trim(char **str, const char *list);

/**
 * @param str   length prefixed string, *str' may change.
 * @param start start index.
 * @param end   end index.
 * @return      'false' on out of range, on out of memory or if '*str' is NULL.
 *              'true' on success.
 */
bool str_substring(char **str, uint32_t start, uint32_t end);

/**
 * @param str  length prefixed string, '*str' may change.
 * @param rep  string to be replaced
 * @param with string to replace with
 * @return    'true' on success or if '*str' is NULL.  'false' on out of memory
 */
bool str_replace(char **str, const char *rep, const char *with);

/**
 * Tokenization is zero-copy but a bit tricky. This function will mutate 'str',
 * but it is temporary. On each 'str_token_begin' call, this function will
 * place '\0' character at the end of a token and put delimiter at the end of
 * the 'str'.
 * e.g user1-user2\0 after first iteration will be user1\0user2-
 *
 * str_token_end() will fix original string if necessary.
 *
 * usage:
 *
 * char *str = str_create("user1-user2-user3");
 * char *save = NULL; // Must be initialized with NULL.
 * const char *token;
 *
 * while ((token = str_token_begin(str, &save, "-") != NULL) {
 *      printf("token : %s \n", token);
 * }
 *
 * str_token_end(str, &save);
 *
 *
 * @param str   length prefixed string, must not be NULL.
 * @param save  helper variable for tokenizer code.
 * @param delim delimiter list.
 * @return      token.
 */
const char *str_token_begin(char *str, char **save, const char *delim);
void str_token_end(char *str, char **save);

#ifdef __cplusplus
}
#endif

#if defined(_UTILS_IMPL) || defined(_UTILS_STR_IMPL)

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * String with 'length' at the start of the allocated memory
 *  e.g :
 *  -----------------------------------------------
 *  | 0 | 0 | 0 | 4 | 'T' | 'E' | 'S' | 'T' | '\0'|
 *  -----------------------------------------------
 *
 *  User can keep pointer to first character, so it's like C style strings with
 *  additional functionality when it's used with these functions here.
 */
struct str {
	uint32_t len;
	char buf[];
};

#define str_meta(arr)                                                       \
	((struct str *) ((char *) (arr) -offsetof(struct str, buf)))

#define str_bytes(n) ((n) + sizeof(struct str) + 1)

#ifndef STR_MAX
#define STR_MAX (UINT32_MAX - sizeof(struct str) - 1)
#endif

char *str_create(const char *arr)
{
	size_t len;

	if (arr == NULL || (len = strlen(arr)) > STR_MAX) {
		return NULL;
	}

	return str_create_len(arr, (uint32_t) len);
}

char *str_create_len(const char *arr, uint32_t len)
{
	struct str *c;

	if (arr == NULL) {
		return NULL;
	}

	c = str_malloc(str_bytes(len));
	if (c == NULL) {
		return NULL;
	}

	memcpy(c->buf, arr, len);
	c->buf[len] = '\0';
	c->len = len;

	return c->buf;
}

char *str_create_va(const char *fmt, va_list va)
{
	int rc;
	char tmp[1024];
	struct str *arr;
	va_list args;

	va_copy(args, va);
	rc = vsnprintf(tmp, sizeof(tmp), fmt, args);
	if (rc < 0) {
		va_end(args);
		return NULL;
	}
	va_end(args);

	arr = str_malloc(str_bytes(rc));
	if (arr == NULL) {
		return NULL;
	}

	arr->len = (uint32_t) rc;

	if (rc < (int) sizeof(tmp)) {
		memcpy(arr->buf, tmp, arr->len + 1);
	} else {
		va_copy(args, va);
		rc = vsnprintf(arr->buf, arr->len + 1, fmt, args);
		va_end(args);

		if (rc < 0 || (uint32_t) rc > arr->len) {
			str_free(arr);
			return NULL;
		}
	}

	return arr->buf;
}

char *str_create_fmt(const char *fmt, ...)
{
	char *arr;
	va_list args;

	va_start(args, fmt);
	arr = str_create_va(fmt, args);
	va_end(args);

	return arr;
}

char *str_create_random(uint32_t len)
{

	if (!len) return NULL;

	const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
    char *str = str_create_len("", len);
	for (size_t n = 0; n < len; n++) {
		str[n] = charset[rand() % (int) (sizeof charset - 1)];
	} str[len] = '\0';

	return str;
}

char *str_encode_base64(const char *str) {

	size_t len;

	if (str == NULL || (len = strlen(str)) > STR_MAX) {
		return NULL;
	}

	const char base64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
    char* encoded = (char*) malloc(len);
    char *p = encoded; int i;

    for (i = 0; i < len - 2; i += 3) {
        *p++ = base64[(str[i] >> 2) & 0x3F];
        *p++ = base64[((str[i] & 0x3) << 4) | ((int) (str[i + 1] & 0xF0) >> 4)];
        *p++ = base64[((str[i + 1] & 0xF) << 2) | ((int) (str[i + 2] & 0xC0) >> 6)];
        *p++ = base64[str[i + 2] & 0x3F];
    } 

    if (i < len) {
        *p++ = base64[(str[i] >> 2) & 0x3F];
        if (i == (len - 1)) {
            *p++ = base64[((str[i] & 0x3) << 4)];
        } else {
            *p++ = base64[((str[i] & 0x3) << 4) | ((int) (str[i + 1] & 0xF0) >> 4)];
            *p++ = base64[((str[i + 1] & 0xF) << 2)];
        }
    }

    *p++ = '\0';

	return str_create_len(encoded, len);
}

void str_destroy(char **arr)
{
	if (arr == NULL || *arr == NULL) {
		return;
	}

	str_free(str_meta(*arr));
	*arr = NULL;
}

int64_t str_len(const char *arr)
{
	if (arr == NULL) {
		return -1;
	}

	return str_meta(arr)->len;
}

char *str_dup(const char *arr)
{
	struct str *m;

	if (arr == NULL) {
		return NULL;
	}

	m = str_meta(arr);
	return str_create_len(arr, m->len);
}

bool str_set(char **arr, const char *param)
{
	char *c;

	c = str_create(param);
	if (c == NULL) {
		return false;
	}

	str_destroy(arr);
	*arr = c;

	return true;
}

bool str_set_fmt(char **arr, const char *fmt, ...)
{
	char *ret;
	va_list args;

	va_start(args, fmt);
	ret = str_create_va(fmt, args);
	va_end(args);

	if (ret != NULL) {
		str_destroy(arr);
		*arr = ret;
	}

	return ret != NULL;
}

bool str_append(char **arr, const char *param)
{
	size_t len, alloc;
	struct str *m;

	if (*arr == NULL) {
		*arr = str_create(param);
		return *arr != NULL;
	}

	m = str_meta(*arr);
	len = strlen(param);
	alloc = str_bytes(m->len + len);

	if (len > STR_MAX - m->len ||
	    (m = str_realloc(m, alloc)) == NULL) {
		return false;
	}

	memcpy(&m->buf[m->len], param, len);
	m->len += (uint32_t) len;
	m->buf[m->len] = '\0';
	*arr = m->buf;

	return true;
}

bool str_cmp(const char *arr, const char *other)
{
	struct str *s1 = str_meta(arr);
	struct str *s2 = str_meta(other);

	return s1->len == s2->len && !memcmp(s1->buf, s2->buf, s1->len);
}

static void swap(char *arr, char *d)
{
	char tmp;
	char *c = arr + str_meta(arr)->len;

	tmp = *c;
	*c = *d;
	*d = tmp;
}

const char *str_token_begin(char *arr, char **save, const char *delim)
{
	char *it = arr;

	if (arr == NULL) {
		return NULL;
	}

	if (*save != NULL) {
		it = *save;
		swap(arr, it);
		if (*it == '\0') {
			return NULL;
		}
		it++;
	}

	*save = it + strcspn(it, delim);
	swap(arr, *save);

	return it;
}

void str_token_end(char *arr, char **save)
{
	char *end;

	if (arr == NULL) {
		return;
	}

	end = arr + str_meta(arr)->len;
	if (*end == '\0') {
		return;
	}

	swap(arr, *save);
}

bool str_trim(char **arr, const char *list)
{
	uint32_t diff;
	size_t len;
	char *head, *end;

	if (*arr == NULL) {
		return true;
	}

	len = str_meta(*arr)->len;
	head = *arr + strspn(*arr, list);
	end = (*arr) + len;

	while (end > head) {
		end--;
		if (!strchr(list, *end)) {
			end++;
			break;
		}
	}

	if (head != *arr || end != (*arr) + len) {
		diff = (uint32_t) (end - head);
		head = str_create_len(head, diff);
		if (head == NULL) {
			return false;
		}

		str_destroy(arr);
		*arr = head;
	}

	return true;
}

bool str_substring(char **arr, uint32_t start, uint32_t end)
{
	char *c;
	struct str *meta;

	if (*arr == NULL) {
		return false;
	}

	meta = str_meta(*arr);
	if (start > meta->len || end > meta->len || start > end) {
		return false;
	}

	c = str_create_len(*arr + start, end - start);
	if (c == NULL) {
		return false;
	}

	str_destroy(arr);
	*arr = c;

	return true;
}

bool str_replace(char **arr, const char *replace, const char *with)
{
	assert(replace != NULL && with != NULL);

	int64_t diff;
	size_t rep_len, with_len;
	size_t len_unmatch, count, size;
	struct str *dest, *meta;
	char *tmp, *orig, *orig_end;

	if (*arr == NULL) {
		return true;
	}

	rep_len = strlen(replace);
	with_len = strlen(with);

	if (rep_len >= UINT32_MAX || with_len >= UINT32_MAX) {
		return false;
	}

	meta = str_meta(*arr);
	orig = *arr;
	orig_end = *arr + meta->len;
	diff = (int64_t) with_len - (int64_t) rep_len;

	// Fast path, same size replacement.
	if (diff == 0) {
		while ((orig = strstr(orig, replace)) != NULL) {
			memcpy(orig, with, rep_len);
			orig += rep_len;
		}

		return true;
	}

	// Calculate new string size.
	tmp = orig;
	size = meta->len;
	for (count = 0; (tmp = strstr(tmp, replace)) != NULL; count++) {
		tmp += rep_len;
		// Check overflow.
		if ((int64_t) size > (int64_t) STR_MAX - diff) {
			return false;
		}
		size += diff;
	}

	// No match.
	if (count == 0) {
		return true;
	}

	dest = str_malloc(str_bytes(size));
	if (!dest) {
		return false;
	}

	dest->len = (uint32_t) size;
	tmp = dest->buf;

	while (count--) {
		len_unmatch = strstr(orig, replace) - orig;
		memcpy(tmp, orig, len_unmatch);
		tmp += len_unmatch;

		// Copy extra '\0' byte just to silence sanitizer.
		memcpy(tmp, with, with_len + 1);
		tmp += with_len;
		orig += len_unmatch + rep_len;
	}

	memcpy(tmp, orig, orig_end - orig + 1);

	str_destroy(arr);
	*arr = dest->buf;

	return true;
}

#endif
#endif