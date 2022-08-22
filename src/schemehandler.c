#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "schemehandler.h"
#include "option.h"
#include "str.h"
#include "ini.h"
#include "path.h"
#include "pipe.h"

typedef struct app_args {
    bool terminal;
    char* launch;
    char* executable;
    char* scheme;
    char* config_dir;
} app_args;

typedef struct scheme_handler {
    file_desc pipe;
    pthread_t th;
    void* data;
    void* (*callback)(void* data, const char* endpoint, const char* query);
} scheme_handler;

static struct option_item options[] = {{.letter = 'l', .name = "uri-launch"},
                                       {.letter = 'r', .name = "uri-register"},
                                       {.letter = 'h', .name = "help"}};

bool scheme_register(const char* protocol, const char* exec, bool terminal) {
    #if defined(__linux__)
    // open file /usr/share/applications/[protocol].desktop and write the string
    char* full_dir = str_create_fmt("%s/.local/share/applications/%s.desktop", getenv("HOME"), protocol);
    FILE *fp = fopen(full_dir, "w");
    str_destroy(&full_dir);
    if (fp == NULL) return NULL;
    fprintf(fp, "[Desktop Entry]\nExec=%s --uri-launch=%%U\nMimeType=x-scheme-handler/%s;\nName=%s\nTerminal=%s\nType=Application", exec, protocol, protocol, terminal ? "true" : "false");
    fclose(fp);

    // add the uri scheme
    char* command = str_create_fmt("xdg-mime default %s.desktop x-scheme-handler/%s", protocol, protocol);
    system(command);
    str_destroy(&command);
    
    #elif defined(_WIN32) || defined(_WIN64)
    HKEY key;
    HKEY subdir;
    LPCTSTR value;

    if (RegCreateKeyEx(HKEY_CLASSES_ROOT, TEXT(protocol), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &key, NULL) != ERROR_SUCCESS) {
        printf("Error %u ", (unsigned int)GetLastError());
        return false;
    }

    value = TEXT("URL: URL Protocol");
    RegSetValueEx(key, NULL, 0, REG_SZ, (LPBYTE)value, (_tcslen(value)+1) * sizeof(TCHAR));
    value = TEXT("");
    RegSetValueEx(key, TEXT("URL Protocol"), 0, REG_SZ, (LPBYTE)value, (_tcslen(value)+1) * sizeof(TCHAR));
    RegCreateKeyEx(key, TEXT("shell\\open\\command"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &subdir, NULL);
    RegCloseKey(key);
    char* val = str_create_fmt("\"%s\" --uri-launch=\"%%1\"", exec);
    value = TEXT(val);
    RegSetValueEx(subdir, NULL, 0, REG_SZ, (LPBYTE)value, (_tcslen(value)+1) * sizeof(TCHAR));
    RegCloseKey(subdir);
    str_destroy(&val);
    return true;

    #elif defined(TARGET_OS_MAC) || defined(__MAC__)
    return false;

    #else
    return false;

    #endif
}

bool scheme_open(const char* url) {
    char* command;
    #if defined(_WIN32) || defined(_WIN64)
    command = str_create_fmt("explorer %s", url);
    #elif defined(TARGET_OS_MAC) || defined(__MACH__)
    command = str_create_fmt("open %s", url);
    #elif defined(__linux__)
    command = str_create_fmt("xdg-open %s", url);
    #endif
    system(command);
    str_destroy(&command);
    return true;
}

void* thread_task(scheme_handler* handler) {
    while (true) {
        pipe_open(&handler->pipe, READONLY);
        char buf[FILENAME_MAX];
        file_read(&handler->pipe, buf, FILENAME_MAX);
        pipe_close(&handler->pipe);
        strtok(buf, "/");
        #if defined(_WIN32) || defined(_WIN64)
        char* endpoint = strtok(NULL, "/");
        char* query = strtok(NULL, "?");
        #else
        char* endpoint = strtok(strtok(NULL, "/"), "?");
        char* query = strtok(NULL, "?");
        #endif
        handler->callback(handler->data, endpoint, query);
    }
}

int process_ini(void *arg, int line, const char *section, const char *key, const char *value) {
    app_args* args = arg;
    if (strcmp(section, "SchemeHandler")) return 0;
    if (!strcmp(key, "scheme")) args->scheme = str_create(value);
    else if (!strcmp(key, "terminal")) args->terminal = strcmp(value, "false") ? true: false;
    return 0;
}

scheme_handler* app_open(int argc, char* argv[], const char* dir, const char* name, void* (*callback)(void* data, const char* endpoint, const char* query), void* data) {
    // Load the config
    app_args args = {
        .launch = NULL, 
        .executable = getexecname(),
        .scheme = NULL, 
        .terminal = true,
        .config_dir = getexecdir()
    };

    path_add(&args.config_dir, dir);
    path_add(&args.config_dir, name);
    str_append(&args.config_dir, ".ini");
    ini_parse_file(&args, process_ini, args.config_dir);

    // Read the inputed options
    struct option opt = {
        .argv = argv,
        .count = sizeof(options) / sizeof(options[0]),
        .options = options
    };

    char *value;
    for (int i = 1; i < argc; i++) {
        switch (option_at(&opt, i, &value)) {
        case 'l':
            args.launch = str_create(value);
            break;
        case 'r':
            if (scheme_register(args.scheme, args.executable, args.terminal))
                printf("Successfully registered uri scheme\n");
            else printf("Failed to configure the uri scheme\n");
            return NULL;
        case '?':
            printf("Unknown option : %s \n", argv[i]);
            return NULL;
        }
    }

    // Create the piping and the file lock
    file_desc lock;
    char* lock_name = getexecdir();
    path_add(&lock_name, "pid.lock");
    int opened = file_open(&lock, lock_name, READONLY, true);
    str_destroy(&lock_name);

    if (opened) {
        scheme_handler* handler = (scheme_handler*) malloc(sizeof(scheme_handler));
        handler->data = data;
        handler->callback = callback;
        pipe_create(&handler->pipe, args.scheme);
        pthread_create(&handler->th, NULL, &thread_task, handler);
        if (args.launch) {
            pipe_open(&handler->pipe, WRITEONLY);
            file_write(&handler->pipe, args.launch);
            pipe_close(&handler->pipe);
        } return handler;
    } else if (args.launch) {
        file_desc pipe;
        pipe_create(&pipe, args.scheme);
        pipe_open(&pipe, WRITEONLY);
        file_write(&pipe, args.launch);
        pipe_close(&pipe);
        return NULL;
    }
}