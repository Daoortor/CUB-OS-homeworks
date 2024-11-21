#include "player.h"

#include <stdio.h>
#include <stdlib.h>

extern player_t* player_new(void) {
    player_t* player = malloc(sizeof(player_t));
    player->chlng = chlng_new();
    return player;
}

extern void player_reset(player_t* player) {
    player->solved = 0;
    player->total = 0;
    player->finished = false;
    chlng_reset(player->chlng);
}

extern void player_del(player_t* player) {
    chlng_del(player->chlng);
    free(player);
}

extern int player_fetch_chlng(player_t* player) {
    int rc = chlng_fetch_text(player->chlng);
    chlng_hide_word(player->chlng);
    return rc;
}

extern int player_get_greeting(player_t *, char *result) {
    snprintf(result, BUFFER_SIZE,
        "M: Guess the missing ____!\nM: Send your guess in the form 'R: <word>'.\n");
    return SUCCESS;
}
