// gentree.c

// generate a file tree under the specified directory

#define _GNU_SOURCE

#include "rtcommon.h"
#include "readparms.h"
#include "genfile.h"
#include "gentree.h"
#include "timer.h"

void gentree(char *dirpath, int lvl, threadinfo_t *thi)
{
  char path[255];
  char buf[80];
  char *szTaskCmd;
  int d;
  sharedtaskinfo_t *sti = thi->sti;
  rtparams_t *rp = thi->sti->rp;

  int fd;
 
  Dprintf ("In gentree: dirpath = %s, lvl = %d, cfg.nlevels = %d\n",
          dirpath, lvl, sti->cfg->nlevels);

  if (lvl == sti->cfg->nlevels)
  {
    // leaf node - just enqueue the path
    // thread will dequeue the "task", call WriteFiles, then
    // sync the directory if needed
    // it will also record write stats (per thread)

    szTaskCmd = (char *) malloc(strlen(dirpath) + 3);
    strcpy (szTaskCmd, "L:");
    strcat (szTaskCmd, dirpath);

    Dprintf("gentree: calling Fifoput with path %s\n", szTaskCmd);
    // Blocking Put
    FifoPutBlock(sti->fifo, szTaskCmd);

    free(szTaskCmd);

#ifndef DEBUG
    printf ("%d", lvl);
    fflush (stdout);
#endif /* DEBUG */

    return;
  }
  else
  {
    // non-leaf node - enqueue the path, fan out the directories, and recurse

    szTaskCmd = (char *) malloc(strlen(dirpath) + 3);
    strcpy (szTaskCmd, "N:");
    strcat (szTaskCmd, dirpath);

    Dprintf("gentree: calling Fifoput with path %s\n", szTaskCmd);
    // Blocking Put
    FifoPutBlock(sti->fifo, szTaskCmd);

    free(szTaskCmd);

    if (rp->bUseFsync)
    {
      // fsync the directory
      fd = open(dirpath, O_RDWR);
      fsync(fd);
      close(fd);
    }

#ifndef DEBUG
    printf ("[%d]", lvl);
    fflush (stdout);
#endif /* DEBUG */

    // gen fanout directories recursively
    for (d = 0; d < sti->cfg->fanout; d++)
    {
      strcpy (path, dirpath);
      sprintf (buf, "/%02d", d);
      strcat (path, buf);

      Dprintf("calling mkdir: %s\n", path);
      if (mkdir (path, 0755) != 0)
	perror("mkdir failed");

      gentree (path, lvl + 1, thi);

      if (rp->bUseFsync)
      {
	// fsync the directory
	fd = open(dirpath, O_RDWR);
	fsync(fd);
	close(fd);
      }
    }
  }
}

// Write files into specified directory - thread specific task

int WriteFiles(char *dirpath, threadinfo_t *thi)
{
  int len;
  int nbytes = 0;
  uint etus = 0;

  int f;
  char fn[80];
  char *sPath;
  int nFilesWrite;
  bool_t bLeaf = FALSE;
  int fd;
  simpletimer_t t;

  sharedtaskinfo_t *sti = thi->sti;
  rtstats_t *rs = thi->rs;
  rtparams_t *rp = sti->rp;

  nFilesWrite = sti->cfg->nfilesnode;
  if (strncmp(dirpath, "L:", 2) == 0)
    {
      bLeaf = TRUE;
      nFilesWrite = sti->cfg->nfilesleaf;
    }
  sPath = dirpath + 2;

  if (nFilesWrite != 0)
  {
    // write the node/leaf files and return
    for (f = 0; f < nFilesWrite; f++)
      {
	sprintf(fn, "%08d.dat", f);

	len = randmnsd(thi->pdrd, sti->cfg->filesizemu,
		       sti->cfg->filesizesd, 3);

	// Dprintf ("Calling genfile (%s, %s, %d)\n", sPath, fn, len);

	if (len > 0)
	  {
	    gettimeofday (&t.tv_start, NULL);
	    nbytes = genfile (sPath, fn, len, thi);
	    gettimeofday (&t.tv_end, NULL);

	    if (nbytes != len)
	      fprintf (stderr, "genfile failed: %s/%s, len=%d, nbytes=%d\n", sPath, fn, len, nbytes);
	    else
	      {
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

	      }
	  }
      }

    if (rp->bUseFsync)
    {
      // fsync the directory
      fd = open(dirpath, O_RDWR);
      fsync(fd);
      close(fd);
    }

    if (bLeaf)
      Dprintf ("leaf files: %d creates, %ld bytes, %8.3f et\n",
	       rs->nfiles, rs->nbytes, rs->procTime);
    else
      Dprintf ("node files: %d creates, %ld bytes, %8.3f et\n",
	       rs->nfiles, rs->nbytes, rs->procTime);
  }

  return (0);
}

static void *
GenFiles(void* arg)
{
  threadinfo_t *thi = (threadinfo_t *) arg;
  sharedtaskinfo_t *sti = thi->sti;

  fifo_t *f = sti->fifo;
  char *szTaskCmd;
  bool_t bDone = FALSE;
  int rc;

  // Keep trying to dequeue a task; pthread_exit() on "end"
  // If you pull "end", requeue it before quitting so the next thread can
  // pick up "end"
  // Will need to dequeue one last "end" after all threads are done.
  // FifoPut() creates a copy of the supplied string and enqueues it
  // FifoGet() should free the string pointer when done with it

  while (! bDone )
  {
    if ((szTaskCmd=FifoGet(f)) != NULL)
    {
      Dprintf("GenFiles: got szTaskCmd = %s\n", szTaskCmd);
      if (strcmp(szTaskCmd, "end") == 0)
      {
	Dprintf("thread %ld got 'end'\n", (ulong) pthread_self());
	bDone = TRUE;
	// Blocking Put
	FifoPutBlock(f, "end");
	assert (rc == 0);
      }
      else
      {
	Dprintf("Calling WriteFiles: path=%s\n", szTaskCmd);
	rc = WriteFiles(szTaskCmd, thi);
	assert (rc == 0);
      }
      free(szTaskCmd);
    }
  }

  pthread_exit(NULL);

}

void Usage (void)
{
  fprintf (stderr, "Usage (Files): gentree [-t <nthreads>] -c <cfgfilename> [-f] <rootpath>\n");
  fprintf (stderr, "       -t <nthreads>    is the number of write threads(8)\n");
  fprintf (stderr, "       -c <cfgfilename> is gentree config file\n");
  fprintf (stderr, "       -f use fsync() for filesystem writes\n");
  fprintf (stderr, "       <rootpath> is the tree root\n");

  exit (EXIT_FAILURE);
}

int main (int argc, char *argv[])
{
  char *dirpath;
  cfg_t cfg;
  uint i, j;
  // apr_queue_t *q;
  // apr_pool_t a;
  fifo_t f;
  int opt;
  rtparams_t rtparams;
  simpletimer_t t;
  simpletimer_t trs;
  sharedtaskinfo_t sti;
  threadinfo_t *thi;
  uint nfiles = 0;
  unsigned long nbytes = 0;
  double procTime = 0.0;

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
  rtparams.bUseFsync = FALSE;

  while ((opt = getopt(argc, argv, "t:c:f")) != -1)
  {
    switch (opt)
    {
    case 't':
      rtparams.nThreads = atoi(optarg);
      break;
    case 'c':
      rtparams.szCfgFilename = optarg;
      break;
    case 'f':
      rtparams.bUseFsync = TRUE;
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

  // Shared Task Info
  sti.cfg = &cfg;
  sti.fifo = &f;
  sti.rp = &rtparams;

  // Create a queue - this should be big enough for most scenarios
  // apr_queue_create (&q, 1000, &a);
  FifoCreate(&f, rtparams.nThreads);

  // start the clock ...
  gettimeofday (&t.tv_start, NULL);

  // start a gang of threads looking to write files at a node or leaf
  // main thread will do the tree-recurse, queueing new "tasks" to write
  // files

  thi = (threadinfo_t *) malloc(rtparams.nThreads * sizeof(threadinfo_t));
  if (thi == NULL)
  {
    perror("calloc failed on threadinfo array");
    exit(EXIT_FAILURE);
  }

  for (i = 0; i < rtparams.nThreads; i++)
  {
    thi[i].iThread = i;
    thi[i].sti = &sti;

    thi[i].rs = (rtstats_t *) malloc(sizeof(rtstats_t));

    thi[i].rs->nfiles = 0;;
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

    if (pthread_create(&thi[i].threadId, NULL, GenFiles, &thi[i]))
      {
	perror("pthread_create failed");
	exit(EXIT_FAILURE);
      }

    Dprintf("started thread: %ld\n", thi[i].threadId);
  }

  Dprintf("calling gentree: dirpath=%s\n", dirpath);

  gentree(dirpath, 0, thi);

  // Blocking Put
  FifoPutBlock(&f, "end");

  Dprintf ("Joining threads ...\n");

  for (i = 0; i < rtparams.nThreads; i++)
  {
    Dprintf("joining thread %ld\n", thi[i].threadId);
    if (pthread_join(thi[i].threadId, NULL))
      perror("pthread_join");
  }

  gettimeofday (&t.tv_end, NULL);

  printf ("\n\nGentree: %d threads\n", rtparams.nThreads);

  for (i = 0; i < rtparams.nThreads; i++)
  {
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

    Dprintf("thr %d: elapsed = %8.3f\n", i, timer_elapsed (thi[i].rs->t));
    Dprintf("        (cumulative) nfiles = %d, bytes = %ld\n",
	    nfiles, nbytes);
    Dprintf("        (cumulative) procTime = %8.3f\n", procTime);
  }

  printf ("\n");
  printf ("Gentree: nFiles = %d, nBytes =%ld\n", nfiles, nbytes);
  printf ("         cumulative procTime = %8.3f\n", procTime);
  printf ("         elapsed time(): %8.3f secs\n", timer_elapsed(t));
  printf ("         write rate = %d objs/s\n",
	  (int) (nfiles / timer_elapsed(t)));
  printf ("\n");
  printf ("         min latency = %d us\n", (int) nMinLatencyUs);
  printf ("         max latency = %d us\n", (int) nMaxLatencyUs);
  printf ("         avg latency = %d us\n",
	  (int) (nTotLatencyUs / nfiles));
  printf ("         2ms+ latency count = %d\n", n2msLatencyCount);
  printf ("\n");

  for (j = 0; j < NHISTBINS; j++)
    printf ("         latency hist[%d..%d] = %d\n",
	    j * HISTBINSIZE, ((j+1) * HISTBINSIZE) - 1, aLatencyHist[j]);

  // cleanup
  // should be one 'end' to drain
  i = FifoDrain(&f);
  assert (i == 1);

  FifoDestroy(&f);

  for (i = 0; i < rtparams.nThreads; i++)
  {
    free(thi[i].rs);
    free(thi[i].pdrd);
  }
  free(thi);

  printf ("Done ... exiting\n");
  fflush(stdout);

  exit(EXIT_SUCCESS);
}
