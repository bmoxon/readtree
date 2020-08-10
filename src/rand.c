// rand.c

#include "rand.h"

int randrangeint (struct drand48_data *pdrd, int a, int b)
{
  double dr;
  int rv;

  drand48_r(pdrd, &dr);
  rv = (int) (dr * (b - a + 1)) + a;

  return (rv);
}

long long randrangell (struct drand48_data *pdrd, long long a, long long b)
{
  double dr;
  long long rv;

  drand48_r(pdrd, &dr);
  rv = (long long) (dr * (b - a + 1)) + a;

  return (rv);
}

int randmnsd (struct drand48_data *pdrd, int m, int sd, int nsd)
{
  double dr;
  int rv;

  drand48_r(pdrd, &dr);
  rv = ((dr - 0.5) * nsd * sd) + m;

  return (rv);
}

