// rand.h

#ifndef _RAND_H
#define _RAND_H

#include <stdlib.h>

int randrangeint (struct drand48_data *pdrd, int a, int b);

long long randrangell (struct drand48_data *pdrd, long long a, long long b);

int randmnsd (struct drand48_data *pdrd, int m, int sd, int nsd);

#endif // _RAND_H
