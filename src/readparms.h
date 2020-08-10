// readparms.h

#ifndef _READPARMS_H
#define _READPARMS_H

struct cfg {
  int nlevels;
  int fanout;
  int nfilesnode;
  int nfilesleaf;
  int filesizemu;
  int filesizesd;
};
typedef struct cfg cfg_t;;

int modval (char mod, int val);
void readparms (const char *filename, cfg_t *pcfg);

#endif // _READPARMS_H
