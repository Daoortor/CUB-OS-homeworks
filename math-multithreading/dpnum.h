#ifndef DPNUM_H
#define DPNUM_H

#include <stdbool.h>
#include <threads.h>

#define BASE 10
#define UL_SIZE sizeof(unsigned long)*8
#define BUFFER_SIZE 1024
// One more than the number of positive perfect digital invariant numbers less than or equal to ULONG_MAX
// See https://oeis.org/A023052/b023052.txt.
// This is the size of the buffer used to store them.
#define MAX_PDI_NUMBER_COUNT 58

#define SAFE_CALL(F) if (process_error(argv, (F)) != EXIT_SUCCESS) return EXIT_FAILURE;

enum error {
    SUCCESS = 0,
    INVALID_OPTION,
    ARGUMENT_OUT_OF_RANGE,
    INVALID_ARGUMENT_VALUE,
    INVALID_RANGE,
    UNEXPECTED_ARGUMENT,
    INSUFFICIENT_MEMORY,
    THREAD_ERROR,
};

struct config {
    unsigned long s;
    unsigned long e;
    unsigned long t;
    bool v;
    char error_option;
    char error_arg[BUFFER_SIZE];
};

struct args {
    unsigned long start;
    unsigned long end;
    unsigned long *dest;
    int dest_size;
};

int safe_parse_ulong_option(char flag, unsigned long *result);

int parse_flags(int argc, char **argv);

int process_error(char *argv[], enum error error);

void precompute_powers();

unsigned long compute_power_sum(char *digits, unsigned long exponent);

bool check(unsigned long n, char *buf);

int check_range(void *args_ptr);

int run_thread(char **argv, int thread_index, unsigned long *dest, thrd_t *thread_id, struct args *args_buf);

#endif //DPNUM_H
