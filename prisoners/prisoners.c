#include "prisoners.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#define N_DRAWERS 100
#define N_ATTEMPTS 50
int drawers[N_DRAWERS];

pthread_mutex_t global_lock;
pthread_mutex_t drawer_locks[N_DRAWERS];
struct args args_lists[N_DRAWERS];
bool thread_results[N_DRAWERS];
struct config config = {100, 0};

int safe_parse_ulong_option(char flag, unsigned long *result) {
    errno = 0;
    char *endptr = 0;
    *result = strtoul(optarg, &endptr, 10);
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
    while ((flag = getopt(argc, argv, "n:s:")) != -1) {
        int code;
        switch (flag) {
            case 's':
                code = safe_parse_ulong_option('s', &config.s);
            if (code != SUCCESS) {
                return code;
            }
            break;
            case 'n':
                code = safe_parse_ulong_option('n', &config.n);
            if (code != SUCCESS) {
                return code;
            }
            break;
            default:
                return INVALID_OPTION;
        }
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
        case INSUFFICIENT_RESOURCES:
            fprintf(stderr, "%s: insufficient resources\n", argv[0]);
        default:
            return EXIT_SUCCESS;
    }
}

void init_drawers() {
    for (int i = 0; i < N_DRAWERS; i++) {
        drawers[i] = i;
    }
    for (int i = N_DRAWERS-1; i >= 0; i--) {
        int j = rand() % (i+1);
        int temp = drawers[i];
        drawers[i] = drawers[j];
        drawers[j] = temp;
    }
}

int next_drawer(int index, int seed, bool use_smart_strategy) {
    if (use_smart_strategy) {
        return seed == -1 ? index : drawers[seed];
    }
    return rand() % N_DRAWERS;
}

void *run_prisoner(void *args_ptr) {
    struct args *args = args_ptr;
    int cur = next_drawer(args->index, -1, args->use_smart_strategy);
    if (args->use_global_lock) {
        pthread_mutex_lock(&global_lock);
    } else {
        pthread_mutex_lock(&drawer_locks[cur]);
    }
    for (int i=0; i<N_ATTEMPTS; i++) {
        if (drawers[cur] == args->index) {
            thread_results[args->index] = true;
            pthread_mutex_unlock(&global_lock);
            return NULL;
        }
        int old_cur = cur;
        cur = next_drawer(i, cur, args->use_smart_strategy);
        if (!args->use_global_lock) {
            pthread_mutex_unlock(&drawer_locks[old_cur]);
            pthread_mutex_lock(&drawer_locks[cur]);
        }
    }
    if (args->use_global_lock) {
        pthread_mutex_unlock(&global_lock);
    } else {
        pthread_mutex_unlock(&drawer_locks[cur]);
    }
    return NULL;
}

int simulate(enum strategy strategy, int *success_count, double *time_milliseconds) {
    clock_t start = clock();
    *success_count = 0;
    for (int simulation_number = 0; simulation_number < config.n; simulation_number++) {
        init_drawers();
        pthread_t threads[N_DRAWERS];
        for (int i = 0; i < N_DRAWERS; i++) {
            thread_results[i] = false;
        }
        for (int i = 0; i < N_DRAWERS; i++) {
            struct args *args = &args_lists[i];
            args->index = i;
            args->result = &thread_results[i];
            if (strategy == RANDOM_GLOBAL || strategy == SMART_GLOBAL) {
                args->use_global_lock = true;
            }
            if (strategy == SMART_DRAWER || strategy == SMART_GLOBAL) {
                args->use_smart_strategy = true;
            }
            int code = pthread_create(&threads[i], NULL, run_prisoner, args);
            if (code == EAGAIN) {
                return INSUFFICIENT_RESOURCES;
            }
        }
        for (int i = 0; i < N_DRAWERS; i++) {
            pthread_join(threads[i], NULL);
        }
        bool success = true;
        for (int i = 0; i < N_DRAWERS; i++) {
            if (!thread_results[i]) {
                success = false;
                break;
            }
        }
        if (success) {
            (*success_count)++;
        }
    }
    clock_t end = clock();
    *time_milliseconds = (double)(end - start) / CLOCKS_PER_SEC * 1000;
    return SUCCESS;
}

int main(int argc, char **argv) {
    SAFE_CALL(parse_flags(argc, argv));
    srand(config.s);

    for (const struct named_strategy *named_strategy = strategies; named_strategy != strategies + 4; named_strategy++) {
        int success_count;
        double time_milliseconds;
        simulate(named_strategy->strategy, &success_count, &time_milliseconds);
        printf("%s      %i/%lu wins/games: %.2f%% %.3f/%lu ms = %.3f ms\n", named_strategy->name, success_count,
            config.n, (double)success_count * 100.0 / config.n, time_milliseconds, config.n,
            time_milliseconds / config.n);
    }
}
