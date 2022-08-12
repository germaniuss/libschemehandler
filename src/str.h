/*
 * BSD-3-Clause
 *
 * Copyright 2021 Ozan Tezcan
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef STR_H
#define STR_H

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

#define STR_VERSION "2.0.0"

#ifdef HAVE_CONFIG_H
#include "config.h"
#else
#define str_malloc malloc
#define str_realloc realloc
#define str_free free
#endif

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

#endif
