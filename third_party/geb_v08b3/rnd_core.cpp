/* rnd_core.cpp */
#include "rnd_core.h"
#include <string.h>

static int ix1,ix2,ix3;
static float r[98];
static int iff=0;
static int iset=0;
static Real gset;

//BEGIN NICKED (& SLIGHTLY MODIFIED [rnd_state]) ----
/*random.h
  Dave Cliff May 1991
  Some general random routines
*/

// now in header file: #define Real double
/*Real may be double or float: on some machines, double can be faster than float*/

/*rseed: reseed random number generator*/
void rseed(int *,int);

/*randval: return a (near)uniform distributed random number in the range 0..limit, as a Real*/
Real randval(Real);

/*irand: return a random integer in [0..limit-1]*/
int irand(int);

/*gaussrand: returns a normally distributed variable with zero mean and unit variance.
  Adapted from the "Numerical recipies in C" book by Press, Flannery, Teukolsky, and Vetterling, p.217
*/
Real gaussrand(void);

/*normrand: return a random deviate from a normal distribution with specified mean and standard distribution*/
Real normrand(Real mean,Real sd);
//------------------------------------
/*random.c
  Dave Cliff May 1991
*/
#include <math.h>
#include <time.h>
#include <stdio.h>
//------------------------------------ #include "random.h"

/*ran1 from the numerical recipes in c book -- it's the slowest but (?) best*/

/* ******************** ran1 ****************** */
#define M1 259200
#define IA1 7141
#define IC1 54773
#define RM1 (1.0/M1)
#define M2 134456
#define IA2 8121
#define IC2 28411
#define RM2 (1.0/M2)
#define M3 243000
#define IA3 4561
#define IC3 51349

float ran1(int *idum)
{
    //static int ix1,ix2,ix3;
    //static float r[98];
    float temp;
    //static int iff=0;
    int j;
    void nrerror();

    if (*idum < 0 || iff == 0) {
        iff=1;
        ix1=(IC1-(*idum)) % M1;
        ix1=(IA1*ix1+IC1) % M1;
        ix2=ix1 % M2;
        ix1=(IA1*ix1+IC1) % M1;
        ix3=ix1 % M3;
        for (j=1;j<=97;j++) {
            ix1=(IA1*ix1+IC1) % M1;
            ix2=(IA2*ix2+IC2) % M2;
            r[j]=(ix1+ix2*RM2)*RM1;
        }
        *idum=1;
    }
    ix1=(IA1*ix1+IC1) % M1;
    ix2=(IA2*ix2+IC2) % M2;
    ix3=(IA3*ix3+IC3) % M3;
    j=1 + ((97*ix3)/M3);
    if (j > 97 || j < 1)
    /* nrerror("RAN1: This cannot happen."); */
    { fprintf(stderr,"RAN1: This cannot happen."); fflush(stderr); }
    temp=r[j];
    r[j]=(ix1+ix2*RM2)*RM1;
    return temp;
}

#undef M1
#undef IA1
#undef IC1
#undef RM1
#undef M2
#undef IA2
#undef IC2
#undef RM2
#undef M3
#undef IA3
#undef IC3

/* ********************************************** */


/* NB Ran1 is not exported -- its masked by the old system-dependent
   random routines*/

/*rseed: reseed the random number generator from the system clock*/
/*if the argument is zero then the system clock is used, otherwise the
  argument is the seed*/
void rseed(int *s,int verbose)
{ time_t tseed;
  int    seed;

  if((*s)==0)
  { time(&tseed);
    seed=(int)(tseed%32767);
    *s=seed; }
  else seed=*s;
  if(verbose) fprintf(stdout,"\n: Seed is %d\n",seed);
  /* srandom(seed); */
  seed=seed*-1;
  ran1(&seed);
}

/*randval: return a (near)uniform distributed random number in the
  range 0..limit, as a Real*/
Real randval(Real limit)
{ float  rv;
  int    i=1;
  /*get a random value in the range 0..1*/
  /*rv=random()/MAX_RAND*/
  rv=ran1(&i);
  return(limit*((Real)rv));
}

/*irand: return a random integer in [0..limit-1]*/
int irand(int limit)
{ int ir;
  /*while loop is used to trap the exceptional case where
    the underlying deviate in [0,1] actually returns 1.00*/
  ir=limit;
  while(ir==limit)
  { ir=(int)(floor(randval((Real)limit))); }
  return(ir);
}

/*gaussrand: returns a normally distributed variable with zero mean and
  unit variance.
  This is adapted from the "Numerical recipies in C" book by Press, Flannery,
  Teukolsky, and Vetterling, p.217
*/
Real gaussrand(void)
{ //static int iset=0;
  //static Real gset;
  Real fac,r,v1,v2;

  if(iset==0)
  { do { v1=2.0*randval(1.0)-1.0;
         v2=2.0*randval(1.0)-1.0;
         r=(v1*v1)+(v2*v2);
       } while (r>=1.0);
    fac=sqrt(-2.0*log(r)/r);
    gset=v1*fac;
    iset=1;
    return(v2*fac);
  }
  else
  { iset=0;
    return(gset);
  }
}

/*normrand: return a random deviate from a normal distribution with specified mean and standard distribution*/
Real normrand(Real mean,Real sd)
{ return((sd*gaussrand())+mean); }
//END NICKED--------------------------

//OLD:
//#include <stdlib.h>
//#include <time.h>
//void randomize() {srand(time(0));}
////int random(int M) {return rand()%M;}
//int random(int M) {return (int) (((float)M)*rand()/(RAND_MAX+1.0));}

//From header: int randomize(const int seed=0); // 0 to seed from the system clock
int randomize(const int seed) {int rseed_value=seed; rseed(&rseed_value,0); return rseed_value;}
int random(int M) {if (M==0) return 0; return irand(M);}

void get_rnd_state(      rnd_state& s)
{ s.ix1=ix1; s.ix2=ix2; s.ix3=ix3;
  int n; for (n=0;n<98;n++) s.r[n]=r[n];
  s.iff=iff;
  s.iset=iset; s.gset=gset;
}

void set_rnd_state(const rnd_state& s)
{ ix1=s.ix1; ix2=s.ix2; ix3=s.ix3;
  int n; for (n=0;n<98;n++) r[n]=s.r[n];
  iff=s.iff;
  iset=s.iset; gset=s.gset;
}

int rnd_file__save(FILE *file)
{ if (!fprintf(file,"%d %d %d ",ix1,ix2,ix3)) return 1;
  int n; for (n=0;n<98;n++) if (!fprintf(file,"%a ",r[n])) return 1;
  if (!fprintf(file,"%d ",iff)) return 1;
  if (!fprintf(file,"%d ",iset)) return 1;
  if (!fprintf(file,"%a",gset)) return 1;
  fprintf(file,"\n"); // not needed but nice to have
  return 0;
}

int rnd_file__load(FILE *file)
{ if (fscanf(file,"%d %d %d ",&ix1,&ix2,&ix3)==EOF) return 1;
  int n; for (n=0;n<98;n++) if (fscanf(file,"%a ",&(r[n]))==EOF) return 1;
  if (fscanf(file,"%d ",&iff)==EOF) return 1;
  if (fscanf(file,"%d ",&iset)==EOF) return 1;
  if (fscanf(file,"%lf",&gset)==EOF) return 1;
  return 0;
}

int rnd_file__save(const char *file_name)
{ FILE *file = fopen(file_name,"w");
  if (!file)
  { fprintf(stderr,"%s%s%s","Failed to open file ",file_name," for writing random number state\n"); fflush(stderr);
    return 1;
  }
  int ret=rnd_file__save(file);
  fclose(file);
  return ret;
}

int rnd_file__load(const char *file_name)
{ FILE *file;
  if (!strcmp(file_name,"-")) file = stdin;
  else
  { file = fopen(file_name,"r");
    if (!file)
    { fprintf(stderr,"%s%s%s","Failed to open file ",file_name," for reading random number state\n"); fflush(stderr);
      return 1;
    }
  }
  int ret=rnd_file__load(file);
  fclose(file);
  return ret;
}
