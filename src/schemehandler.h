#ifndef SCHEME_HANDLER_H
#define SCHEME_HANDLER_H

#include <stdbool.h>
#include <pthread.h>

#if defined(_WIN32) || defined(_WIN64)
#include <tchar.h>
#else

#endif

#define SCHEME_HANDLER_VERSION "0.7.2"

typedef struct scheme_handler scheme_handler;

bool scheme_register(const char* protocol, const char* exec, bool terminal);
bool scheme_open(const char* url);
scheme_handler* app_open(int argc, char* argv[], const char* dir, void* (*callback)(void* data, const char* value), void* data);

#endif