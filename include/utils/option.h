#ifndef _UTILS_OPTION_H
#define _UTILS_OPTION_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

struct option_item {
	const char letter;
	const char *name;
};

struct option {
	struct option_item *options;
	int count;
	char **argv;
};

/**
 *
 * @param opt    Already initialized opt struct
 * @param index  Index for argv
 * @param value  [out] Value for the option if exists. It should be after '='
 *               sign. E.g : -key=value or -k=value. If value does not exists
 *               (*value) will point to '\0' character. It won't be NULL itself.
 *
 *               To check if option has value associated : if (*value != '\0')
 *
 * @return       Letter for the option. If option is not known, '?' will be
 *               returned.
 */
char option_at(struct option *opt, int index, char **value);

#ifdef __cplusplus
}
#endif

#if defined(_UTILS_IMPL) || defined(_UTILS_OPTION_IMPL)

#include <string.h>

char option_at(struct option *opt, int index, char **value)
{
	char id = '?';
	size_t len;
	char *pos;
	const char *curr, *name;

	pos = opt->argv[index];
	*value = NULL;

	if (*pos != '-') {
		return id;
	}

	pos++; // Skip first '-'
	if (*pos != '-') {
		for (int i = 0; i < opt->count; i++) {
			if (*pos == opt->options[i].letter &&
			    strchr("= \0", *(pos + 1)) != NULL) {
				id = *pos;
				pos++; // skip letter
				*value = pos + (*pos != '=' ? 0 : 1);
				break;
			}
		}
	} else {
		while (*pos && *pos != '=') {
			pos++;
		}

		for (int i = 0; i < opt->count; i++) {
			curr = opt->argv[index] + 2; // Skip '--'
			name = opt->options[i].name;
			len = (pos - curr);

			if (name == NULL) {
				continue;
			}

			if (len == strlen(name) &&
			    memcmp(name, curr, len) == 0) {
				id = opt->options[i].letter;
				*value = pos + (*pos != '=' ? 0 : 1);
				break;
			}
		}
	}

	return id;
}

#endif
#endif