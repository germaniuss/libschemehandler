#ifndef PIPE_H
#define PIPE_H

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#else
#include <sys/stat.h>
#include <fcntl.h>
#endif

#define PIPE_VERSION "1.0.0"

typedef struct my_pipe {
    char* name;
    #if defined(_WIN32) || defined(_WIN64)
    HANDLE hPipe;
    #elif defined(__linux__)
    int fd;
    #endif
} my_pipe;

void pipe_create(my_pipe* pipe, const char* name, unsigned int mode);
void pipe_open(my_pipe* pipe, const char* name, const char* read_write);
char* pipe_read(my_pipe* pipe, char buf[], unsigned int size);
void pipe_write(my_pipe* pipe, const char* str);
void pipe_close(my_pipe* pipe);

#endif