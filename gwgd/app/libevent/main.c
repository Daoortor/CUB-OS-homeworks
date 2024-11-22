#define _POSIX_C_SOURCE 200809L

#include "../../lib/lib.h"
#include "../../util/player.h"
#include "main.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <netinet/in.h>
#include <sys/socket.h>

struct event_base *event_base;

struct config config = {
    BLOCKING, 8888, 0, {}, {}
};

void eh_invalid_option() {}
void eh_argument_out_of_range() { fprintf(stderr, "%s: -%c: argument %s out of range\n", config.program_name, config.error_option, config.error_arg); }
void eh_invalid_argument_value() { fprintf(stderr, "%s: -%c: invalid argument %s\n", config.program_name, config.error_option, config.error_arg); }
void eh_conflicting_options() { fprintf(stderr, "%s: -%c: option conflicting with %s\n", config.program_name, config.error_option, config.error_arg); }
void eh_unexpected_argument() { fprintf(stderr, "%s: unexpected argument %s\n", config.program_name, config.error_arg); }
void eh_insufficient_memory() { fprintf(stderr, "%s: not enough memory\n", config.program_name); }
void eh_thread_error() { fprintf(stderr, "%s: thread error\n", config.program_name); }
void eh_system_not_supported() { fprintf(stderr, "%s: system not supported\n", config.program_name); }
void eh_interrupted() { fprintf(stderr, "%s: interrupted\n", config.program_name); }
void eh_permission_denied() { fprintf(stderr, "%s: permission denied\n", config.program_name); }
void eh_unsupported_address_family() { fprintf(stderr, "%s: unsupported address family\n", config.program_name); }
void eh_opened_files_limit_reached() { fprintf(stderr, "%s: system-wide limit on the number of open files or file descriptors reached\n", config.program_name); }
void eh_socket_bind_failed() { fprintf(stderr, "%s: socket bind failed (perhaps this port is already in use?)\n", config.program_name); }
void eh_port_in_use() { fprintf(stderr, "%s: port %lu is already in use\n", config.program_name, config.port); }
void eh_fork_error() { fprintf(stderr, "%s: port %lu is already in use\n", config.program_name, config.port); }
void eh_pipe_error() { fprintf(stderr, "%s: pipe error\n", config.program_name); }
void eh_unknown_error() { fprintf(stderr, "%s: unknown error\n", config.program_name); }

struct error_handler error_handlers[SMALL_BUFFER_SIZE] = {
    {INVALID_OPTION, eh_invalid_option},
    {ARGUMENT_OUT_OF_RANGE, eh_argument_out_of_range},
    {INVALID_ARGUMENT_VALUE, eh_invalid_argument_value},
    {CONFLICTING_OPTIONS, eh_conflicting_options},
    {UNEXPECTED_ARGUMENT, eh_unexpected_argument},
    {INSUFFICIENT_MEMORY, eh_insufficient_memory},
    {THREAD_ERROR, eh_thread_error},
    {SYSTEM_NOT_SUPPORTED, eh_system_not_supported},
    {INTERRUPTED, eh_interrupted},
    {PERMISSION_DENIED, eh_permission_denied},
    {UNSUPPORTED_ADDRESS_FAMILY, eh_unsupported_address_family},
    {OPENED_FILES_LIMIT_REACHED, eh_opened_files_limit_reached},
    {SOCKET_BIND_FAILED, eh_socket_bind_failed},
    {PORT_IN_USE, eh_port_in_use},
    {FORK_ERROR, eh_fork_error},
    {PIPE_ERROR, eh_pipe_error},
    {UNKNOWN_ERROR, eh_unknown_error},
    {-1, NULL}
};

char *error_option = &config.error_option;
char *error_arg_ptr = config.error_arg;
char **error_arg = &error_arg_ptr;

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
                    snprintf(config.error_arg, BUFFER_SIZE, "%lu", config.port);
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
            SAFE_CALL_OR_HANDLE_AND_RETURN_NULL(chlng_fetch_text(c));
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

void cb_func(evutil_socket_t fd, short, void *arg) {
    struct sockfd_config *conf = arg;
    int connfd = accept(fd, (struct sockaddr*)&conf->cli, &conf->len);
    if (connfd == -1) {
        perror("Accept failed");
        return;
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
                errno = INSUFFICIENT_MEMORY;
                return;
            }
            pthread_detach(thread);
        break;
        case FORKING:
            pid_t pid = fork();
            if (pid == 0) {
                close(fd);
                run_game(thread_args);
                exit(0);
            }
            if (pid < 0) {
                errno = FORK_ERROR;
                return;
            }
            close(connfd);
    }
}

int init_socket(struct sockfd_config *conf, int domain) {
    conf->sockfd = socket(domain, SOCK_STREAM, 0);
    if (conf->sockfd == -1) {
        switch (errno) {
            case EADDRINUSE:
                return PORT_IN_USE;
            case EACCES:
                return PERMISSION_DENIED;
            case EAFNOSUPPORT:
                return UNSUPPORTED_ADDRESS_FAMILY;
            case EMFILE:
            case ENFILE:
                return OPENED_FILES_LIMIT_REACHED;
            case ENOMEM:
                return INSUFFICIENT_MEMORY;
            case EPROTONOSUPPORT:
                return SYSTEM_NOT_SUPPORTED;
            default:
                return UNKNOWN_ERROR;
        }
    }
    conf->servaddr.sin_family = domain;
    conf->servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    conf->servaddr.sin_port = htons(config.port);
    if (bind(conf->sockfd, (struct sockaddr*)&conf->servaddr, sizeof(conf->servaddr)) != 0 || listen(conf->sockfd, BACKLOG) != 0) {
        return SOCKET_BIND_FAILED;
    }
    conf->len = sizeof(conf->cli);
    return EXIT_SUCCESS;
}

int register_listening_sockets() {
    event_base = event_base_new();

    struct sockfd_config *ipv4_conf = malloc(sizeof(struct sockfd_config));
    struct sockfd_config *ipv6_conf = malloc(sizeof(struct sockfd_config));
    if (ipv4_conf == NULL || ipv6_conf == NULL) {
        return INSUFFICIENT_MEMORY;
    }

    SAFE_CALL_OR_FAIL(init_socket(ipv4_conf, AF_INET));
    SAFE_CALL_OR_FAIL(init_socket(ipv6_conf, AF_INET6));

    event_new(event_base, ipv4_conf->sockfd, EV_READ | EV_WRITE, cb_func, &ipv4_conf);
    event_new(event_base, ipv6_conf->sockfd, EV_READ | EV_WRITE, cb_func, &ipv6_conf);

    return EXIT_SUCCESS;
}

int main(int argc, char **argv) {
    SAFE_CALL_OR_FAIL(parse_flags(argc, argv));
    SAFE_CALL_OR_FAIL(register_listening_sockets());
    while (errno == EXIT_SUCCESS) {}
    process_error(errno, error_handlers);
    event_base_free(event_base);
    return EXIT_FAILURE;
}
