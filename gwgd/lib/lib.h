#ifndef LIB_H
#define LIB_H

#define INT_BASE 10
#define BUFFER_SIZE 1024
#define SMALL_BUFFER_SIZE 32
#define SAFE_CALL_OR_FAIL(F) if (process_error((F), error_handlers) != EXIT_SUCCESS) return EXIT_FAILURE;
#define SAFE_CALL_OR_HANDLE_AND_RETURN_NULL(F) if (process_error((F), error_handlers) != EXIT_SUCCESS) return NULL;
#define BACKLOG 5

struct config;
enum error;
extern char *error_option;
extern char **error_arg;
extern struct error_handler *error_handlers_ptr;

struct error_handler {
    int error;
    void (*handle)();
};

int process_error(int error, struct error_handler *error_handlers);

int safe_parse_ulong_option(char flag, unsigned long *result);

#endif //LIB_H
