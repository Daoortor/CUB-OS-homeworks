#include "xargs.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdint.h>

#define XARGS_MAX_ARGS 1024
#define XARGS_BUFFER_SIZE 1024
#define MIN(A, B) ((A) < (B) ? (A) : (B))
#define SAFE_CALL(F) if (process_error(argv, (F)) != EXIT_SUCCESS) return EXIT_FAILURE;

char storage[XARGS_BUFFER_SIZE];
char *args[XARGS_MAX_ARGS];
char *passed_args[XARGS_MAX_ARGS];
char *cur = storage;
char command_to_execute[XARGS_BUFFER_SIZE] = "/bin/echo";
int arg_count = 0;

struct flags flags = {INT32_MAX, false, 1};

int parse_flags(const int argc, char *argv[], struct flags *f) {
    char option;
    while ((option = getopt(argc, argv, "n:tj:")) != -1) {
        switch (option) {
            case 'n':
                f->n = atoi(optarg);
                break;
            case 't':
                f->t = true;
                break;
            case 'j':
                f->j = atoi(optarg);
                break;
            default:
                return INVALID_OPTION;
        }
    }
    return SUCCESS;
}

int parse_arg_from_stdin(const char *end, int *len) {
    if (cur == end) {
        return EOF;
    }
    char c = getchar();
    while (isspace(c) && c != EOF) {
        c = getchar();
    }
    if (c == EOF) {
        return EOF;
    }
    char current_quotation_symbol = 0;
    const char *start = cur;
    if (c == '\'' || c == '\"') {
        current_quotation_symbol = c;
    } else {
        *cur++ = c;
    }
    while ((c = getchar()) != EOF && cur != end) {
        if (c == current_quotation_symbol || (isspace(c) && current_quotation_symbol == 0)) {
            current_quotation_symbol = 0;
            break;
        }
        if ((c == '\'' || c == '\"') && current_quotation_symbol == 0) {
            return UNEXPECTED_QUOTATION_MARK;
        }
        *cur++ = c;
    }
    *len = cur - start;
    *cur++ = 0;
    return current_quotation_symbol == 0 ? SUCCESS : MISSING_QUOTATION_MARK;
}

int parse_args_from_stdin(const char *storage_end, char **dest, const int maxnum, int *arg_count) {
    int code = 0;
    int len = 0;
    while (*arg_count != maxnum) {
        char *old_cur = cur;
        code = parse_arg_from_stdin(storage_end, &len);
        if (code != EXIT_SUCCESS) {
            return code;
        }
        dest[*arg_count] = old_cur;
        (*arg_count)++;
    }
    return 0;
}

void get_command_to_execute(int argc, char *argv[], char *result) {
    if (optind >= argc) {
        return;
    }
    memcpy(result, argv[optind], strlen(argv[optind]) + 1);
    optind++;
}

int process_error(char *argv[], const enum parse_result error) {
    switch (error) {
        case INVALID_OPTION:
            return EXIT_FAILURE;
        case UNEXPECTED_QUOTATION_MARK:
            fprintf(stderr, "%s: unexpected token: \"\n", argv[0]);
            return EXIT_FAILURE;
        case MISSING_QUOTATION_MARK:
            fprintf(stderr, "%s: missing quotation mark\n", argv[0]);
            return EXIT_FAILURE;
        default:
            return EXIT_SUCCESS;
    }
}

int run(int argc, char *argv[]) {
    int additional_argc = argc - optind;
    passed_args[0] = command_to_execute;
    memcpy(passed_args + 1, argv + optind, additional_argc * sizeof(char *));
    for (char **p = args; p < args + arg_count; p += flags.n) {
        const int new_passed_args_count = MIN(flags.n, args + arg_count - p);
        memcpy(passed_args + additional_argc + 1, p, new_passed_args_count * sizeof(char *));
        passed_args[additional_argc + new_passed_args_count + 1] = NULL;
        const pid_t pid = fork();
        if (pid < 0) {
            fprintf(stderr, "Fork failed");
            return EXIT_FAILURE;
        }
        if (pid == 0) {
            for (char **p2 = passed_args; *p2 != NULL; p2++) {
                printf("%s ", *p2);
            }
            printf("\n");
            execvp(passed_args[0], passed_args);
            fprintf(stderr, "Process finished with error %i\n", errno);
            return EXIT_FAILURE;
        }
        waitpid(pid, NULL, 0);
    }
    return EXIT_SUCCESS;
};

int main(int argc, char *argv[]) {
    SAFE_CALL(parse_flags(argc, argv, &flags));
    SAFE_CALL(parse_args_from_stdin(storage + XARGS_BUFFER_SIZE, args, XARGS_MAX_ARGS, &arg_count));
    if (flags.t) {
        fprintf(stderr, "%i arguments entered: ", arg_count);
        for (int i = 0; i < arg_count; i++) {
            fprintf(stderr, "\"%s\" ", args[i]);
        }
        fprintf(stderr, "\n");
    }
    get_command_to_execute(argc, argv, command_to_execute);
    return run(argc, argv);
}