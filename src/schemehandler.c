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
    void* (*callback)(void* data, const char* value);
} callback_data;

typedef struct app_args {
    bool regist;
    bool terminal;
    char* launch;
    char* executable;
    char* scheme;
} app_args;

static struct option_item options[] = {{.letter = 'l', .name = "uri-launch"},
                                       {.letter = 'r', .name = "uri-register"},
                                       {.letter = 'h', .name = "help"}};

bool scheme_register_linux(const char* protocol, const char* handler, bool terminal) {
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

bool scheme_register_windows(const char* protocol, const char* handler, bool terminal) {
    return false;
}

bool scheme_register_mac(const char* protocol, const char* handler, bool terminal) {
    return false;
}

bool scheme_register(const char* protocol, const char* handler, bool terminal) {
    #if defined(__linux__)
    return scheme_register_linux(protocol, handler, terminal);
    #elif defined(_WIN32) || defined(_WIN64)
    return scheme_register_windows(protocol, handler, terminal);
    #elif defined(TARGET_OS_MAC) || defined(__MACH__)
    return scheme_register_mac(protocol, handler, terminal);
    #endif
    return false;
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
    char* key; char* value;

    FILE *fp = fopen(dir, "w");
    if (fp == NULL) return NULL;

    fprintf(fp, "registered=%s\n", args->regist ? "true" : "false");
    fprintf(fp, "scheme=%s\n", args->scheme);
    fprintf(fp, "terminal=%s\n", args->terminal ? "true" : "false");
        
    fclose(fp);
    return 1;
}

void* thread_task(void* arg) {
    scheme_handler* handler = (scheme_handler*) arg;
    char buf[FILENAME_MAX];
    while (true) {
        while (handler->info);
        file_desc pipe;
        pipe_open(&pipe, handler->pipe_name, READONLY);
        char buf[FILENAME_MAX];
        file_read(&pipe, buf, FILENAME_MAX);
        file_close(&pipe);
        handler->value = str_create(buf);
        handler->info = true;
    }
}

scheme_handler* app_open(int argc, char* argv[], const char* dir) {
    
    scheme_handler* handler = (scheme_handler*) malloc(sizeof(scheme_handler));
    handler->info = false;

    app_args args;
    args.launch = "";
    args.scheme = "";
    args.regist = true;
    args.terminal = true;
    args.executable = getexecname();
    printf("%s\n", args.executable);
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

    file_desc pipe;
    pipe_create(&pipe, "myfifo");

    file_desc lock;
    if(file_open(&lock, args.executable, READONLY | EXCLUSIVE | O_NONBLOCK, true)) {
        if (!*args.launch) return NULL;
        // this is the second instance
        pipe_open(&pipe, pipe.name, WRITEONLY);
        file_write(&pipe, args.launch);
        file_close(&pipe);
        return NULL;
    } else {
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

        handler->pipe_name = pipe.name;
        pthread_create(&handler->th, NULL, &thread_task, handler);

        if (*args.launch) {
            pipe_open(&pipe, pipe.name, WRITEONLY);
            file_write(&pipe, args.launch);
            file_close(&pipe);
        }

        return handler;
    }
}

// recreate this in any language (only here for testing)
void uri_handle_loop(void* in) {
    callback_data* data = (callback_data*) in;
    scheme_handler* handler = data->handler;
    while (true) {
        while (!handler->info);
        data->callback(data->data, handler->value);
        handler->info = false;
    } // free handler and free data
}

// recreate this in any language (only here for testing)
pthread_t listen_callback(scheme_handler* handler, void* (*callback)(void* data, const char* value), void* in) {
    callback_data* data = (callback_data*) malloc(sizeof(callback_data));
    data->data = in;
    data->handler = handler;
    data->callback = callback;
    pthread_t ptr;
    pthread_create(&ptr, NULL, &uri_handle_loop, data);
    return ptr;
}