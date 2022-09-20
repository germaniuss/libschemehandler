#ifndef SCHEME_HANDLER_H
#define SCHEME_HANDLER_H

#include <utils/option.h>
#include <utils/str.h>
#include <utils/ini.h>
#include <utils/path.h>
#include <utils/pipe.h>

#include <stdbool.h>
#include <pthread.h>

#if defined(_WIN32) || defined(_WIN64)
#include <tchar.h>
#endif

typedef struct scheme_handler scheme_handler;

bool scheme_register(const char* protocol, const char* exec, bool terminal);
bool scheme_open(const char* url);
scheme_handler* app_open(int argc, char* argv[], const char* dir, const char* name, void* (*callback)(void* data, const char* endpoint, const char* query), void* data);

#endif