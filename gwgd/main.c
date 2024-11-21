#define _POSIX_C_SOURCE 200809L

#include "player.h"
#include "main.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <netinet/in.h>

struct config config = {
    BLOCKING, 8888, 0, {}, {}
};

int safe_parse_ulong_option(char flag, unsigned long *result) {
    errno = 0;
    char *endptr = 0;
    *result = strtoul(optarg, &endptr, INT_BASE);
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
    strcpy(config.program_name, argv[0]);
    char flag;
    while ((flag = getopt(argc, argv, "tfp:")) != -1) {
        int code;
        switch (flag) {
            case 't':
                config.server_mode = THREAD;
                break;
            case 'f':
                if (config.server_mode == THREAD) {
                    config.error_option = flag;
                    strncpy(config.error_arg, "t", BUFFER_SIZE - 1);
                    return CONFLICTING_OPTIONS;
                }
                config.server_mode = FORKING;
                break;
            case 'p':
                code = safe_parse_ulong_option('p', &config.port);
                if (code != SUCCESS) {
                    return code;
                }
                if (config.port > 65535) {
                    config.error_option = flag;
                    snprintf(config.error_arg, BUFFER_SIZE, "%u", code);
                    return ARGUMENT_OUT_OF_RANGE;
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

int process_error(enum error error) {
    switch (error) {
        case INVALID_OPTION:
            return EXIT_FAILURE;
        case ARGUMENT_OUT_OF_RANGE:
            fprintf(stderr, "%s: -%c: argument %s out of range\n", config.program_name, config.error_option, config.error_arg);
            return EXIT_FAILURE;
        case INVALID_ARGUMENT_VALUE:
            fprintf(stderr, "%s: -%c: invalid argument %s\n", config.program_name, config.error_option, config.error_arg);
            return EXIT_FAILURE;
        case CONFLICTING_OPTIONS:
            fprintf(stderr, "%s: -%c: option conflicting with %s\n", config.program_name, config.error_option, config.error_arg);
            return EXIT_FAILURE;
        case UNEXPECTED_ARGUMENT:
            fprintf(stderr, "%s: unexpected argument %s\n", config.program_name, config.error_arg);
            return EXIT_FAILURE;
        case INSUFFICIENT_MEMORY:
            fprintf(stderr, "%s: not enough memory\n", config.program_name);
            return EXIT_FAILURE;
        case THREAD_ERROR:
            fprintf(stderr, "%s: unknown thread error\n", config.program_name);
            return EXIT_FAILURE;
        case SYSTEM_NOT_SUPPORTED:
            fprintf(stderr, "%s: system not supported\n", config.program_name);
            return EXIT_FAILURE;
        case INTERRUPTED:
            fprintf(stderr, "%s: interrupted\n", config.program_name);
            return EXIT_FAILURE;
        case PERMISSION_DENIED:
            fprintf(stderr, "%s: permission denied\n", config.program_name);
            return EXIT_FAILURE;
        case UNSUPPORTED_ADDRESS_FAMILY:
            fprintf(stderr, "%s: unsupported address family\n", config.program_name);
            return EXIT_FAILURE;
        case FILE_LIMIT_REACHED:
            fprintf(stderr, "%s: system-wide limit on the number of open files or file descriptors reached\n", config.program_name);
            return EXIT_FAILURE;
        case SOCKET_BIND_FAILED:
            fprintf(stderr, "%s: socket bind failed\n", config.program_name);
            return EXIT_FAILURE;
        case PORT_IN_USE:
            fprintf(stderr, "%s: port %lu is already in use\n", config.program_name, config.port);
            return EXIT_FAILURE;
        case FORK_ERROR:
            fprintf(stderr, "%s: fork error\n", config.program_name);
            return EXIT_FAILURE;
        case PIPE_ERROR:
            fprintf(stderr, "%s: pipe error\n", config.program_name);
            return EXIT_FAILURE;
        case UNKNOWN_ERROR:
            fprintf(stderr, "%s: unknown error\n", config.program_name);
            return EXIT_FAILURE;
        default:
            return EXIT_SUCCESS;
    }
}

// ReSharper disable once CppDFAConstantFunctionResult
void *run_game(void *args) {
    int connfd = ((struct talk_args *)args)->connfd;
    char read_buf[BUFFER_SIZE];
    char write_buf[BUFFER_SIZE];
    player_t *player = player_new();
    player_get_greeting(player, write_buf);
    write(connfd, write_buf, strlen(write_buf));
    int challenge_count = 0;
    int win_count = 0;
    chlng_t *c = chlng_new();
    bool keep_challenge = false;
    while (1) {
        if (keep_challenge) {
            keep_challenge = false;
        } else {
            SAFE_CALL_FROM_THREAD(chlng_fetch_text(c));
            chlng_hide_word(c);
        }
        snprintf(write_buf, BUFFER_SIZE, "C: %s", c->text);
        write(connfd, write_buf, strlen(write_buf));
        char *cur = read_buf;
        while (1) {
            switch (recv(connfd, cur, 1, 0)) {
                case -1:
                case 0:
                    player_del(player);
                    close(connfd);
                    return NULL;
                default:
            }
            if (*cur == '\n') {
                break;
            }
            cur++;
        }
        *cur = '\0';
        if (cur - read_buf < 2) {
            snprintf(write_buf, BUFFER_SIZE, "M: invalid response.\n Responses should be either of the format `R: <guess>` (make a guess) or `Q:<any text>` (exit).\n");
            keep_challenge = true;
            write(connfd, write_buf, strlen(write_buf));
            continue;
        }
        if (strncmp(read_buf, "Q:", 2) == 0) {
            snprintf(write_buf, BUFFER_SIZE, "M: you mastered %i/%i challenges. Goodbye!\n",
                win_count, challenge_count);
            write(connfd, write_buf, strlen(write_buf));
            break;
        }
        if (strncmp(read_buf, "R:", 2) == 0) {
            if (strncmp(read_buf + 3, c->word, BUFFER_SIZE) == 0) {
                snprintf(write_buf, BUFFER_SIZE, "O: Congratulation! Challenge passed!\n");
                write(connfd, write_buf, strlen(write_buf));
                win_count++;
            } else {
                snprintf(write_buf, BUFFER_SIZE, "O: Wrong guess '%s' - expected '%s'\n", read_buf + 3, c->word);
                write(connfd, write_buf, strlen(write_buf));
            }
            challenge_count++;
        } else {
            snprintf(write_buf, BUFFER_SIZE, "M: invalid response.\n Responses should be either of the format `R: <guess>` (make a guess) or `Q:<any text>` (exit).\n");
            keep_challenge = true;
            write(connfd, write_buf, strlen(write_buf));
        }
    }
    player_del(player);
    close(connfd);
    free(args);
    return NULL;
}

int server(void) {
    struct sockaddr_in servaddr, cli;
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        switch (errno) {
            case EADDRINUSE:
                return PORT_IN_USE;
            case EACCES:
                return PERMISSION_DENIED;
            case EAFNOSUPPORT:
                return UNSUPPORTED_ADDRESS_FAMILY;
            case EMFILE:
            case ENFILE:
                return FILE_LIMIT_REACHED;
            case ENOMEM:
                return INSUFFICIENT_MEMORY;
            case EPROTONOSUPPORT:
                return SYSTEM_NOT_SUPPORTED;
            default:
                return UNKNOWN_ERROR;
        }
    }
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(config.port);
    if (bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) != 0 || listen(sockfd, BACKLOG) != 0) {
        return SOCKET_BIND_FAILED;
    }
    socklen_t len = sizeof(cli);
    while (true) {
        int connfd = accept(sockfd, (struct sockaddr*)&cli, &len);
        if (connfd == -1) {
            perror("Accept failed");
            continue;
        }
        // Freed in run_game()
        // ReSharper disable once CppDFAMemoryLeak
        struct talk_args *thread_args = malloc(sizeof(struct talk_args));
        thread_args->connfd = connfd;
        switch (config.server_mode) {
            case BLOCKING:
                run_game(thread_args);
                break;
            case THREAD:
                pthread_t thread;
                const int code = pthread_create(&thread, NULL, &run_game, thread_args);
                if (code == EAGAIN) {
                    // Freed in run_game()
                    // ReSharper disable once CppDFAMemoryLeak
                    return INSUFFICIENT_MEMORY;
                }
                pthread_detach(thread);
                break;
            case FORKING:
                pid_t pid = fork();
                if (pid == 0) {
                    close(sockfd);
                    run_game(thread_args);
                    exit(0);
                }
                if (pid < 0) {
                    return FORK_ERROR;
                }
                close(connfd);
        }
    }
}

int main(int argc, char **argv) {
    SAFE_CALL(parse_flags(argc, argv));
    SAFE_CALL(server());
    return 0;
}
