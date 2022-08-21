#ifndef PIPE_H
#define PIPE_H

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#define READONLY GENERIC_READ
#define WRITEONLY GENERIC_WRITE
#define READWRITE GENERIC_WRITE | GENERIC_READ
#else
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/file.h>
#include <errno.h>
#define READONLY O_RDONLY
#define WRITEONLY O_WRONLY
#define READWRITE O_RDWR
#endif

#define PIPE_VERSION "1.0.0"

typedef struct file_desc {
    char* name;
    #if defined(_WIN32) || defined(_WIN64)
    HANDLE hPipe;
    int isReadPipe;
    #elif defined(__linux__)
    int fd;
    #endif
} file_desc;

void pipe_create(file_desc* pipe, const char* name);
int pipe_open(file_desc* pipe, int mode);
void pipe_close(file_desc* pipe);

int file_open(file_desc* pipe, const char* name, int mode, int lock);
char* file_read(file_desc* pipe, char buf[], unsigned int size);
void file_write(file_desc* pipe, const char* str);
void file_close(file_desc* pipe);

#endif