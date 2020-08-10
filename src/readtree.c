// readtree.c

// read the file tree under the specified directory into a filename vector

#define _GNU_SOURCE

#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include "rtcommon.h"
#include "readtree.h"
#include "readparms.h"
#include "timer.h"

#define NFILES_CHECK (15)

unsigned int g_nfilesread = 0;
char **g_filename;
unsigned long *g_filesize;
pthread_mutex_t g_lock;

static void *
ReadFiles (void* arg)
{
  uint i;
  int l;
  unsigned long nbytes;
  simpletimer_t t;
  uint etus = 0;
  struct stat statbuf;
  char path[FILENAME_MAX];
  char pathcomp[FILENAME_MAX];
  uint n;

  threadinfo_t *thi = (threadinfo_t *) arg;
  sharedtaskinfo_t *sti = thi->sti;
  rtstats_t *rs = thi->rs;

  // read nfiles files (completely)
  for (i = 0; i < sti->rp->nReadsperThread; i++)
  {
    // printf("fanout = %d, nfilesleaf = %d\n", sti->cfg->fanout, sti->cfg->nfilesleaf);
    strcpy(path, sti->rp->dirPath);
    if (path[strlen(path)-1] != '/')
      strcat(path, "/");
    for (l = 0; l < sti->cfg->nlevels; l++)
    {
      // printf ("%f\n", (((double) rand() / RAND_MAX)));
      int x = randrangeint(thi->pdrd, 0, sti->cfg->fanout - 1);
      snprintf(pathcomp, 6, "%02d/", x);
      // sprintf(pathcomp, "%02d/", randrangeint(thi->pdrd, 0, sti->cfg->fanout - 1));
      strcat(path, pathcomp);
    }

    sprintf(pathcomp, "%08d.dat", randrangeint(thi->pdrd, 0, sti->cfg->nfilesleaf - 1));
    strcat(path, pathcomp);

    n = (unsigned int) randrangeint(thi->pdrd, 1, 100);
    Dprintf ("n = %d, sti->rp-nMetaPct = %d\n", n, sti->rp->nMetaPct);
    if (n < sti->rp->nMetaPct)
    {
      stat(path, &statbuf);
      rs->nMeta++;
    }
    else
    {
      usleep(sti->rp->sleepus);
      gettimeofday(&t.tv_start, NULL);
      nbytes = readfile(path, thi);
      gettimeofday(&t.tv_end, NULL);

      etus = timer_elapsed_us(t);
      rs->nfiles++;
      rs->nbytes += nbytes;
      rs->procTime += (double) etus / 1000000.;

      rs->nTotLatencyUs += etus;
      if (etus < rs->nMinLatencyUs)
	rs->nMinLatencyUs = etus;
      if (etus > rs->nMaxLatencyUs)
	rs->nMaxLatencyUs = etus;
      if (etus > 2000)
	rs->n2msLatencyCount++;
      rs->aLatencyHist[min((NHISTBINS - 1), (int) (etus / HISTBINSIZE))]++;

      // First check is not under lock; need to recheck once we've
      // acquired the lock and make sure we should still update.  Else there
      // is a nasty race condition that can overrun the array.
      // This is done rather than locking every time, as only a few records
      // will get checked and we don't want to incur the penalty for every
      // read.

      if (g_nfilesread < NFILES_CHECK)
      {
	pthread_mutex_lock(&g_lock);

	// check again to prevent overrun race condition; just bail if "full"
	if (g_nfilesread < NFILES_CHECK)
	{
	  // printf (" ... sum = %d\n", sum);
	  g_filename[g_nfilesread] = strdup(path);
	  g_filesize[g_nfilesread] = nbytes;
	  g_nfilesread++;
	}
	
	pthread_mutex_unlock(&g_lock);
      }
    }
  }

  pthread_exit(NULL);
}

// readfile()
// return number of bytes read

unsigned long
readfile (char *path, threadinfo_t *thi)
{
  unsigned long nbytes_read = 0;

  int fd;
  char *buf;
  struct stat statbuf;
  size_t len;


  stat (path, &statbuf);
  len = statbuf.st_size;

  fd = open (path, O_RDONLY);
  if (fd < 0)
  {
    printf ("Unable to open file: %s\n", path);
    exit (1);
  }

  buf = (char *) malloc (len);
  nbytes_read = read(fd, buf, len);
  if (nbytes_read != len)
    printf ("Incorrect read file %s: len = %zi, nbytes_read = %ld\n",
	    path, len, nbytes_read);

  close(fd);
  free(buf);

  Dprintf ("readfile %s, len = %ld\n", path, nbytes_read);

  return (nbytes_read);
}

void Usage (void)
{
  fprintf (stderr, "Usage (Files): readtree [-t <nthreads>] -n <nfiles> -m <metapct> -c <cfgfilename> <rootpath>\n");
  fprintf (stderr, "       -t <nthreads>    is the number of write threads(8)\n");
  fprintf (stderr, "       -n <nfiles>      is the number of files to read PER THREAD\n");
  fprintf (stderr, "       -m <metapct>     is the percentage of metadata ops to perform\n");
  fprintf (stderr, "       -s <sleepus>     is the number of microseconds to sleep between IOPS (0)\n");
  fprintf (stderr, "       -c <cfgfilename> is gentree config file\n");
  fprintf (stderr, "       <rootpath> is the tree root\n");

  exit (EXIT_FAILURE);
}

int main (int argc, char *argv[])
{
  char *dirpath;
  cfg_t cfg;
  simpletimer_t t;

  double et_readfiles;
  uint i, j;
  int opt;
  fifo_t f;
  rtparams_t rtparams;
  sharedtaskinfo_t sti;
  threadinfo_t *thi;

  uint nfiles = 0;
  uint nMeta = 0;
  unsigned long nbytes = 0;
  double procTime = 0.0;
  simpletimer_t tb;
  simpletimer_t trs; // use to seed random number generator

  uint nTotLatencyUs = 0;
  uint nMinLatencyUs = INT_MAX;
  uint nMaxLatencyUs = 0;
  uint n2msLatencyCount = 0;
  uint aLatencyHist[NHISTBINS];
  memset(aLatencyHist, 0, NHISTBINS * sizeof(uint));

  if (argc == 1)
    Usage ();

  // Defaults
  rtparams.nThreads = 8;
  rtparams.szCfgFilename = NULL;
  rtparams.nReadsperThread = 0;
  rtparams.nMetaPct = 0;
  rtparams.bUseFsync = FALSE;
  rtparams.sleepus = 0;

  while ((opt = getopt(argc, argv, "t:c:m:s:n:")) != -1)
  {
    switch (opt)
    {
    case 't':
      rtparams.nThreads = atoi(optarg);
      break;
    case 'c':
      rtparams.szCfgFilename = optarg;
      break;
    case 'm':
      rtparams.nMetaPct = atoi(optarg);
      break;
    case 's':
      rtparams.sleepus = atoi(optarg);
      break;
    case 'n':
      rtparams.nReadsperThread = atoi(optarg);
      break;
    default: /* '?' */
      Usage();
    }
  }

  Dprintf ("optind = %d\n", optind);
  if (optind >= argc)
  {
    fprintf(stderr, "Expected argument after options\n");
    exit(EXIT_FAILURE);
  }
  dirpath = argv[optind++];
  Dprintf ("dirpath = %s\n", dirpath);

  if (rtparams.szCfgFilename == NULL)
  {
    fprintf(stderr, "missing -c flag (required)\n");
    Usage();
  }

  rtparams.dirPath = dirpath;
  readparms(rtparams.szCfgFilename, &cfg);

  g_filename = malloc(NFILES_CHECK * sizeof(char *));
  g_filesize = malloc(NFILES_CHECK * sizeof(unsigned long));

  printf ("readtree: reading contents of %d randomly selected files\n",
	  rtparams.nReadsperThread);
  printf ("          in each of %d threads\n", rtparams.nThreads);
  printf ("...\n");
  fflush(stdout);

  // Shared Task Info
  sti.cfg = &cfg;
  sti.fifo = &f;
  sti.rp = &rtparams;

  // init the g_lock
  pthread_mutex_init(&g_lock, NULL);

  // start the clock ...
  gettimeofday (&t.tv_start, NULL);

  // start a gang of threads looking to read files

  thi = (threadinfo_t *) calloc(rtparams.nThreads, sizeof(threadinfo_t));
  if (thi == NULL) {
    perror("calloc failed on threadinfo array");
    exit(EXIT_FAILURE);
  }

  Dprintf("rtparams.nThreads = %d\n", rtparams.nThreads);
  for (i = 0; i < rtparams.nThreads; i++)
  {
    thi[i].iThread = i;
    thi[i].sti = &sti;
    thi[i].rs = (rtstats_t *) malloc(sizeof(rtstats_t));

    thi[i].rs->nfiles = 0;
    thi[i].rs->nMeta = 0;
    thi[i].rs->nbytes = 0;
    thi[i].rs->procTime = 0;
    thi[i].rs->nTotLatencyUs = 0;
    thi[i].rs->nMinLatencyUs = INT_MAX;
    thi[i].rs->nMaxLatencyUs = 0;
    thi[i].rs->n2msLatencyCount = 0;
    memset(thi[i].rs->aLatencyHist, 0, NHISTBINS * sizeof(uint));

    thi[i].pdrd = (struct drand48_data *) malloc(sizeof(struct drand48_data));
    gettimeofday(&trs.tv_start, NULL);
    Dprintf("thread %i seeding with %ld\n", i, trs.tv_start.tv_usec);
    srand48_r(trs.tv_start.tv_usec, thi[i].pdrd);

    if (pthread_create(&thi[i].threadId, NULL, ReadFiles, &thi[i]))
    {
      perror("pthread_create failed");
      exit(EXIT_FAILURE);
    }

    Dprintf("started thread: %ld\n", thi[i].threadId);
  }

  Dprintf ("Joining threads ...\n");

  // Now go wait for the threads to finish
  for (i = 0; i < rtparams.nThreads; i++)
  {
    Dprintf("joining thread %ld\n", thi[i].threadId);
    if (pthread_join(thi[i].threadId, NULL))
      perror("pthread_join");
  }

  printf("pthread_join complete\n");

  gettimeofday (&t.tv_end, NULL);
  et_readfiles = timer_elapsed (t);
  tb.tv_start.tv_sec = INT_MAX;
  tb.tv_end.tv_sec = 0;

  for (i = 0; i < rtparams.nThreads; i++)
  {
    nMeta += thi[i].rs->nMeta;
    nfiles += thi[i].rs->nfiles;
    nbytes += thi[i].rs->nbytes;
    procTime += thi[i].rs->procTime;

    nTotLatencyUs += thi[i].rs->nTotLatencyUs;
    if (thi[i].rs->nMinLatencyUs < nMinLatencyUs)
      nMinLatencyUs = thi[i].rs->nMinLatencyUs;
    if (thi[i].rs->nMaxLatencyUs > nMaxLatencyUs)
      nMaxLatencyUs = thi[i].rs->nMaxLatencyUs;
    n2msLatencyCount += thi[i].rs->n2msLatencyCount;
    for (j = 0; j < NHISTBINS; j++)
      aLatencyHist[j] += thi[i].rs->aLatencyHist[j];

#ifdef UNDEF
    Dprintf("thr %d: thi tv_start.tv_sec=%ld, tb tv_start.tv_sec=%ld\n",
	   i, thi[i].rs->t.tv_start.tv_sec, tb.tv_start.tv_sec);
    Dprintf("        thi tv_end.tv_sec=%ld, tb tv_end.tv_sec=%ld\n",
	   thi[i].rs->t.tv_end.tv_sec, tb.tv_end.tv_sec);

    if (timercmp(&(thi[i].rs->t.tv_start), &(tb.tv_start), <))
      timer_start_copy(&tb, &thi[i].rs->t);
    if (timercmp(&(thi[i].rs->t.tv_end), &(tb.tv_end), >))
      timer_end_copy(&tb, &thi[i].rs->t);
#endif // UNDEF

    // Dprintf("thr %d: elapsed = %8.3f\n", i, timer_elapsed (thi[i].rs->t));
    Dprintf("        (cumulative) nfiles = %d\n", nfiles);
    Dprintf("        (cumulative) procTime = %8.3f\n", procTime);
  }

  printf ("\n");
  printf ("Readtree: nMeta = %d, nFiles = %d, nBytes = %ld\n",
	  nMeta, nfiles, nbytes);
  printf ("          cumulative procTime = %8.3f\n", procTime);
  printf ("          elapsed time(): %8.3f secs\n", timer_elapsed(t));
  printf ("          read rate = %d objs/s\n",
	  (int) (nfiles / timer_elapsed(t)));
  printf ("\n");
  printf ("          min latency = %d us\n", (int) nMinLatencyUs);
  printf ("          max latency = %d us\n", (int) nMaxLatencyUs);
  printf ("          avg latency = %d us\n",
	  (int) (nTotLatencyUs / nfiles));
  printf ("          2ms+ latency count = %d\n", n2msLatencyCount);
  printf ("\n");

  for (j = 0; j < NHISTBINS; j++)
    printf ("          latency hist[%d..%d] = %d\n",
	    j * HISTBINSIZE, ((j+1) * HISTBINSIZE) - 1, aLatencyHist[j]);

  printf ("\n");
  printf ("Data check ...\n");
  for (i = 0; i < (uint) min(g_nfilesread, NFILES_CHECK); i++)
    printf ("[%2d]: %s (%ld)\n", i, g_filename[i], g_filesize[i]);

  // cleanup
  printf("Freeing thread structs\n");
  for (i = 0; i < rtparams.nThreads; i++)
  {
    free(thi[i].rs);
    free(thi[i].pdrd);
  }
  free(thi);

  // global arrays
  printf("Freeing global arrays\n");
  free (g_filesize);
  for (i = 0; i< g_nfilesread; i++)
    free (g_filename[i]);
  free (g_filename);

  printf ("Done ... exiting\n");
  fflush(stdout);

  exit(EXIT_SUCCESS);
}
