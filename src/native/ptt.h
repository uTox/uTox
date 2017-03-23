#ifndef NATIVE_PTT_H
#define NATIVE_PTT_H
// Push-to-talk

/** returns 0 if push to talk is enabled, and the button is up, else returns 1. */
void init_ptt(void);
bool get_ptt_key(void); // Never used. Remove?
bool set_ptt_key(void); // Never used. Remove?
// Returns a bool indicating whether you should send audio or not.
bool check_ptt_key(void);
void exit_ptt(void);

#endif
