// rtcommon.h

#ifndef _RT_COMMON_H
#define _RT_COMMON_H

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <sys/time.h>
#include <assert.h>
#include <pthread.h>
#include <math.h>
#include <values.h>
#include <stdint.h>

#include <limits.h>
#include <linux/limits.h>

// #include "ldlfds.h"
// #include "apr_queue.h"
// #include "apr_pools.h"

#include "readparms.h"
#include "fifo.h"
#include "timer.h"
#include "rand.h"

// open(2) man page indicates this can only be used with 4K IOs
// and 4K-aligned buffers
//#define USE_O_DIRECT

// #define MAX_KEY_LEN    ((uint64_t)(256))
// #define MAX_VAL_LEN    ((uint64_t)(1 << 20))

typedef unsigned char bool_t;
#ifndef FALSE
#define FALSE (0)
#define TRUE (! FALSE)
#endif // FALSE

#define digits(n) ((int) (log10((double) n) + 1))

#define NHISTBINS (20)
#define HISTBINSIZE (2000)

// Per thread File/Object stats
struct RTStats
{
  long nbytes;
  double procTime;
  simpletimer_t t;
  uint nfiles;
  uint nMeta;
  uint nTotLatencyUs;
  uint nMinLatencyUs;
  uint nMaxLatencyUs;
  uint n2msLatencyCount;
  uint aLatencyHist[NHISTBINS];
};
typedef struct RTStats rtstats_t;

struct RTParams
{
  char *szCfgFilename;
  char *dirPath;
  uint nThreads;
  uint nReadsperThread;
  uint nMetaPct;
  ulong sleepus;
  bool_t bUseFsync;
};
typedef struct RTParams rtparams_t;

struct SharedTaskInfo {
  cfg_t *cfg;
  rtparams_t *rp;
  fifo_t *fifo;
};
typedef struct SharedTaskInfo sharedtaskinfo_t;

struct ThreadInfo {
  sharedtaskinfo_t *sti;
  struct drand48_data *pdrd;
  rtstats_t *rs;
  pthread_t threadId;
  uint iThread;
  uint exit_status;
};
typedef struct ThreadInfo threadinfo_t;

#ifdef DEBUG 
//#define _ADD_CR(s)                  s "\n"
#define _ADD_CR(s)                  s
#define Dprintf(fmt, args...)       do { \
printf(_ADD_CR(fmt), ##args); \
fflush(stdout); \
  } while (0)
#else 
#define Dprintf(fmt, args...)       do { } while (0)
#endif 

#define max(a, b) ( ((a) > (b)) ? (a) : (b) )
#define min(a, b) ( ((a) < (b)) ? (a) : (b) )

#endif /* _RT_COMMON_H */
