#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "path.h"

#include "str.h"

char* getexecname() {
    // On linux
    char buf[FILENAME_MAX];
    int bytes = readlink("/proc/self/exe", buf, FILENAME_MAX);
    buf[bytes] = 0;
    return str_create(buf);
};

char* getexecdir() {
    // On linux
    char* val = getexecname();
    char * tld = strrchr(val, '/');
    *tld = '\0';
    char* ret = str_create(val);
    str_destroy(&val);
    return ret;
}

char* getcurrentdir() {
    // On linux
    char buf[FILENAME_MAX];
    getcwd(buf, FILENAME_MAX);
    return str_create(buf);
}

void path_add(char** path, const char* dir) {
    if (*dir == '/' || *path == NULL) {
        str_destroy(path);
        *path = str_create(dir);
    } else if (*dir) str_append_fmt(path, "/%s", dir);
}