#include "pipe.h"
#include <string.h>

#if defined(_WIN32) || defined(_WIN64)

void pipe_create(file_desc* pipe, const char* name) {
    pipe->name = (char*) malloc(strlen(name) + 10);
    memcpy(pipe->name, "\\\\.\\pipe\\", 10);
    strcat(pipe->name, name);
    pipe->hPipe = CreateNamedPipe(
        TEXT(pipe->name),
        PIPE_ACCESS_DUPLEX,
        PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
        1,
        1024 * 16,
        1024 * 16,
        NMPWAIT_USE_DEFAULT_WAIT,
        NULL
    );
}

int pipe_open(file_desc* pipe, int mode) {
    if (mode == READONLY) {
        pipe->isReadPipe = TRUE;
        return ConnectNamedPipe(pipe->hPipe, NULL);
    } return file_open(pipe,  pipe->name, mode, 1);
}

void pipe_close(file_desc* pipe) {
    if (pipe->isReadPipe) {
        DisconnectNamedPipe(pipe->hPipe);
    } else file_close(pipe);
}

int file_open(file_desc* pipe, const char* name, int mode, int lock) {
    if (!lock) pipe->hPipe = CreateFile(TEXT(name), mode,  FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, OPEN_ALWAYS, 0, NULL);
    else pipe->hPipe = CreateFile(TEXT(name), mode,  0, NULL, OPEN_ALWAYS, 0, NULL);
    return pipe->hPipe != INVALID_HANDLE_VALUE;
}

char* file_read(file_desc* pipe, char buf[], unsigned int size) {
    DWORD dwRead;
    ReadFile(pipe->hPipe, buf, size, &dwRead, NULL);
}

void file_write(file_desc* pipe, const char* str) {
    DWORD dwWritten;
    WriteFile(pipe->hPipe,str, strlen(str) + 1, &dwWritten, NULL);
}

void file_close(file_desc* pipe) {
    CloseHandle(pipe->hPipe);
}

#else

char* get_full_name(const char* name) {
    char* full_name = (char*) malloc(strlen(name) + 6);
    memcpy(full_name, "/tmp/", 6);
    strcat(full_name, name);
    return full_name;
}

void pipe_create(file_desc* pipe, const char* name) {
    pipe->name = get_full_name(name);
    mkfifo(pipe->name, 0666);
}

int pipe_open(file_desc* pipe, int mode) {
    return file_open(pipe, pipe->name, mode, 0);
}

void pipe_close(file_desc* pipe) {
    file_close(pipe);
}

int file_open(file_desc* pipe, const char* name, int mode, int lock) {
    pipe->fd = open(name, mode, 0666);
    if (lock) return (!flock(pipe->fd, LOCK_EX | LOCK_NB) || EWOULDBLOCK != errno);
    return pipe->fd != -1;
}

char* file_read(file_desc* pipe, char buf[], unsigned int size) {
    read(pipe->fd, buf, size);
}

void file_write(file_desc* pipe, const char* str) {
    write(pipe->fd, str, strlen(str) + 1);
}

void file_close(file_desc* pipe) {
    close(pipe->fd);
}

#endif