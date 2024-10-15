#include "dpnum.h"

#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

unsigned long power[BASE][UL_SIZE];
unsigned long max_power[BASE];

struct config config = {1, 10000, 1, false, 0, {}};

int safe_parse_ulong_option(char flag, unsigned long *result) {
    errno = 0;
    char *endptr = 0;
    *result = strtoul(optarg, &endptr, BASE);
    if (*endptr != '\0') {
        config.error_option = flag;
        strncpy(config.error_arg, optarg, BUFFER_SIZE - 1);
        return INVALID_ARGUMENT_VALUE;
    }
    if (errno == EINVAL) {
        config.error_option = flag;
        strncpy(config.error_arg, optarg, BUFFER_SIZE - 1);
        return INVALID_ARGUMENT_VALUE;
    }
    if (errno == ERANGE) {
        config.error_option = flag;
        strncpy(config.error_arg, optarg, BUFFER_SIZE - 1);
        return ARGUMENT_OUT_OF_RANGE;
    }
    return SUCCESS;
}

int parse_flags(int argc, char **argv) {
    char flag;
    while ((flag = getopt(argc, argv, "s:e:t:v")) != -1) {
        int code;
        switch (flag) {
            case 's':
                code = safe_parse_ulong_option('s', &config.s);
                if (code != SUCCESS) {
                    return code;
                }
                break;
            case 'e':
                code = safe_parse_ulong_option('e', &config.e);
                config.e++; // Set to one after the end for convenience
                if (code != SUCCESS) {
                    return code;
                }
                break;
            case 't':
                code = safe_parse_ulong_option('t', &config.t);
                if (code != SUCCESS) {
                    return code;
                }
                break;
            case 'v':
                config.v = true;
                break;
            default:
                return INVALID_OPTION;
        }
    }
    if (config.s > config.e) {
        return INVALID_RANGE;
    }
    if (optind < argc) {
        strncpy(config.error_arg, argv[optind], BUFFER_SIZE - 1);
        return UNEXPECTED_ARGUMENT;
    }
    return SUCCESS;
}

int process_error(char *argv[], enum error error) {
    switch (error) {
        case INVALID_OPTION:
            return EXIT_FAILURE;
        case ARGUMENT_OUT_OF_RANGE:
            fprintf(stderr, "%s: -%c: argument %s out of range\n", argv[0], config.error_option, config.error_arg);
            return EXIT_FAILURE;
        case INVALID_ARGUMENT_VALUE:
            fprintf(stderr, "%s: -%c: invalid argument %s\n", argv[0], config.error_option, config.error_arg);
            return EXIT_FAILURE;
        case UNEXPECTED_ARGUMENT:
            fprintf(stderr, "%s: unexpected argument %s\n", argv[0], config.error_arg);
            return EXIT_FAILURE;
        case INVALID_RANGE:
            fprintf(stderr, "%s: invalid range: %lu..%lu\n", argv[0], config.s, config.e);
            return EXIT_FAILURE;
        case INSUFFICIENT_MEMORY:
            fprintf(stderr, "%s: not enough memory", argv[0]);
            return EXIT_FAILURE;
        case THREAD_ERROR:
            fprintf(stderr, "%s: unknown thread error", argv[0]);
            return EXIT_FAILURE;
        default:
            return EXIT_SUCCESS;
    }
}

void precompute_powers() {
    max_power[0] = ULONG_MAX;
    for (int digit=1; digit<BASE; digit++) {
        power[digit][0] = 1;
        unsigned long i = 1;
        for (; i < UL_SIZE && power[digit][i-1] < ULONG_MAX / 2; i++) {
            power[digit][i] = power[digit][i-1] * digit;
        }
        max_power[digit] = i - 1;
    }
}

unsigned long compute_power_sum(char *digits, unsigned long exponent) {
    char *c = digits;
    long long result = 0;
    for (; *c!='\0'; c++) {
        int digit = *c - '0';
        if (exponent > max_power[digit]) {
            return 0;
        }
        result += power[digit][exponent];
    }
    return result;
}

bool check(const unsigned long n, char *buf) {
    if (n == 0) {
        return false;
    }
    snprintf(buf, UL_SIZE, "%lu", n);
    for (unsigned long exponent=0; exponent<UL_SIZE; exponent++) {
        if (compute_power_sum(buf, exponent) == n) {
            return true;
        }
    }
    return false;
}

int check_range(void *args_ptr) {
    const struct args *args = args_ptr;
    unsigned long *dest_cur = args->dest;
    for (unsigned long n = args->start; n <= args->end && dest_cur != args->dest + args->dest_size - 1; n++) {
        char buf[UL_SIZE];
        if (check(n, buf)) {
            *dest_cur = n;
            dest_cur++;
        }
    }
    *dest_cur = 0;
    return EXIT_SUCCESS;
}

int run_thread(char **argv, int thread_index, unsigned long *dest, thrd_t *thread_id, struct args *args_buf) {
    args_buf->start = config.s + (config.e - config.s) * thread_index / config.t;
    args_buf->end = config.s + (config.e - config.s) * (thread_index + 1) / config.t - 1;
    args_buf->dest = dest;
    args_buf->dest_size = MAX_PDI_NUMBER_COUNT;
    if (config.v) {
        fprintf(stderr, "%s: t%i searching [%lu, %lu]\n", argv[0], thread_index, args_buf->start, args_buf->end);
    }
    const int code = thrd_create(thread_id, &check_range, args_buf);
    if (code == thrd_nomem) {
        return INSUFFICIENT_MEMORY;
    }
    if (code == thrd_error) {
        return THREAD_ERROR;
    }
    return EXIT_SUCCESS;
}

int main(int argc, char **argv) {
    precompute_powers();
    SAFE_CALL(parse_flags(argc, argv));
    unsigned long **results = malloc(sizeof(unsigned long *) * config.t);
    for (unsigned long **p = results; p != results + config.t; p++) {
        *p = malloc(sizeof(unsigned long) * MAX_PDI_NUMBER_COUNT);
    }
    struct args *args_lists = malloc(sizeof(struct args) * config.t);
    thrd_t *threads = malloc(sizeof(thrd_t) * config.t);

    for (int thread_index = 0; thread_index < config.t; thread_index++) {
        SAFE_CALL(run_thread(argv, thread_index, results[thread_index],
            &threads[thread_index], &args_lists[thread_index]));
    }
    for (int thread_index = 0; thread_index < config.t; thread_index++) {
        thrd_join(threads[thread_index], NULL);
        if (config.v) {
            fprintf(stderr, "%s: t%i finishing\n", argv[0], thread_index);
        }
        for (unsigned long *cur = results[thread_index]; cur != results[thread_index] + MAX_PDI_NUMBER_COUNT
            && *cur != 0; cur++) {
            printf("%lu\n", *cur);
        }
    }

    free(threads);
    for (unsigned long **p = results; p != results + config.t; p++) {
        free(*p);
    }
    free(results);
    free(args_lists);
    return EXIT_SUCCESS;
}
