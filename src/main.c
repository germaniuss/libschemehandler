#include "schemehandler.h"
#include <stdio.h>

void* handle_url_request(void* data, char* value) {
    printf("%s\n", value);
}

int main(int argc, char* argv[]) {
    scheme_handler* handler = app_open(argc, argv, "");
    if (handler) {
        pthread_t* th = listen_callback(handler, &handle_url_request, NULL);
        getchar();
    } return 0;
}