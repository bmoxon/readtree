// fifo.c

#undef DEBUG

#include <pthread.h>
#include "fifo.h"
#include "rtcommon.h"

int FifoCreate(fifo_t *f, int size)
{
  if ((f->q = (char **) malloc (size * sizeof(char *))) == 0)
  {
    fprintf(stderr, "FifoCreate: queue malloc failed\n");
    return (-1);
  }

  f->size = size + 1;
  f->in = 0;
  f->out = 0;

  pthread_mutex_init(&f->lock, NULL);

  Dprintf("FifoCreate: f=0x%lx, q=0x%lx, size = %d, in = %d, out = %d\n",
	  (unsigned long) f, (unsigned long) f->q, f->size, f->in, f->out);
  return (0);
}

void FifoDestroy(fifo_t *f)
{
  free(f->q);
}

// Blocking Put - sleep a bit if full
void FifoPutBlock(fifo_t *f, char *s)
{
  int rc;
  bool_t bDone = FALSE;

  while (! bDone)
  {
    rc = FifoPut(f, s);
    if (rc == 0)
      bDone = TRUE;
    else
      usleep(BLOCKING_PUT_SLEEP_US);
  }
}

int FifoPut(fifo_t *f, char *s)
{
  char *scopy;

  pthread_mutex_lock(&(f->lock));

  // check for full
  if (((f->in + 1) % f->size) == f->out)
  {
    pthread_mutex_unlock(&(f->lock));
    Dprintf("FifoPut: queue is full\n");
    return (-1);
  }

  // make a copy of s to enqueue; dequeue caller should free
  scopy = (char *) malloc (strlen(s) + 1);
  strcpy(scopy, s);

  f->q[f->in] = scopy;
  f->in = (f->in + 1) % f->size;
  pthread_mutex_unlock(&(f->lock));
  Dprintf("FifoPut: enqueued %s\n", scopy);
  return (0);
}

char *FifoGet(fifo_t *f)
{
  char *s;

  pthread_mutex_lock(&(f->lock));
  // check for empty
  if (f->in == f->out)
  {
    pthread_mutex_unlock(&(f->lock));
    Dprintf("FifoGet: queue is empty\n");
    return (NULL);
  }

  s = f->q[f->out];
  f->out = (f->out +1) % f->size;
  pthread_mutex_unlock(&(f->lock));

  Dprintf("FifoGet: dequeued %s\n", s);

  return (s);
}

int FifoDrain(fifo_t *f)
{
  char *szTaskCmd;
  bool_t bDone = FALSE;
  int nDrain = 0;

  while (! bDone )
  {
    if ((szTaskCmd=FifoGet(f)) != NULL)
    {
      Dprintf("FifoDrain: got szTaskCmd = %s\n", szTaskCmd);
      free(szTaskCmd);
      nDrain++;
    }
    else
      bDone = TRUE;
  }

  return nDrain;
}
