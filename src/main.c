#include "schemehandler.h"
#include <stdio.h>

void* handle_url_request(void* data, char* value) {
    printf("%s\n", value);
}

int main(int argc, char* argv[]) {
    scheme_handler* handler = app_open(argc, argv, "");
    if (handler) {
        // This is the first instance of application, listen for callback and continue with the application
        listen_callback(handler, &handle_url_request, NULL);
        printf("Hello World\n");
        getchar();
    } return 0;
}