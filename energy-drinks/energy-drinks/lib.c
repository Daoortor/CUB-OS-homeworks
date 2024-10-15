#include "lib.h"

#include <bits/getopt_core.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct error_info error_info = {'\0', {}};

int safe_parse_ulong_option(char flag, ulong *result) {
    errno = 0;
    char *endptr = 0;
    *result = strtoul(optarg, &endptr, 10);
    if (*endptr != '\0') {
        error_info.error_option = flag;
        strncpy(error_info.error_arg, optarg, BUFFER_SIZE - 1);
        return INVALID_ARGUMENT_VALUE;
    }
    if (errno == EINVAL) {
        error_info.error_option = flag;
        strncpy(error_info.error_arg, optarg, BUFFER_SIZE - 1);
        return INVALID_ARGUMENT_VALUE;
    }
    if (errno == ERANGE) {
        error_info.error_option = flag;
        strncpy(error_info.error_arg, optarg, BUFFER_SIZE - 1);
        return ARGUMENT_OUT_OF_RANGE;
    }
    return SUCCESS;
}

int process_error(char **argv, error error) {
    switch (error) {
    case INVALID_OPTION:
        return EXIT_FAILURE;
    case ARGUMENT_OUT_OF_RANGE:
        fprintf(stderr, "%s: -%c: argument %s out of range\n", argv[0],
                error_info.error_option, error_info.error_arg);
        return EXIT_FAILURE;
    case INVALID_ARGUMENT_VALUE:
        fprintf(stderr, "%s: -%c: invalid argument %s\n", argv[0],
                error_info.error_option, error_info.error_arg);
        return EXIT_FAILURE;
    case UNEXPECTED_ARGUMENT:
        fprintf(stderr, "%s: unexpected argument %s\n", argv[0],
                error_info.error_arg);
        return EXIT_FAILURE;
    case INSUFFICIENT_RESOURCES:
        fprintf(stderr, "%s: insufficient resources\n", argv[0]);
        return EXIT_FAILURE;
    default:
        return EXIT_SUCCESS;
    }
}
