#include "pipe.h"
#include <string.h>

#if defined(_WIN32) || defined(_WIN64)

char* get_full_name(const char* name) {
    char* full_name = (char*) malloc(strlen(name) + 10);
    memcpy(full_name, "\\\\.\\pipe\\", 10);
    strcat(full_name, name);
    return full_name;
}

void pipe_create(my_pipe* pipe, const char* name, unsigned int mode) {
    char* full_name = get_full_name(name);
    pipe->hPipe = CreateNamedPipe(
        TEXT(full_name),
        PIPE_ACCESS_DUPLEX,
        PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,   // FILE_FLAG_FIRST_PIPE_INSTANCE is not needed but forces CreateNamedPipe(..) to fail if the pipe already exists...
        1,
        1024 * 16,
        1024 * 16,
        NMPWAIT_USE_DEFAULT_WAIT,
        NULL
    ); free(full_name);
}

void pipe_open(my_pipe* pipe, const char* name, const char* read_write) {
    char* full_name = get_full_name(name);
    pipe->hPipe = CreateFile(TEXT(full_name), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL); 
    free(full_name);
}

char* pipe_read(my_pipe* pipe, char buf[], unsigned int size) {
    DWORD dwRead;
    ReadFile(pipe->hPipe, buf, size, &dwRead, NULL);
}

void pipe_write(my_pipe* pipe, const char* str) {
    DWORD dwWritten;
    WriteFile(pipe->hPipe,str, strlen(str) + 1, &dwWritten, NULL);
}

void pipe_close(my_pipe* pipe) {
    CloseHandle(pipe->hPipe);
}

#else

char* get_full_name(const char* name) {
    char* full_name = (char*) malloc(strlen(name) + 6);
    memcpy(full_name, "/tmp/", 6);
    strcat(full_name, name);
    return full_name;
}

void pipe_create(my_pipe* pipe, const char* name, unsigned int mode) {
    char* full_name = get_full_name(name);
    mkfifo(full_name, mode);
    pipe->name = name;
    free(full_name);
}

void pipe_open(my_pipe* pipe, const char* name, const char* read_write) {
    char* full_name = get_full_name(name);
    if (strcmp(read_write, "w") == 0)
        pipe->fd = open(full_name, O_WRONLY);
    else if (strcmp(read_write, "r") == 0)
        pipe->fd = open(full_name, O_RDONLY);
    else if (strcmp(read_write, "wr") == 0)
        pipe->fd = open(full_name, O_RDWR);
    free(full_name);
}

char* pipe_read(my_pipe* pipe, char buf[], unsigned int size) {
    read(pipe->fd, buf, size);
}

void pipe_write(my_pipe* pipe, const char* str) {
    write(pipe->fd, str, strlen(str) + 1);
}

void pipe_close(my_pipe* pipe) {
    close(pipe->fd);
}

#endif