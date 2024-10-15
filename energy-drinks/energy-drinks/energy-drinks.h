#ifndef ENERGY_DRINKS_H
#define ENERGY_DRINKS_H

#define BUFFER_SIZE 1024
#define DEFAULT_STUDENT_COUNT 2
#define DEFAULT_MACHINE_CAPACITY 4
#define DEFAULT_DRINK_PRICE 5
#define TIMEOUT_SECONDS 10

#define SAFE_CALL(F)                                                           \
    if (process_error(argv, (enum error)(F)) != EXIT_SUCCESS) {                \
        return EXIT_FAILURE;                                                   \
    }

#define SAFE_RUN_THREAD(THREAD, F, ARGS, ...)                                  \
    if (pthread_create(THREAD, NULL, F, ARGS) == EINVAL) {                     \
        return INSUFFICIENT_RESOURCES;                                         \
    }                                                                          \
    STDOUT_LOG_FORMAT(__VA_ARGS__)

#define STDOUT_LOG_FORMAT(...)                                                 \
    printf("%s: [%lu/%lu drinks, %lu coins, %lu inserted] ", global_argv[0],   \
           dispensing_machine.drink_count, config.machine_capacity,            \
           dispensing_machine.coins_count,                                     \
           dispensing_machine.inserted_coins_count);                           \
    printf(__VA_ARGS__);

#include <pthread.h>
#include <sys/types.h>

enum refill_status {
    NOT_EMPTY,
    WAITING_FOR_REFILL,
    REFILLED,
};

enum drink_status {
    RECEIVED,
    WAITING_FOR_DRINK,
    DISPENSED,
};

struct config {
    ulong student_count;
    ulong machine_capacity;
    ulong drink_price;
};

struct student_args {
    int index;
};

struct dispensing_machine {
    ulong drink_count;
    ulong coins_count;
    ulong inserted_coins_count;
    enum refill_status refill_status;
    enum drink_status drink_status;
    pthread_mutex_t m;
};

int safe_parse_ulong_option(char flag, ulong *result);

int parse_flags(int argc, char **argv);

void *student(void *);

void *supplier(void *);

void *machine(void *);

struct timespec delayed_for(uint seconds);

int run(void);

#endif // ENERGY_DRINKS_H
