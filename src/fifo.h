// fifo.h

#include <malloc.h>
#include <errno.h>

#ifndef _FIFO_H
#define _FIFO_H

#define BLOCKING_PUT_SLEEP_US (10000)

struct sFifo {
  pthread_mutex_t lock;
  int size;
  int in;
  int out;
  char **q;
};
typedef struct sFifo fifo_t;

int FifoCreate(fifo_t *f, int size);
void FifoDestroy(fifo_t *f);
int FifoPut(fifo_t *f, char *s);
void FifoPutBlock(fifo_t *f, char *s);
char *FifoGet(fifo_t *f);
int FifoDrain(fifo_t *f);

#endif // _FIFO_H
