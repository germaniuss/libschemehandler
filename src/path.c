#include <stdio.h>
#include "path.h"

#include "str.h"

char* get_exec_name(const char* local) {
    char* val = str_create(local);
    char* save = NULL;
    char* execname = NULL;
    char* token = NULL;

    if (*val == '/') return val;

    while (token = str_token_begin(val, &save, "/")) {
        str_destroy(&execname);
        execname = str_create(token);
    } str_destroy(&val);

    char buf[FILENAME_MAX];
    return str_create_fmt("%s/%s", getcwd(buf, FILENAME_MAX), execname);
};

char* get_local_dir(const char* local) {
    char* val = str_create(local);
    char* save = NULL;
    char* execname = NULL;
    char* token = NULL;

    char buf[FILENAME_MAX];
    if (*val != '/') return getcwd(buf, FILENAME_MAX);

    char* returnVal= NULL;
    while (token = str_token_begin(val, &save, "/")) {
        str_destroy(&returnVal);
        returnVal = execname;
        execname = str_create(val);
    } 
    
    str_destroy(&val);
    str_destroy(&execname);

    return returnVal;
}