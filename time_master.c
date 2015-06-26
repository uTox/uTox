#include "main.h"

/* init bits */
_Bool timemaster_alive = 0, timemaster_sleeping = 0, timemaster_seppuku = 0;
/* Tracking bits */
uint32_t timemaster_count = 0, timemaster_size = 0;
/* Run/action bits */
_Bool timemaster_recalulate = 0;
/* List of tracked actions */
TIMEMSTR_ACTION **timemaster_list;

/* Setup and overhead functions. */
static void timemaster_init(void){
    /* Get some memory to hold our brain.  (start with 10 because why not!) */
    timemaster_list = calloc(sizeof(**timemaster_list), 10);
    if (timemaster_list) {
        timemaster_size = 10;
        timemaster_count = 0;
    } else {
        debug("cant' start timemaster\n");
    }
}

static _Bool timemaster_resize(void){
    /* check for space at end of array
     * NULL vars that have expired
     * Move vars into empty spaces
     * check for space again
     * verify count and size */
}


static uint32_t tmst_time_to_action(void){
    /* Chew through actions we need to take, and return the time to the nearest due time. */
    return 10; /* Guaranteed to be accurate */
}

static void tmst_update_next_time(uint32_t nap_length){
    if (!nap_length) {
        /* If we didn't get to take a nap, do nothing. */
        return;
    }
    /* Loop through action list, and update the next time and last time. */
    for ( uint32_t i = 0 ; i <= timemaster_size  ; i++ ) {
        if (timemaster_list[i]) {
            if (timemaster_list[i]->complete) {
                timemaster_list[i] = NULL;
                timemaster_count--;
            }
            if (timemaster_list[i]->next_run < nap_length) {
                debug("OH SHIT!! We missed action! That's really bad!\n");
                timemaster_list[i]->next_run = 0;
            } else {
                timemaster_list[i]->next_run -= nap_length;
            }
        }
    }
}

static void tmst_do_tween(TIMEMSTR_ACTION *action){
    // Do tween animations
}


void timemaster_thread(void){
    /* Announce we're running. */
    timemaster_init();
    timemaster_alive = 1;

    /* local vars */
    uint32_t sleep_time;

    while(1){
        if (timemaster_seppuku) {
            /* uTox is trying to exit, SHUT DOWN EVERYTHING! */
            break;
        }

        /* calculate wait time until next action,*/
        if (timemaster_recalulate) {
            sleep_time = tmst_time_to_action();
        }

        /* Action is due. */
        if (!sleep_time) {
            for ( uint32_t i = 0 ; ( i <= timemaster_count || i <= timemaster_size ) ; i++ ) {
                if ( timemaster_list[i] ) {
                    TIMEMSTR_ACTION *action = timemaster_list[i];
                    if (action->callback) {
                        /* hit the callback and be done. */
                        action->function();

                    } else if ( action->update) {
                        /* Update the var they want, and free the excess. */
                        void *free_me = action->new_val;
                        action->target = action->new_val;
                        free(free_me);

                    } else if ( action->tween ){
                        /* Tweening involves math, and Math Is Hard Barbie! */
                        tmst_do_tween(action);
                    }

                    if (!action->repeating) {
                        action->complete = 1;
                    }
                }
            }
        }

        yieldcpu(sleep_time);

        tmst_update_next_time(sleep_time);

        // if action is due DOOEET
        //
        // else
        //
        // sleep,
        // update time remaining on actions,
        // and then continue
        //
        /* first, sleep the amount of time we were told to wait*/
        // do current action
        // done
    }
    timemaster_alive = 0;
}

static TIMEMSTR_ACTION* timemaster_first_free(){
    do {
        for (uint32_t i = 0 ; timemaster_size; i++) {
            if ( timemaster_list[i] ) {
                continue;
            } else {
                return timemaster_list[i];
            }
        }
    } while (timemaster_resize());
    return NULL;
}

void timemaster_add_callback_at(void *function, uint32_t millisecs){
    TIMEMSTR_ACTION *slot = timemaster_first_free();
    if (!slot) {
        debug("Couldn't set timer\n");
        return;
    }

    memset(slot, 0, sizeof(*slot));
    slot->callback = 1;
    slot->function = function;
    slot->next_run = millisecs;
}
/*void timemaster_add_callback_after(function, millisecs){

}
void timemaster_add_callback_every(function, millisecs){

}
void timemaster_add_callback_clear(function, target??){

}

void timemaster_add_value_pop(target, new val, millisecs){

}

void timemaster_add_tween(target, start, end, steps, millisecs, exponint){

}

void timemaster_add_tween_cycle(target, start, end, steps, millisecs, exponint, delay ){

}

void timemaster_add_tween_repeat(target, start, end, steps, millisecs, exponint, delay ){

}*/

