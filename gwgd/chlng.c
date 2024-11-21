#include "chlng.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern chlng_t* chlng_new(void) {
    chlng_t *chlng = malloc(sizeof(chlng_t));
    chlng->text = malloc(BUFFER_SIZE);
    chlng->word = malloc(BUFFER_SIZE);
    return chlng;
}

extern void chlng_reset(chlng_t *c) {
    memset(c->text, '\0', BUFFER_SIZE);
    memset(c->word, '\0', BUFFER_SIZE);
}

extern void chlng_del(chlng_t *c) {
    free(c->text);
    free(c->word);
    free(c);
}

extern int chlng_fetch_text(chlng_t *c) {
    FILE *pipe = popen("fortune -s", "r");
    if (pipe == NULL) {
        return PIPE_ERROR;
    }
    char *cur = c->text;
    while (cur - c->text < BUFFER_SIZE && fgets(cur, BUFFER_SIZE - (cur - c->text), pipe)) {
        cur += strlen(cur);
    }
    pclose(pipe);
    return SUCCESS;
}

// ReSharper disable once CppDFAConstantFunctionResult
extern int chlng_hide_word(chlng_t *c) {
    int word_start_indexes[BUFFER_SIZE];
    int *index_cur = word_start_indexes;
    char *text_cur = c->text;
    bool prev_is_alpha = false;
    while (*text_cur != '\0') {
        if (isalpha(*text_cur)) {
            if (!prev_is_alpha) {
                *index_cur = text_cur - c->text;
                index_cur++;
            }
            prev_is_alpha = true;
        } else {
            prev_is_alpha = false;
        }
        text_cur++;
    }
    if (index_cur == word_start_indexes) {
        return SUCCESS;
    }
    int erase_start_index = word_start_indexes[rand() % (index_cur - word_start_indexes)];
    text_cur = c->text + erase_start_index;
    char *word_cur = c->word;
    while (isalpha(*text_cur)) {
        *word_cur = *text_cur;
        *text_cur = '_';
        word_cur++;
        text_cur++;
    }
    *word_cur = '\0';
    return SUCCESS;
}
