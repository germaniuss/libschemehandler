#ifndef PATH_H
#define PATH_H

#define PATH_VERSION "1.0.0"

char* getexecname();
char* getexecdir();
char* getcurrentdir();
void path_add(char** path, const char* dir);

#endif