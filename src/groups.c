#include "main.h"

void group_free(GROUPCHAT *g) {
    uint16_t i = 0;
    while(i != g->edit_history_length) {
        free(g->edit_history[i]);
        i++;
    }
    free(g->edit_history);

    char_t **np = g->peername;
    uint32_t j = 0;
    while (j < g->peers) {
        char_t *n = *np++;
        if(n) {
            free(n);
        }
        j++;
    }

    uint32_t k = 0;
    while (k < g->msg.number) {
        free(g->msg.data[k]);
        k++;
    }

    free(g->msg.data);

    memset(g, 0, sizeof(GROUPCHAT));//
}
