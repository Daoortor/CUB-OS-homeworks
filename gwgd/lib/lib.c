#include "lib.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <bits/getopt_core.h>

int INVALID_ARGUMENT_VALUE = 1;
int ARGUMENT_OUT_OF_RANGE = 2;

int process_error(int error, struct error_handler *error_handlers) {
    for (struct error_handler *cur = error_handlers; cur->error != -1; cur++) {
        if (cur->error == error) {
            cur->handle();
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}

int safe_parse_ulong_option(char flag, unsigned long *result) {
    errno = 0;
    char *endptr = 0;
    *result = strtoul(optarg, &endptr, INT_BASE);
    if (*endptr != '\0') {
        *error_option = flag;
        strncpy(*error_arg, optarg, BUFFER_SIZE - 1);
        return INVALID_ARGUMENT_VALUE;
    }
    if (errno == EINVAL) {
        *error_option = flag;
        strncpy(*error_arg, optarg, BUFFER_SIZE - 1);
        return INVALID_ARGUMENT_VALUE;
    }
    if (errno == ERANGE) {
        *error_option = flag;
        strncpy(*error_arg, optarg, BUFFER_SIZE - 1);
        return ARGUMENT_OUT_OF_RANGE;
    }
    return EXIT_SUCCESS;
}
