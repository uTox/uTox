#ifndef NATIVE_THREAD_H
#define NATIVE_THREAD_H

void thread(void func(void *), void *args);
void yieldcpu(uint32_t ms);

#endif
