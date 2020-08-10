// timer.h

#ifndef _TIMER_H
#define _TIMER_H

#include <sys/time.h>
#include <sys/types.h>

struct timer {
  struct timeval tv_start;
  struct timeval tv_end;
};
typedef struct timer simpletimer_t;

double timer_elapsed(simpletimer_t t);
uint timer_elapsed_us(simpletimer_t t);

#endif // _TIMER_H
