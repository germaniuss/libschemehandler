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

typedef struct callback_data {
    scheme_handler* handler;
    void* data;
    void* (*callback)(void* data, const char* endpoint, const char* query);
} callback_data;

typedef struct app_args {
    bool regist;
    bool terminal;
    char* launch;
    char* executable;
    char* scheme;
} app_args;

typedef struct scheme_handler {
    file_desc pipe;
    pthread_t th;
} scheme_handler;

static struct option_item options[] = {{.letter = 'l', .name = "uri-launch"},
                                       {.letter = 'r', .name = "uri-register"},
                                       {.letter = 'h', .name = "help"}};

#if defined(__linux__)
bool scheme_register(const char* protocol, const char* exec, bool terminal) {
    // open file /usr/share/applications/[protocol].desktop and write the string
    char* full_dir = str_create_fmt("%s/.local/share/applications/%s.desktop", getenv("HOME"), protocol);
    FILE *fp = fopen(full_dir, "w");
    str_destroy(&full_dir);
    if (fp == NULL) return NULL;
    fprintf(fp, "[Desktop Entry]\nExec=%s --uri-launch=%%U\nMimeType=x-scheme-handler/%s;\nName=%s\nTerminal=%s\nType=Application", handler, protocol, protocol, terminal ? "true" : "false");
    fclose(fp);

    // add the uri scheme
    char* command = str_create_fmt("xdg-mime default %s.desktop x-scheme-handler/%s", protocol, protocol);
    system(command);
    str_destroy(&command);
}
#elif defined(_WIN32) || defined(_WIN64)
bool scheme_register(const char* protocol, const char* exec, bool terminal) {
    HKEY key;
    HKEY subdir;
    LPCTSTR value;

    if (RegCreateKeyEx(HKEY_CLASSES_ROOT, TEXT(protocol), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &key, NULL) != ERROR_SUCCESS) {
        printf("Error %u ", (unsigned int)GetLastError());
        return false;
    }

    value = TEXT("URL: URL Protocol");
    if (RegSetValueEx(key, NULL, 0, REG_SZ, (LPBYTE)value, (_tcslen(value)+1) * sizeof(TCHAR)) != ERROR_SUCCESS) {
        printf("Error %u ", (unsigned int)GetLastError());
        RegCloseKey(key);
        return false;
    }

    value = TEXT("");
    if (RegSetValueEx(key, TEXT("URL Protocol"), 0, REG_SZ, (LPBYTE)value, (_tcslen(value)+1) * sizeof(TCHAR)) != ERROR_SUCCESS) {
        printf("Error %u ", (unsigned int)GetLastError());
        RegCloseKey(key);
        return false;
    }

    if (RegCreateKeyEx(key, TEXT("shell\\open\\command"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &subdir, NULL) != ERROR_SUCCESS) {
        printf("Error %u ", (unsigned int)GetLastError());
        RegCloseKey(key);
        return false;
    }

    RegCloseKey(key);
    char* val = str_create_fmt("\"%s\" --uri-launch=\"%%1\"", exec);
    value = TEXT(val);
    if (RegSetValueEx(subdir, NULL, 0, REG_SZ, (LPBYTE)value, (_tcslen(value)+1) * sizeof(TCHAR)) != ERROR_SUCCESS) {
        RegCloseKey(subdir);
        printf("Error %u ", (unsigned int)GetLastError());
        return false;
    } RegCloseKey(subdir);
    str_destroy(&val);

    printf("Key changed in registry \n");
    return true;
}
#elif defined(TARGET_OS_MAC) || defined(__MAC__)
bool scheme_register(const char* protocol, const char* exec, bool terminal) {
    return false;
}
#else
bool scheme_register(const char* protocol, const char* exec, bool terminal) {
    return false;
}
#endif

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

int process_ini(void *arg, int line, const char *section, const char *key, const char *value) {
    app_args* args = arg;
    if (!strcmp(key, "scheme")) args->scheme = str_create(value);
    else if (!strcmp(key, "registered")) args->regist = strcmp(value, "false") ? true: false;
    else if (!strcmp(key, "terminal")) args->terminal = strcmp(value, "false") ? true: false;
    return 0;
}

int load(app_args* args, const char* dir) {
    int rc = ini_parse_file(args, process_ini, dir);
    return rc;
}

bool save(app_args* args, const char* dir) {
    FILE *fp = fopen(dir, "w");
    if (fp == NULL) return NULL;

    fprintf(fp, "registered=%s\n", args->regist ? "true" : "false");
    fprintf(fp, "scheme=%s\n", args->scheme);
    fprintf(fp, "terminal=%s\n", args->terminal ? "true" : "false");
        
    fclose(fp);
    return 1;
}

void* thread_task(void* arg) {
    callback_data* data = (callback_data*) arg;
    scheme_handler* handler = data->handler;
    while (true) {
        pipe_open(&handler->pipe, READONLY);
        char buf[FILENAME_MAX];
        file_read(&handler->pipe, buf, FILENAME_MAX);
        pipe_close(&handler->pipe);
        char* value = str_create(buf);
        char* save = NULL;
        str_token_begin(value, &save, "://");
        const char* token = str_token_begin(value, &save, "?");
        char* endpoint = str_create(token);
        const char* token = str_token_begin(value, &save, "?");
        char* query = str_create(token);
        data->callback(data->data, endpoint, query);
        str_destroy(&value);
        str_destroy(&endpoint);
        str_destroy(&query);
    }
}

scheme_handler* app_open(int argc, char* argv[], const char* dir, void* (*callback)(void* data, const char* endpoint, const char* query), void* data) {
    
    scheme_handler* handler = (scheme_handler*) malloc(sizeof(scheme_handler));
    callback_data* callback_dat = (callback_data*) malloc(sizeof(callback_data));
    callback_dat->callback = callback;
    callback_dat->handler = handler;
    callback_dat->data = data;

    app_args args;
    args.launch = "";
    args.scheme = "";
    args.regist = true;
    args.terminal = true;
    args.executable = getexecname();
    char* config_dir = getexecdir();
    path_add(&config_dir, dir);
    path_add(&config_dir, "scheme_config.ini");
    
    load(&args, config_dir);

    struct option opt = {.argv = argv,
                         .count = sizeof(options) / sizeof(options[0]),
                         .options = options};

    char *value;
    for (int i = 1; i < argc; i++) {
        char c = option_at(&opt, i, &value);
        switch (c) {
        case 'l':
            args.launch = str_create(value);
            break;
        case '?':
            printf("Unknown option : %s \n", argv[i]);
            break;
        }
    }

    pipe_create(&handler->pipe, "myfifo");

    char* lock_name = getexecdir();
    path_add(&lock_name, "mylock.lock");

    file_desc lock;
    if(file_open(&lock, lock_name, READONLY, true)) {
        str_destroy(&lock_name);
        // this is the first instance
        if (!args.regist) {
            if (scheme_register(args.scheme, args.executable, args.terminal)) {
                printf("Successfully registered uri scheme\n");
                args.regist = true;
                save(&args, config_dir);
            } else {
                printf("Failed to configure the uri scheme\n");
                return NULL;
            }
        }

        pthread_create(&handler->th, NULL, &thread_task, callback_dat);

        if (*args.launch) {
            pipe_open(&handler->pipe, WRITEONLY);
            file_write(&handler->pipe, args.launch);
            pipe_close(&handler->pipe);
        } return handler;
    } else {
        str_destroy(&lock_name);
        if (!*args.launch) return NULL;
        // this is the second instance
        pipe_open(&handler->pipe, WRITEONLY);
        file_write(&handler->pipe, args.launch);
        pipe_close(&handler->pipe);
        return NULL;
    }
}