/* Copyright (c) 2011 Alastair Channon
* 
* Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
* 
* The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

/* geb_core.h */
#ifndef geb_core_h
#define geb_core_h
#include "ann_core.h"

#define only_generate_rules_for_shadows_when_needed
#ifndef only_generate_rules_for_shadows_when_needed
#warning NOT USING only_generate_rules_for_shadows_when_needed
#else   //only_generate_rules_for_shadows_when_needed
#endif  //only_generate_rules_for_shadows_when_needed

//#define compute_SHADOW_used_component_Complexities
#ifdef  compute_SHADOW_used_component_Complexities
#ifndef only_generate_rules_for_shadows_when_needed
#warning Using compute_SHADOW_used_component_Complexities
#else   //only_generate_rules_for_shadows_when_needed
#warning Using compute_SHADOW_used_component_Complexities and DISABLING incompatible option only_generate_rules_for_shadows_when_needed
#undef  only_generate_rules_for_shadows_when_needed
#endif  //only_generate_rules_for_shadows_when_needed
#endif  //compute_SHADOW_used_component_Complexities

extern int geb_seed;

extern int world_length;
extern int world_size;

extern Gene_string reproduce_character;
extern Gene_string kill_character_start;
extern Gene_string turn_anti_clockwise_character_start;
extern Gene_string turn_clockwise_character_start;
extern Gene_string move_forward_character_start;

extern long int GA_time;
extern long int stats__num_reproductions;

void ga_initialise();
void ga_main_loop();
void ga_terminate();

enum bearing {north,east,south,west};

extern int cosdeg64[360];
extern int sindeg64[360];

class Organism;                            //forward declaration //for geb_file
void reset_not_processed_arrays();         //forward declaration //for geb_file
extern int num_not_processed;                                    //for geb_file
extern int *not_processed_positions;                             //for geb_file
extern Organism **not_processed_organisms;                       //for geb_file
extern int stats__num_individuals;                               //for geb_file
extern int SHADOWstats__num_individuals;
extern Organism *protected_one;                                  //for geb_file
extern int limits__start_num_individuals;                        //for geb_file

//-----------------------------------------------------------------------------
// The following must be defined in the application that includes this file:
void show_individual(int at_x_64,int at_y_64,int direction_deg,int pen_colour);
void unshow_individual(int at_x_64,int at_y_64,int direction_deg);
//-----------------------------------------------------------------------------

class Organism : public Ann
#ifndef only_generate_rules_for_shadows_when_needed
{ //reproduce now returns a pointer to a new shadow organism - see geb_core.cpp:
  friend Organism* reproduce(Organism *parentA,Organism *parentB,int child_at,
                             int child_at_x_64,int child_at_y_64);
#else   //only_generate_rules_for_shadows_when_needed
{ //reproduce now returns a pointer to a new shadow GENOTYPE - see geb_core.cpp:
  friend Genotype* reproduce(Organism *parentA,Organism *parentB,int child_at,
                             int child_at_x_64,int child_at_y_64);
#endif  //only_generate_rules_for_shadows_when_needed
  public:    Organism(Genotype *with_genotype,int _at_x_64,int _at_y_64); //constructor
    virtual ~Organism(); //a shadow organism must be deleted
             void copy_net_inputs_from_organism_in_front();
             void actions();
             int pen_colour;
             void show_in_world_window();
             void unshow_in_world_window();
             int at,at_x,at_y;
             int at_x_64,at_y_64;
             bearing direction;
             int direction_deg;
             int birth_time;
             int stats__age; // statistic
  protected: void try_to_reproduce_with_organism_in_front();
             void try_to_kill_organism_in_front();
             void try_to_clone_into_organism_in_front();
             void turn_anti_clockwise(int by_deg);
             void turn_clockwise(int by_deg);
             void try_to_move_forward(int distance_64);
};

extern Organism **world;    // toroidal 2D world of Organisms | eg.: 0.......15
                            //                                |      16......31
                            //                                |      :    :   :
                            //                                |      240....255
#ifndef only_generate_rules_for_shadows_when_needed
extern Organism **SHADOWworld;
#else   //only_generate_rules_for_shadows_when_needed
extern Genotype **SHADOWgenotypes;
extern int *SHADOWstats__ages;
extern int *SHADOWbirth_times;
#endif  //only_generate_rules_for_shadows_when_needed

// Fgets to avoid "warning: ignoring return value of ‘char* fgets(char*, int, FILE*)’, declared with attribute warn_unused_result [-Wunused-result]"
extern void Fgets(char *s,int size,FILE *file);

// Fscanf to avoid "warning: ignoring return value of ‘int fscanf(FILE*, const char*, ...)’, declared with attribute warn_unused_result [-Wunused-result]"
template <class T>
void Fscanf(FILE *file,const char *format,T *p1=0,T *p2=0)
{ int ret,req=0; if (p1) req=1; if (p2) req=2;
  if      (req==0) ret = fscanf(file,format,0);// ",0" to avoid "warning: format not a string literal and no format arguments [-Wformat-security]"
  else if (req==1) ret = fscanf(file,format,p1);
  else             ret = fscanf(file,format,p1,p2);
  if (ret != req) {fprintf(stderr,"Error: Unexpected return value from fscanf\n"); fflush(stderr);}
}
extern void Fscanf(FILE *file,const char *format);

#endif
