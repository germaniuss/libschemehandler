#include "schemehandler.h"
#include <stdio.h>

void* handle_url_request(void* data, char* value) {
    printf("%s\n", value);
}

int main(int argc, char* argv[]) {
    scheme_handler* handler = app_open(argc, argv, "", &handle_url_request, NULL);
    if (handler) {
        getchar();
    } return 0;
}