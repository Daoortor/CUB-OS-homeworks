#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

bool verbose;
extern char **environ;

struct key_value_pair {
    char *key;
    char *value;
};

int parse_key_value_pair(const char *arg, char *buf, struct key_value_pair *kvp) {
    if (!*strchr(arg, '=')) return -1;
    strcpy(buf, arg);
    kvp->key = strtok(buf, "=");
    kvp->value = strtok(NULL, "=");
    return 0;
}

int main(int argc, char **argv) {
    char option;
    char buf[1024];
    struct key_value_pair kvp;
    while (option = getopt(argc, argv, "vu:"), option != -1) {
        switch (option) {
            case 'v':
                verbose = true;
            break;
            case 'u':
                unsetenv(optarg);
            break;
            case '?':
                printf("Unrecognized option '%c'\n", option);
        }
    }
    for (int index = optind; index < argc; index++) {
        int exit_code = parse_key_value_pair(argv[index], buf, &kvp);
        if (exit_code) {
            exit_code = setenv(kvp.key, kvp.value, 1);
            if (verbose) {
                printf(exit_code == 0 ? "Set environment variable %s to %s\n" : "Failed setting %s to %s\n",
                    kvp.key, kvp.value);
            }
        } else {
            execvp(argv[index], argv + index);
        }
    }
    for (int entryIndex = 0; environ[entryIndex] != NULL; entryIndex++) {
        printf("%s\n", environ[entryIndex]);
    }
}
