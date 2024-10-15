#ifndef XARGS_H
#define XARGS_H
#include <stdbool.h>

enum parse_result {
    EOF = -1,
    SUCCESS = 0,
    INVALID_OPTION,
    UNEXPECTED_QUOTATION_MARK,
    MISSING_QUOTATION_MARK,
};

struct flags {
    int n;
    bool t;
    int j;
    char error_flag;
};

int parse_arg_from_stdin(const char *end, int *len);

int parse_args_from_stdin(const char *storage_end, char **dest, int maxnum, int *arg_count);

int parse_flags(int argc, char *argv[], struct flags *f);

void get_command_to_execute(int argc, char *argv[], char *result);

int process_error(char *argv[], enum parse_result error);

int run(int argc, char *argv[]);

#endif //XARGS_H
