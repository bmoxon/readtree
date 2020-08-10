// readparms.c

// read the fs configuration parms

#include "rtcommon.h"
#include "readparms.h"

void readparms (const char *filename, cfg_t *pcfg)
{
  FILE *fp;
  char tok[80];
  char mod;
  int val;

  fp = fopen (filename, "r");
  if (fp == NULL)
  {
    printf ("fopen failed: %s\n", filename);
    perror ("error");
    exit (2);
  }

  while ( fscanf (fp, "%s %d%c\n", tok, &val, &mod) != EOF)
  {
    Dprintf ("Got tok = %s, val = %d, mod = %c\n", tok, val, mod);

    if (!strcmp(tok, "NLEVELS"))
      pcfg->nlevels = val;
    else if (!strcmp(tok, "FANOUT"))
      pcfg->fanout = val;
    else if (!strcmp(tok, "NFILESNODE"))
      pcfg->nfilesnode = val;
    else if (!strcmp(tok, "NFILESLEAF"))
      pcfg->nfilesleaf = val;
    else if (!strcmp(tok, "FILESIZEMU"))
      pcfg->filesizemu = modval(mod, val);
    else if (!strcmp(tok, "FILESIZESD"))
      pcfg->filesizesd = modval(mod, val);
    else
      printf ("Unexpected token %s\n", tok);
  }

  printf ("Got nlevels = %d, fanout = %d\n", pcfg->nlevels, pcfg->fanout);
  printf ("    nfilesnode = %d, nfilesleaf = %d\n", pcfg->nfilesnode, pcfg->nfilesleaf);
  printf ("    filesizemu = %d, filesizesd = %d\n", pcfg->filesizemu, pcfg->filesizesd);

  fclose (fp);
}

int modval (char mod, int val)
{
  int retval = val;

  switch (mod)
  {
    case 'K':
    case 'k':
      retval <<= 10;
      break;
    case 'M':
    case 'm':
      retval <<= 20;
      break;
    case 'G':
    case 'g':
      retval <<= 30;
      break;
  }

  return (retval);
}
