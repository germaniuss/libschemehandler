#ifndef _UTILS_INI_H
#define _UTILS_INI_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

typedef struct ini_entry {
	const char* key;
	const char* value;
} ini_entry;

typedef struct ini_section {
	const char* name;
	ini_entry entries[32];
	uint32_t size;
} ini_section;

typedef struct ini {
	const char* filename;
	ini_section sections[32];
	uint32_t size;
} ini;

// Set max line length. If a line is longer, it will be truncated silently.
#define INI_MAX_LINE_LEN 1024

/**
 * @param arg      user arg
 * @param line     current line number
 * @param section  section
 * @param key      key
 * @param value    value
 * @return         Return '0' on success, any other value will make parser
 *                 stop and return error
 */
typedef int (*ini_on_item)(void *arg, int line, const char *section,
			      const char *key, const char *value);

/**
 * @param arg      user data to be passed to 'on_item' callback
 * @param on_item  callback
 * @param filename filename
 * @return         - '0' on success,
 *                 - '-1' on file IO error.
 *                 - positive line number on parsing error
 *                 - 'on_item' return value if it returns other than '0'
 */
int ini_parse_file(void *arg, ini_on_item on_item, const char *filename);

/**
 * @param arg      user data to be passed to 'on_item' callback.
 * @param on_item  callback
 * @param str      string to parse
 * @return         - '0' on success,
 *                 - positive line number on parsing error
 *                 - "on_item's return value" if it returns other than '0'
 */
int ini_parse_string(void *arg, ini_on_item on_item, const char *str);

bool ini_open(ini* arg, const char* filename);
bool ini_save(ini* arg, const char* section, const char* key, const char* value);
bool ini_close(ini* arg);

#ifdef __cplusplus
}
#endif

#if defined(_UTILS_IMPL) || defined(_UTILS_INI_IMPL)

#include <ctype.h>
#include <string.h>

#if defined(_WIN32) || defined(_WIN64)
#pragma warning(disable : 4996)
#endif

static int ini_default_fn(ini *arg, int line, const char *section, const char *key, const char *value) {
	uint32_t index;
	
	if (!arg->size || 
		!section[0] || 
		strcmp(arg->sections[arg->size - 1].name, section)
	) {
		if (arg->size == 32) return 1;
		index = arg->size++;
		arg->sections[index].name = strdup(section);
		arg->sections[index].size = 0;
	} else index = arg->size - 1;

	ini_section* ini_sec = &arg->sections[index];
	if (ini_sec->size == 32) return 1;
	index = ini_sec->size++;
	ini_sec->entries[index].key = strdup(key);
	ini_sec->entries[index].value = strdup(value);
	return 0;
}

bool ini_open(ini* arg, const char* filename) {
	arg->filename = strdup(filename);
	arg->size = 0;
	if (filename == NULL) 
        return NULL;
	return ini_parse_file(arg, ini_default_fn, filename);
}

bool ini_close(ini* arg) {
	
	if (!arg->filename) {
		return NULL;
	}

    FILE *fp = fopen(arg->filename, "w");
    if (fp == NULL) return NULL;

	ini_section* sec;
    for (uint32_t i = 0; i < arg->size; i++) {
		sec = &arg->sections[i];
		if (sec->name[0]) fprintf(fp, "\n[%s]\n", sec->name);
		for (uint32_t j = 0; j < sec->size; j++) {
			fprintf(fp, "%s = %s\n", sec->entries[j].key, sec->entries[j].value);
		}
	}
        
    // close the file
    fclose(fp);
    return 1;
}

bool ini_save(ini* arg, const char* section, const char* key, const char* value) {
	if (!section || !key || !value) {
		return NULL;
	}
	
	bool found;
	uint32_t i;

	// Find section if exists
	found = false;
	for (i = 0; i < arg->size && !found; i++) {
		if (!strcmp(arg->sections[i].name, section)) {
			found = true;
			break;
		}
	}

	if (i == 32) return 0;

	// If not found then add the section and return
	ini_section* sec = &arg->sections[i];
	if (!found) {
		sec->name = strdup(section);
		sec->size = 1;
		sec->entries[0].key = strdup(key);
		sec->entries[0].value = strdup(value);
		return 1;
	}

	// If found then find entry if exists
	found = false;
	for (i = 0; i < sec->size && !found; i++) {
		if (!strcmp(sec->entries[i].key, key)) {
			found = true;
			break;
		}
	}

	if (i == 32) return 0;

	// If not found create entry, else update it
	ini_entry* ent = &sec->entries[i];
	if (!found) {
		ent->key = strdup(key);
		ent->value = strdup(value);
		return 1;
	}

	if (strcmp(ent->value, value)) {
		ent->value = strdup(value);
	} return 1;
}


static char *trim_space(char *str)
{
	char *end;

	while (isspace((unsigned char) *str)) {
		str++;
	}

	end = str + strlen(str) - 1;
	while (end > str && isspace((unsigned char) *end)) {
		end--;
	}

	end[1] = '\0';

	return str;
}

static char *trim_comment(char *str)
{
	char *s = str;

	if (*s == '\0' || *s == ';' || *s == '#') {
		*s = '\0';
		return str;
	}

	while (*s && (s = strchr(s, ' ')) != NULL) {
		s++;
		if (*s == ';' || *s == '#') {
			*s = '\0';
			break;
		}
	}

	return str;
}

static char *trim_bom(char *str)
{
	unsigned char *p = (unsigned char *) str;

	if (strlen(str) >= 3) {
		if (p[0] == 0xEF && p[1] == 0xBB && p[2] == 0xBF) {
			str += 3;
		}
	}

	return str;
}

int ini_parse(void *arg, ini_on_item cb, void *arg1,
		 char *(*next_line)(void *, char *, size_t))
{
	int rc = 0, line = 0;
	char buf[INI_MAX_LINE_LEN];
	char section[256] = {0}, key[256] = {0};
	char *head, *end;

	while ((head = next_line(arg1, buf, sizeof(buf) - 1)) != NULL) {
		if (++line == 1) {
			head = trim_bom(head);
		}

		head = trim_space(trim_comment(head));
		if (*head == '\0') {
			continue;
		}

		if (head > buf && *key) {
			rc = cb(arg, line, section, key, head);
		} else if (*head == '[') {
			if ((end = strchr(head, ']')) == NULL) {
				return line;
			}

			*key = '\0';
			*end = '\0';
			strncpy(section, head + 1, sizeof(section) - 1);
		} else {
			if ((end = strpbrk(head, "=:")) == NULL) {
				return line;
			}

			*end = '\0';
			trim_space(head);
			strncpy(key, head, sizeof(key) - 1);
			rc = cb(arg, line, section, head, trim_space(end + 1));
		}

		if (rc != 0) {
			return line;
		}
	}

	return 0;
}

static char *file_line(void *p, char *buf, size_t size)
{
	return fgets(buf, (int) size, (FILE *) p);
}

static char *string_line(void *p, char *buf, size_t size)
{
	size_t len, diff;
	char *t, *str = (*(char **) p);

	if (str == NULL || *str == '\0') {
		return NULL;
	}

	t = strchr(str, '\n');
	if (t == NULL) {
		t = str + strlen(str);
	}

	diff = (size_t) (t - str);
	len = diff < size ? diff : size;
	memcpy(buf, str, len);
	buf[len] = '\0';

	*(char **) p = (*t == '\0') ? '\0' : t + 1;

	return buf;
}

int ini_parse_file(void *arg, ini_on_item cb, const char *filename)
{
	int rc;
	FILE *file;

	file = fopen(filename, "rb");
	if (!file) {
		return -1;
	}

	rc = ini_parse(arg, cb, file, file_line);
	if (rc == 0) {
		rc = ferror(file) != 0 ? -1 : 0;
	}

	fclose(file);

	return rc;
}

int ini_parse_string(void *arg, ini_on_item cb, const char *str)
{
	char *ptr = (char *) str;
	return ini_parse(arg, cb, &ptr, string_line);
}

#endif
#endif