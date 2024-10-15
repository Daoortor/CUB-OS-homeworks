// ReSharper disable CppDFAUnreachableCode
// ReSharper disable CppDFAConstantConditions
// ReSharper disable CppDFAEndlessLoop
#include "energy-drinks.h"
#include "lib.h"

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct config config = {
    DEFAULT_STUDENT_COUNT,
    DEFAULT_MACHINE_CAPACITY,
    DEFAULT_DRINK_PRICE,
};
struct dispensing_machine dispensing_machine = {0,         0,        0,
                                                NOT_EMPTY, RECEIVED, {}};

char **global_argv;
struct student_args student_args = {};

pthread_mutex_t use_machine = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t notify_student = PTHREAD_COND_INITIALIZER;
pthread_cond_t notify_supplier = PTHREAD_COND_INITIALIZER;
pthread_cond_t notify_machine = PTHREAD_COND_INITIALIZER;

int parse_flags(int argc, char **argv) {
    char flag;
    while ((flag = getopt(argc, argv, "n:c:p:")) != -1) {
        int code;
        switch (flag) {
        case 'c':
            code = safe_parse_ulong_option('c', &config.machine_capacity);
            if (code != SUCCESS) {
                return code;
            }
            break;
        case 'n':
            code = safe_parse_ulong_option('n', &config.student_count);
            if (code != SUCCESS) {
                return code;
            }
            break;
        case 'p':
            code = safe_parse_ulong_option('p', &config.drink_price);
            if (code != SUCCESS) {
                return code;
            }
            break;
        default:
            return INVALID_OPTION;
        }
    }
    if (optind < argc) {
        strncpy(error_info.error_arg, argv[optind], BUFFER_SIZE - 1);
        return UNEXPECTED_ARGUMENT;
    }
    return SUCCESS;
}

struct timespec delayed_for(uint seconds) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += seconds;
    return ts;
}

void dispense_drink(void) { dispensing_machine.drink_count--; }

void *student(void *) {
    unsigned int student_index = student_args.index;
    pthread_mutex_lock(&use_machine);

    STDOUT_LOG_FORMAT("student %i requires an energy drink\n", student_index);

    pthread_mutex_lock(&dispensing_machine.m);
    if (dispensing_machine.drink_count > 0) {
        pthread_mutex_unlock(&dispensing_machine.m);
        goto buy_drink;
    }
    pthread_mutex_unlock(&dispensing_machine.m);

    pthread_mutex_lock(&dispensing_machine.m);
    STDOUT_LOG_FORMAT("student %i waiting for machine to be refilled\n",
                      student_index);
    dispensing_machine.refill_status = WAITING_FOR_REFILL;
    pthread_cond_signal(&notify_supplier);
    pthread_mutex_unlock(&dispensing_machine.m);

    pthread_mutex_lock(&dispensing_machine.m);
    while (dispensing_machine.refill_status != REFILLED) {
        pthread_cond_wait(&notify_student, &dispensing_machine.m);
    }
    dispensing_machine.refill_status = NOT_EMPTY;
    pthread_mutex_unlock(&dispensing_machine.m);

buy_drink:
    pthread_mutex_lock(&dispensing_machine.m);
    STDOUT_LOG_FORMAT("student %i is next to be served\n", student_index);
    for (ulong i = 0; i < config.drink_price; i++) {
        dispensing_machine.inserted_coins_count++;
        STDOUT_LOG_FORMAT("student %i inserted another coin\n", student_index);
    }
    dispensing_machine.drink_status = WAITING_FOR_DRINK;
    STDOUT_LOG_FORMAT("student %i waiting for drink to arrive\n",
                      student_index);
    pthread_cond_signal(&notify_machine);
    pthread_mutex_unlock(&dispensing_machine.m);

    pthread_mutex_lock(&dispensing_machine.m);
    while (dispensing_machine.drink_status != DISPENSED) {
        pthread_cond_wait(&notify_student, &dispensing_machine.m);
    }
    STDOUT_LOG_FORMAT("student %i picked up a drink\n", student_index);
    dispensing_machine.drink_status = RECEIVED;
    STDOUT_LOG_FORMAT("student %i enjoying an energy drink\n", student_index);
    pthread_mutex_unlock(&dispensing_machine.m);

    pthread_mutex_unlock(&use_machine);
    return NULL;
}

void *supplier(void *) {
    while (1) {
        pthread_mutex_lock(&dispensing_machine.m);
        while (dispensing_machine.refill_status != WAITING_FOR_REFILL) {
            struct timespec ts = delayed_for(TIMEOUT_SECONDS);
            int code = pthread_cond_timedwait(&notify_supplier,
                                              &dispensing_machine.m, &ts);
            if (code == ETIMEDOUT) {
                STDOUT_LOG_FORMAT("supplier hasn't received a request in %d "
                                  "seconds, aborting...\n",
                                  TIMEOUT_SECONDS)
                pthread_mutex_unlock(&dispensing_machine.m);
                return NULL;
            }
        }

        STDOUT_LOG_FORMAT("supplier arriving\n");
        STDOUT_LOG_FORMAT("supplier loading %lu drinks\n",
                          config.machine_capacity -
                              dispensing_machine.drink_count);
        dispensing_machine.drink_count = config.machine_capacity;
        STDOUT_LOG_FORMAT("supplier collected %lu coins\n",
                          dispensing_machine.coins_count);
        dispensing_machine.coins_count = 0;
        dispensing_machine.refill_status = REFILLED;
        STDOUT_LOG_FORMAT("supplier leaving\n");
        pthread_cond_signal(&notify_student);

        pthread_mutex_unlock(&dispensing_machine.m);
    }
}

void *machine(void *) {
    while (1) {
        pthread_mutex_lock(&dispensing_machine.m);
        STDOUT_LOG_FORMAT("machine waiting for request\n")
        while (dispensing_machine.drink_status != WAITING_FOR_DRINK) {
            struct timespec ts = delayed_for(TIMEOUT_SECONDS);
            int code = pthread_cond_timedwait(&notify_machine,
                                              &dispensing_machine.m, &ts);
            if (code == ETIMEDOUT) {
                STDOUT_LOG_FORMAT("machine hasn't received a request in %d "
                                  "seconds, aborting...\n",
                                  TIMEOUT_SECONDS)
                pthread_mutex_unlock(&dispensing_machine.m);
                return NULL;
            }
        }
        if (dispensing_machine.inserted_coins_count < config.drink_price) {
            STDOUT_LOG_FORMAT("machine received %lu coins, but it needs at "
                              "least %lu to proceed\n",
                              dispensing_machine.inserted_coins_count,
                              config.drink_price);
        }
        dispensing_machine.coins_count +=
            dispensing_machine.inserted_coins_count;
        dispensing_machine.inserted_coins_count = 0;
        STDOUT_LOG_FORMAT("machine dispensing drink\n")
        dispense_drink();
        dispensing_machine.drink_status = DISPENSED;
        STDOUT_LOG_FORMAT("machine waiting for pickup of drink\n")
        pthread_cond_signal(&notify_student);
        pthread_mutex_unlock(&dispensing_machine.m);
    }
}

int run(void) {
    pthread_t *student_threads =
        // ReSharper disable once CppDFAMemoryLeak
        malloc(sizeof(pthread_t) * config.student_count);
    pthread_t supplier_thread;
    pthread_t machine_thread;

    pthread_mutex_lock(&dispensing_machine.m);
    for (ulong i = 0; i < config.student_count; i++) {
        student_args.index = i;
        SAFE_RUN_THREAD(&student_threads[i], student, NULL,
                        "student %lu established\n", i)
    }
    SAFE_RUN_THREAD(&supplier_thread, supplier, NULL, "supplier established\n")
    SAFE_RUN_THREAD(&machine_thread, machine, NULL, "machine booting up\n")
    pthread_mutex_unlock(&dispensing_machine.m);

    for (ulong i = 0; i < config.student_count; i++) {
        pthread_join(student_threads[i], NULL);
    }
    free(student_threads);
    pthread_join(supplier_thread, NULL);
    pthread_join(machine_thread, NULL);

    return SUCCESS;
}

int main(int argc, char **argv) {
    global_argv = argv;
    SAFE_CALL(parse_flags(argc, argv));
    SAFE_CALL(run());

    return SUCCESS;
}
