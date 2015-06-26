/** time_master.h
 *
 * The following commands allow uTox to interact with time. options include; updating data, getting a call back.
 * You can get a callback after time has elapsed exactly, after a minimum amount of time, or at a recurring interval.
 */


typedef struct timemaster_action {
    /* Type of time action. */
    _Bool callback, update, tween, repeating, complete;
    /* Scheduled time */
    uint32_t next_run, last_run, interval;
    /* Update variables. */
    void *target, *new_val, *steps;
    /* Call back function. */
    void (*function)(void);

} TIMEMSTR_ACTION;

/** the primary function, the thread_worker it self. */
void timemaster_thread(void);
