// genfile.c

// generate a file in the specified dir

#include <stdio.h>
#include "rtcommon.h"

#define MAXRECLEN (80)

int genfile(char *dirpath, char *filename, uint len, threadinfo_t *thi)
{
  char fullpath[PATH_MAX];
  char fmtdescr[MAXRECLEN];
  char *writebuf;
  char *writebufp;

  int byteswritten = 0;
  int recswritten = 0;
  sharedtaskinfo_t *sti = thi->sti;
  rtparams_t *rp = sti->rp;
  // fosinfo_t *fi = sti->fosinfo;
  int fd = -1;
  ssize_t writeres = 0;

  strcpy(fullpath, dirpath);
  strcat(fullpath, "/");
  strcat(fullpath, filename);

  fd = open (fullpath, O_CREAT | O_EXCL | O_RDWR , S_IRUSR | S_IWUSR);

  if (fd <= 0)
  {
    printf ("open failed: %s\n", fullpath);
    perror ("error");
    exit (2);
  }

  strcpy (fmtdescr, filename);
  // strcat (fmtdescr, "|");
  strcat (fmtdescr, "%03d|");

  writebuf = (char *) malloc (len + MAXRECLEN + 1);
  writebufp = writebuf;
  while ((uint) byteswritten < len)
  {
    // sprintf (writebuf, fmtdescr);
    sprintf (writebufp, fmtdescr, recswritten % 1000);
    byteswritten+=strlen(writebufp);
    recswritten++;
    writebufp += strlen(writebufp);
  }

  // Dprintf("[%ld]:genfile: about to write %s\n", pthread_self(), fullpath);

  writeres = write (fd, writebuf, len);
  if (writeres != len)
  {
    fprintf (stderr, "write failed: %d %ld\n", len, writeres);
    perror ("error");
    return (0);
  }

  free (writebuf);

  if (rp->bUseFsync)
    fsync (fd);

  close (fd);

  return (len);
}
