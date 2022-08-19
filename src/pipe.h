#ifndef PIPE_H
#define PIPE_H

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#else
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/file.h>
#include <errno.h>
#define READONLY O_RDONLY
#define WRITEONLY O_WRONLY
#define READWRITE O_RDWR
#define EXCLUSIVE O_EXCL
#define CREATE O_CREAT
#endif

#define PIPE_VERSION "1.0.0"

typedef struct file_desc {
    char* name;
    #if defined(_WIN32) || defined(_WIN64)
    HANDLE hPipe;
    #elif defined(__linux__)
    int fd;
    #endif
} file_desc;

void pipe_create(file_desc* pipe, const char* name);
void pipe_open(file_desc* pipe, const char* name, int mode);

int file_open(file_desc* pipe, const char* name, int mode, int lock);
char* file_read(file_desc* pipe, char buf[], unsigned int size);
void file_write(file_desc* pipe, const char* str);
void file_close(file_desc* pipe);

#endif