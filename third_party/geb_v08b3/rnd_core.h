#ifndef rnd_core_h
#define rnd_core_h
#include <stdio.h>

#define Real double
/*Real may be double or float: on some machines, double can be faster than float*/

int randomize(const int seed=0); // 0 to seed from the system clock
int random(int M);

struct rnd_state
{ int ix1,ix2,ix3;
  float r[98];
  int iff;
  int iset;
  Real gset;
};

void get_rnd_state(      rnd_state& s);
void set_rnd_state(const rnd_state& s);

int rnd_file__save(FILE *file);
int rnd_file__load(FILE *file);

int rnd_file__save(const char *file_name);
int rnd_file__load(const char *file_name);

#endif
