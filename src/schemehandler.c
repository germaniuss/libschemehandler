#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/file.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "schemehandler.h"
#include "option.h"
#include "str.h"
#include "ini.h"
#include "thread.h"

typedef struct scheme_handler {
    bool info;
    char* value;
    char* pipe_name;
    struct thread* th;
} scheme_handler;

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
    char* full_dir = NULL;
    if (dir[0] != '\0') str_append_fmt(&full_dir, "%s/", dir);
    str_append(&full_dir, "scheme_config.ini");
    int rc = ini_parse_file(args, process_ini, full_dir);
    str_destroy(&full_dir);
    return rc;
}

bool save(app_args* args, const char* dir) {
    char* key; char* value;
    char* full_dir = NULL;
    if (dir[0] != '\0') str_append_fmt(&full_dir, "%s/", dir);
    str_append(&full_dir, "scheme_config.ini");

    FILE *fp = fopen(full_dir, "w");
    str_destroy(&full_dir);
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
        int fd = open(handler->pipe_name, O_RDONLY);
        read(fd, buf, FILENAME_MAX);
        close(fd);
        handler->value = str_create(buf);
        handler->info = true;
    }
}

scheme_handler* app_open(int argc, char* argv[], const char* dir) {
    
    scheme_handler* handler = (scheme_handler*) malloc(sizeof(scheme_handler));
    handler->info = false;
    handler->th = NULL;

    app_args args;
    args.launch = "";
    args.scheme = "";
    args.regist = true;
    args.terminal = true;
    args.executable = get_exec_name(argv[0]);
    char* config_dir = get_local_dir(argv[0]);
    if (*dir) str_append_fmt(&config_dir, "/%s", dir);

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

    char * myfifo = "/tmp/myfifo";
    mkfifo(myfifo, 0666);

    int pid_file = open(args.executable, O_RDONLY | O_EXCL, 0666);
    if(flock(pid_file, LOCK_EX | LOCK_NB) && EWOULDBLOCK == errno && *args.launch) {
        // this is the second instance
        printf("URI launch string is: %s\n", args.launch);
        // pipe the launch arguments to the first process
        int fd = open(myfifo, O_WRONLY);
        write(fd, args.launch, strlen(args.launch) + 1);
        close(fd);
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

        handler->pipe_name = myfifo;
        struct thread* th = (struct thread*) malloc(sizeof(struct thread));
        handler->th = th;
        thread_init(th);
        thread_start(th, thread_task, handler);

        if (*args.launch) {
            int fd = open(myfifo, O_WRONLY);
            write(fd, args.launch, strlen(args.launch) + 1);
            close(fd);
        }

        return handler;
    }
}

// recreate this in any language (only here for testing)
void uri_handle_loop(callback_data* data) {
    scheme_handler* handler = data->handler;
    while (true) {
        while (!handler->info);
        data->callback(data->data, handler->value);
        handler->info = false;
    } // free handler and free data
}

// recreate this in any language (only here for testing)
void listen_callback(scheme_handler* handler, void* (*callback)(void* data, const char* value), void* in) {
    struct thread th;
    callback_data* data = (callback_data*) malloc(sizeof(callback_data));
    data->data = in;
    data->handler = handler;
    data->callback = callback;
    uri_handle_loop(data);
    return th;
}