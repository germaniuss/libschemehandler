#ifndef _UTILS_TIMER_H
#define _UTILS_TIMER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "thread.h"
#include <stdbool.h>

struct timer {
    struct thread th;
    bool stopped;
    bool init;
    int ms;
    void* (*callback) (void*);
    void* data;
};

void timer_init(struct timer* timer);
void timer_start(struct timer* timer, int ms, void* (*callback) (void*), void* data);
void timer_term(struct timer* timer);

#ifdef __cplusplus
}
#endif

#if defined(_UTILS_IMPL) || defined(_UTILS_TIMER_IMPL)

#include "time.h"

void* timeit(void* data) {
    struct timer* timer = (struct timer*) data;
    while (timer->init) {
        while (timer->stopped && timer->init);
        if (!timer->init) break;
        time_sleep(timer->ms);
        timer->callback(timer->data);
        timer->stopped = true;
    }
}

void timer_init(struct timer* timer) {
    timer->stopped = true;
    timer->init = true;
    thread_init(&timer->th);
    thread_start(&timer->th, timeit, timer);
}

void timer_start(struct timer* timer, int ms, void* (*callback) (void*), void* data) {
    timer->ms = ms;
    timer->callback = callback;
    timer->data = data;
    timer->stopped = false;
}

void timer_term(struct timer* timer) {
    timer->init = false;
    thread_term(&timer->th);
}

#endif
#endif