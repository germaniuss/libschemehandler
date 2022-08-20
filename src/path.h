#ifndef PATH_H
#define PATH_H

#define PATH_VERSION "1.0.0"

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#include <direct.h> 
#else
#include <unistd.h>
#endif

char* getexecname();
char* getexecdir();
char* getcurrentdir();
void path_add(char** path, const char* dir);

#endif