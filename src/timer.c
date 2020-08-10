// timer.c

#include "timer.h"

double timer_elapsed(simpletimer_t t)
{
  double et =
    ((double) t.tv_end.tv_sec + (double) t.tv_end.tv_usec / 1000000.) -
    ((double) t.tv_start.tv_sec + (double) t.tv_start.tv_usec / 1000000.);

  return et;
}

uint timer_elapsed_us(simpletimer_t t)
{
  uint etus =
    (((uint) t.tv_end.tv_sec - (uint) t.tv_start.tv_sec) * 1000000) +
    ((uint) t.tv_end.tv_usec - (uint) t.tv_start.tv_usec);

  return etus;
}
