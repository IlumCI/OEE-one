/* Copyright (c) 2008 Alastair Channon
* 
* Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
* 
* The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

/* geb_core.cpp */
#include "geb_core.h"
#include <stdlib.h>
//#include <time.h>
#include <math.h>

//#define ShadowLengthsFollowReals
#ifdef  ShadowLengthsFollowReals
#warning You are using ShadowLengthsFollowReals
#warning Make sure that you are not using reset_shadow_to_real_for_each_pass
#endif  //ShadowLengthsFollowReals

//defined (or not) in geb_core.h: only_generate_rules_for_shadows_when_needed
//#ifndef only_generate_rules_for_shadows_when_needed
//#warning NOT USING only_generate_rules_for_shadows_when_needed
//#else   //only_generate_rules_for_shadows_when_needed
//#endif  //only_generate_rules_for_shadows_when_needed

//#define test__use_four_inputs
#ifndef test__use_four_inputs
#else   //test__use_four_inputs
#warning: Using test__use_four_inputs - check same as ann_core.cpp
#endif  //test__use_four_inputs
int geb_seed=0; // default - seed from the system clock

//#define protect_one
#ifndef protect_one
#else   //protect_one
#warning: Using protect_one
//Organism *protected_one; //moved to two lines below for geb_file
#endif  //protect_one
Organism *protected_one;

//#define clone__reproduce_over
//#warning: Using clone__reproduce_over

#define disallow_reproduction_into_killer
#ifndef disallow_reproduction_into_killer
#warning: NOT Using disallow_reproduction_into_killer (NOT as all main versions so far)
#else   //disallow_reproduction_into_killer
//#warning: Using disallow_reproduction_into_killer (as all main versions so far)
#endif  //disallow_reproduction_into_killer

#define morphogenesis_during_life

#ifndef morphogenesis_during_life
const int priming_level = 5;
#else   //morphogenesis_during_life
const int priming_level = 2;
#endif  //morphogenesis_during_life

Gene_string reproduce_character;
Gene_string kill_character_start;
Gene_string turn_anti_clockwise_character_start;
Gene_string turn_clockwise_character_start;
Gene_string move_forward_character_start;

//int copy_to_length=2;
int copy_to_length=3;

int cosdeg64[360];
int sindeg64[360];
const int max_turn_deg=60;

const int limits__die_at_age =             -1; // set to -1 for none
//int   limits__min_num_individuals =        -1; // set to -1 for none
int   limits__min_num_individuals =        20; // set to -1 for none
//If your compiler does not understand "#warning" then comment the next line out.
#warning Using limits__min_num_individuals of 20
int   limits__max_num_individuals =        -1; // set to -1 for none
//int   limits__start_num_individuals =      -1; // set to -1 for none
int   limits__start_num_individuals =      50; // set to -1 for none
//Setting limits__start_num_individuals to 50 has no real effect - just looks fun
int   stats__num_individuals;                  // 
int   SHADOWstats__num_individuals;            // should be equal to stats__num_individuals except during kill/reproduce processing
int   limits__never_kill_or_move_first_  = -1; // set to -1 for none

int world_length = 20; //default
int world_size;

const int max_noise = 600;
const int t_a = 500;
const int t_v = t_a;
const int max_noise_on_isolated_outputs = 500;
//#warning: Using noise on isolated outputs

#define use__copy_scale_at_distance_of_1_64__256
#ifndef use__copy_scale_at_distance_of_1_64__256
// //const int copy_scale_at_distance_of_1_64 = 64; //64 => unscaled (128=>doubled,etc)
// const int copy_scale_at_distance_of_1_64 = 128;
const int copy_scale_64_at_distance_of_1_64 = 128 << 6;
#else   //use__copy_scale_at_distance_of_1_64__256
//geb_v06__default: #warning USING copy_scale_at_distance_of_1_64 of 256
// const int copy_scale_at_distance_of_1_64 = 256;
const int copy_scale_64_at_distance_of_1_64 = 256 << 6;
#endif  //use__copy_scale_at_distance_of_1_64__256

Organism **world;
#ifndef only_generate_rules_for_shadows_when_needed
Organism **SHADOWworld;
#else   //only_generate_rules_for_shadows_when_needed
Genotype **SHADOWgenotypes;
int *SHADOWstats__ages;
int *SHADOWbirth_times;
#endif  //only_generate_rules_for_shadows_when_needed

long int GA_time;
long int stats__num_reproductions;

int *not_processed_positions;
Organism **not_processed_organisms;
int num_not_processed;

const int genotype_length_change_prob = 100;
const int genotype_length_change_max  = 1;
const int num_mutations               = 1;

int forward1(int from_x_64,int from_y_64,int direction_deg)
{ //  position = x + y * world_length; x,y in [0,world_length-1]
  int x_64=from_x_64; int y_64=from_y_64;
  int delta_x_64= sindeg64[direction_deg];
  int delta_y_64=-cosdeg64[direction_deg];
  int x,from_x,y,from_y;
  x=from_x=(from_x_64>>6); y=from_y=(from_y_64>>6);
  while ((x==from_x) && (y==from_y))
  { x_64 += delta_x_64; y_64 += delta_y_64; 
    if (x_64<0) x_64+=world_length<<6; else if (x_64>=world_length<<6) x_64-=world_length<<6;
    if (y_64<0) y_64+=world_length<<6; else if (y_64>=world_length<<6) y_64-=world_length<<6;
    x=x_64>>6; y=y_64>>6;
  }
  return x + y * world_length;
}

Organism::Organism(Genotype *with_genotype,int _at_x_64,int _at_y_64) //constructor
        : Ann(with_genotype)
{ 
#ifndef morphogenesis_during_life
  morphogenesis();
#endif  //morphogenesis_during_life
  stats__age=0; //Statistic
  birth_time=GA_time;
  pen_colour=-1;//not set yet
  at_x_64=_at_x_64; at_y_64=_at_y_64;
  if ((at_x_64==-1) && (at_y_64==-1)) //SHADOW organisms don't have a position; -1,-1 used by Organism::~Organism to denote SHADOW
    {direction_deg=at_x=at_y=-1;direction=(bearing)0;return;} // keep even SHADOW position variables determined
  at_x=at_x_64>>6; at_y=at_y_64>>6;
  //  x: 'left' column is 0 , y: 'top'  row    is 0
  at=at_x+at_y*world_length;
  direction_deg = random(360);
  direction=(bearing) (direction_deg/90);
  //Prime the network (else can not have output on first time step(s)):
  //                  (else eg. get repeated overwrite of individual in front, at start)
  copy_net_inputs_from_organism_in_front();
  for (int rep=0;rep<priming_level;rep++) update_outputs();
}

//NB: In some old versions it was (very) lucky that (-1>>6)==-1, at least on the (Linux&g++&...) systems used.  This is no longer required.

Organism::~Organism() //destructor
{ if ((at_x_64==-1) && (at_y_64==-1)) return; //Only do the following if deleting a real organism (not a SHADOW organism):
  int SHADOWdel=random(SHADOWstats__num_individuals);
  SHADOWstats__num_individuals--;
#ifndef only_generate_rules_for_shadows_when_needed
  delete SHADOWworld[SHADOWdel];
  SHADOWworld[SHADOWdel] = SHADOWworld[SHADOWstats__num_individuals]; //keep in range [0..(-1+SHADOWstats__num_individuals)]
#else   //only_generate_rules_for_shadows_when_needed
  delete SHADOWgenotypes[SHADOWdel];
  SHADOWgenotypes[SHADOWdel]   = SHADOWgenotypes[SHADOWstats__num_individuals];   //keep in range [0..(-1+SHADOWstats__num_individuals)]
  SHADOWstats__ages[SHADOWdel] = SHADOWstats__ages[SHADOWstats__num_individuals]; //keep in range [0..(-1+SHADOWstats__num_individuals)]
  SHADOWbirth_times[SHADOWdel] = SHADOWbirth_times[SHADOWstats__num_individuals]; //keep in range [0..(-1+SHADOWstats__num_individuals)]
#endif  //only_generate_rules_for_shadows_when_needed
}

void Organism::copy_net_inputs_from_organism_in_front()
{ Node_ptr *node_ptr = net_inputs->next; //not dummy;
  for (;node_ptr;node_ptr=node_ptr->next)
    node_ptr->io_value = 0;              //blank
  for (int copy_x=at_x-copy_to_length;copy_x<=at_x+copy_to_length;copy_x++)
    for (int copy_y=at_y-copy_to_length;copy_y<=at_y+copy_to_length;copy_y++)
    { int world_copy_x = (copy_x + world_length) % world_length;
      int world_copy_y = (copy_y + world_length) % world_length;
      if ((world_copy_x==at_x) && (world_copy_y==at_y)) continue;
      Organism *other = world[world_copy_x+world_copy_y*world_length];
      if (!other) continue;
      long delta_x_64 = other->at_x_64 - at_x_64;
      long delta_y_64 = other->at_y_64 - at_y_64;
      if      (delta_x_64> (world_length<<(6-1))) delta_x_64 -= world_length<<6;
      else if (delta_x_64<-(world_length<<(6-1))) delta_x_64 += world_length<<6;
      if      (delta_y_64> (world_length<<(6-1))) delta_y_64 -= world_length<<6;
      else if (delta_y_64<-(world_length<<(6-1))) delta_y_64 += world_length<<6;
      long delta_length_64 = (long) sqrt((double)((delta_x_64*delta_x_64)+(delta_y_64*delta_y_64)));
      int copy_scale_64 = copy_scale_64_at_distance_of_1_64/delta_length_64;
      // some maths: [remember y+ve in geb is down]
      // ( x)     (cos phi)                    i(phi+theta)
      // (-y) = l.(sin phi)   ;  rotate:    l.e             = right + i.forward
      // => right = l.cos(phi+theta) = l.cos(phi).cos(theta) - l.sin(phi).sin(theta)
      //          =  x.cos(theta) + y.sin(theta)
      //  forward = l.sin(phi+theta) = l.sin(phi).cos(theta) + l.cos(phi).sin(theta)
      //          = -y.cos(theta) + x.sin(theta)
      int delta_right = delta_x_64*cosdeg64[direction_deg]
                      + delta_y_64*sindeg64[direction_deg];
      Gene right = (delta_right>0)?1:0;
#ifndef test__use_four_inputs
      for (node_ptr=net_inputs->next;node_ptr;node_ptr=node_ptr->next) //not dummy
        if (*(node_ptr->node->character.first_gene + 1) == right) //if correct left/rightness
        { Gene_string test_character = node_ptr->node->character;
          test_character.first_gene += 2; //ignore left/rightness gene
#else   //test__use_four_inputs
      int delta_forward = delta_x_64*sindeg64[direction_deg]
                        - delta_y_64*cosdeg64[direction_deg];
      Gene forward = (delta_forward>0)?1:0;
      for (node_ptr=net_inputs->next;node_ptr;node_ptr=node_ptr->next) //not dummy
        if (*(node_ptr->node->character.first_gene + 1) == right) //if correct left/rightness
        if (   (node_ptr->node->character.length() == 2)  // special case: "input all left" or "input all right"
            || (*(node_ptr->node->character.first_gene + 3) == forward) //if correct forward/backness
           )
        { Gene_string test_character = node_ptr->node->character;
          test_character.first_gene += 2; //ignore left/rightness gene
          if (node_ptr->node->character.length() != 2)  // [if not] special case: "input all left" or "input all right"
            test_character.first_gene += 2; //ignore left/rightness and forward/backness genes
#endif  //test__use_four_inputs
          for (Node_ptr *other_node_ptr = other->net_outputs->next;other_node_ptr;other_node_ptr=other_node_ptr->next)
            if (test_character.is_start_of(other_node_ptr->node->character))
              node_ptr->io_value += ((copy_scale_64 * other_node_ptr->io_value)>>6);
        }
    }
}

void Organism::actions()
{ Node_ptr *node_ptr; //not dummy
  long output_sum;
  pen_colour = -1; //default (black)
  for (output_sum=-1,node_ptr=net_outputs->next;node_ptr;node_ptr=node_ptr->next)
    if (reproduce_character.is_start_of(node_ptr->node->character))
    { if (output_sum == -1) output_sum=0;
      output_sum += node_ptr->io_value;
      if (output_sum>excitatory_scale)
      { output_sum = excitatory_scale;
        break;
      }
    }
  if (output_sum==-1) output_sum = random(max_noise_on_isolated_outputs);
#ifndef protect_one
#else   //protect_one
  if (this == protected_one) output_sum = 0;
#endif  //protect_one
  if (output_sum>t_a) //if net output > threshold
  { pen_colour = 1; //blue
    try_to_reproduce_with_organism_in_front();
  }
  for (output_sum=-1,node_ptr=net_outputs->next;node_ptr;node_ptr=node_ptr->next)
    if (kill_character_start.is_start_of(node_ptr->node->character))
    { if (output_sum == -1) output_sum=0;
      output_sum += node_ptr->io_value;
      if (output_sum>excitatory_scale)
      { output_sum = excitatory_scale;
        break;
      }
    }
  if (output_sum==-1) output_sum = random(max_noise_on_isolated_outputs);
// #ifndef protect_one
// #else   //protect_one
//   if (this == protected_one) output_sum = excitatory_scale;
// #endif  //protect_one
  if (output_sum>t_a) //if net output > threshold
  { pen_colour = 2; //red
#ifndef clone__reproduce_over
    try_to_kill_organism_in_front();
#else   //clone__reproduce_over
    try_to_clone_into_organism_in_front();
#endif  //clone__reproduce_over
  }
  for (output_sum=-1,node_ptr=net_outputs->next;node_ptr;node_ptr=node_ptr->next)
    if (turn_anti_clockwise_character_start.is_start_of(node_ptr->node->character))
    { if (output_sum == -1) output_sum=0;
      output_sum += node_ptr->io_value;
      if (output_sum>excitatory_scale)
      { output_sum = excitatory_scale;
        break;
      }
    }
  if (output_sum==-1) output_sum = random(max_noise_on_isolated_outputs);
// #ifndef protect_one
// #else   //protect_one
//   if (this == protected_one) output_sum = random(max_noise_on_isolated_outputs);
// #endif  //protect_one
  turn_anti_clockwise((max_turn_deg*output_sum)>>excitatory_scale_shift);
  for (output_sum=-1,node_ptr=net_outputs->next;node_ptr;node_ptr=node_ptr->next)
    if (turn_clockwise_character_start.is_start_of(node_ptr->node->character))
    { if (output_sum == -1) output_sum=0;
      output_sum += node_ptr->io_value;
      if (output_sum>excitatory_scale)
      { output_sum = excitatory_scale;
        break;
      }
    }
  if (output_sum==-1) output_sum = random(max_noise_on_isolated_outputs);
// #ifndef protect_one
// #else   //protect_one
//   if (this == protected_one) output_sum = random(max_noise_on_isolated_outputs);
// #endif  //protect_one
  turn_clockwise((max_turn_deg*output_sum)>>excitatory_scale_shift);
  for (output_sum=-1,node_ptr=net_outputs->next;node_ptr;node_ptr=node_ptr->next)
    if (move_forward_character_start.is_start_of(node_ptr->node->character))
    { if (output_sum == -1) output_sum=0;
      output_sum += node_ptr->io_value;
      if (output_sum>excitatory_scale)
      { output_sum = excitatory_scale;
        break;
      }
    }
  if (output_sum==-1) output_sum = random(max_noise_on_isolated_outputs);
// #ifndef protect_one
// #else   //protect_one
//   if (this == protected_one) output_sum = excitatory_scale;
// #endif  //protect_one
  try_to_move_forward(((output_sum)<<6)>>excitatory_scale_shift);
  //any further net outputs produce no hard coded action
#ifndef protect_one
#else   //protect_one
  if (this == protected_one) pen_colour = 3;
#endif  //protect_one
  show_in_world_window(); //to update colour (pen_colour)
}

void Organism::show_in_world_window()
{ show_individual(at_x_64,at_y_64,direction_deg,pen_colour);
}

void Organism::unshow_in_world_window()
{ unshow_individual(at_x_64,at_y_64,direction_deg);
}

void Organism::try_to_reproduce_with_organism_in_front()
{ int other_parent_at = forward1(at_x_64,at_y_64,direction_deg);
  Organism *other_parent = world[other_parent_at];
  if (!other_parent) return; // if no organism there then do nothing
  int other_parent_at_x_64 = other_parent->at_x_64;
  int other_parent_at_y_64 = other_parent->at_y_64;
  int child_at_x_64,child_at_y_64;
#ifndef clone__reproduce_over
  int child_at = forward1(other_parent_at_x_64,other_parent_at_y_64,direction_deg); //note this one's direction_deg
  if (  (world[child_at])
      ||(stats__num_individuals == limits__max_num_individuals)
     )
  {
#else   //clone__reproduce_over
    int
#endif  //clone__reproduce_over
    child_at = other_parent_at;
#ifndef disallow_reproduction_into_killer
#else   //disallow_reproduction_into_killer
    if (other_parent->pen_colour == 2) return;
#endif  //disallow_reproduction_into_killer
#ifndef protect_one
#else   //protect_one
    if (other_parent == protected_one) return;
#endif  //protect_one
    child_at_x_64 = other_parent_at_x_64;
    child_at_y_64 = other_parent_at_y_64;
#ifndef clone__reproduce_over
  }
  else
  { stats__num_individuals++;
    child_at_x_64 = ((child_at % world_length)<<6)+32;
    child_at_y_64 = ((child_at / world_length)<<6)+32;
  }
#endif  //clone__reproduce_over
#ifndef only_generate_rules_for_shadows_when_needed
  Organism* SHADOWchild = reproduce(this,other_parent,child_at,child_at_x_64,child_at_y_64);
#else   //only_generate_rules_for_shadows_when_needed
  Genotype* SHADOWchild = reproduce(this,other_parent,child_at,child_at_x_64,child_at_y_64);
#endif  //only_generate_rules_for_shadows_when_needed
  if (child_at == other_parent_at)
  { other_parent->unshow_in_world_window();
    delete other_parent; //because child replaces
  }
#ifndef only_generate_rules_for_shadows_when_needed
  SHADOWworld[SHADOWstats__num_individuals] = SHADOWchild; SHADOWstats__num_individuals++;
#else   //only_generate_rules_for_shadows_when_needed
  SHADOWgenotypes[SHADOWstats__num_individuals] = SHADOWchild;
  SHADOWstats__ages[SHADOWstats__num_individuals] = 0;
  SHADOWbirth_times[SHADOWstats__num_individuals] = GA_time;
  SHADOWstats__num_individuals++;
#endif  //only_generate_rules_for_shadows_when_needed
//no #ifndef protect_one
//no #else   //protect_one
//no   if (this == protected_one) protected_one = world[child_at];
//no #endif  //protect_one
  world[child_at]->show_in_world_window();
}

void Organism::try_to_kill_organism_in_front()
{ //'try_to' because there may not be an Organism one step in front
  if (stats__num_individuals == limits__min_num_individuals) return;
  int other_at = forward1(at_x_64,at_y_64,direction_deg);
  if (other_at < limits__never_kill_or_move_first_) return;
  Organism *other = world[other_at];
#ifndef protect_one
#else   //protect_one
  if (other == protected_one) return;
#endif  //protect_one
  if (other)  // if there is an Organism one step in front
  { other->unshow_in_world_window();
    delete other;
    world[other_at] = 0;
    stats__num_individuals--;
  }
}

void Organism::try_to_clone_into_organism_in_front()
{ int other_at = forward1(at_x_64,at_y_64,direction_deg);
  Organism *other = world[other_at];
  if (!other) return; // if no organism there then do nothing
  other->unshow_in_world_window();
  //Copy genotype:
  Genotype *new_genotype = new Genotype;
  new_genotype->first_gene = new Gene [genotype->length()];
  new_genotype->last_gene  = new_genotype->first_gene + genotype->length() -1;
  Gene *parent_gene,*new_gene;
  for (parent_gene=genotype->first_gene,
         new_gene=new_genotype->first_gene;
       parent_gene <= genotype->last_gene;
       parent_gene++,new_gene++)
    *new_gene = *parent_gene;
  //mutation:
  for (int mutation=0;mutation<num_mutations;mutation++)
  { Gene *mutate_gene = new_genotype->first_gene + random(new_genotype->length());
    if (*mutate_gene == 0) *mutate_gene = 1;
    else                   *mutate_gene = 0;
  }
  world[other_at] = new Organism(new_genotype,other->at_x_64,other->at_y_64);
  delete other;
  world[other_at]->show_in_world_window();
}

void Organism::turn_anti_clockwise(int by_deg)
{ if (!by_deg) return;
  unshow_in_world_window();
  direction_deg = (direction_deg + 360 - (by_deg%360))%360;
  direction=(bearing) (direction_deg/90);
  show_in_world_window();
}

void Organism::turn_clockwise(int by_deg)
{ if (!by_deg) return;
  unshow_in_world_window();
  direction_deg = (direction_deg + by_deg)%360;
  direction=(bearing) (direction_deg/90);
  show_in_world_window();
}

void Organism::try_to_move_forward(int distance_64)
{ if (!distance_64) return;
  if (at < limits__never_kill_or_move_first_) return;
  int to_x_64=at_x_64 + ((distance_64*sindeg64[direction_deg])/(1<<6));
  if (to_x_64<0)                       to_x_64+=(world_length<<6);
  else if (to_x_64>=(world_length<<6)) to_x_64-=(world_length<<6);
  int to_y_64=at_y_64 - ((distance_64*cosdeg64[direction_deg])/(1<<6));
  if (to_y_64<0)                       to_y_64+=(world_length<<6);
  else if (to_y_64>=(world_length<<6)) to_y_64-=(world_length<<6);
  int to_x=to_x_64>>6; int to_y=to_y_64>>6;
  int to=to_x+to_y*world_length;
  if ((to==at) || !world[to]) //should only move to another 'square' if nothing there;
  { unshow_in_world_window();
    if (to!=at)
    { world[to] = world[at];
      world[at] = 0;
      at=to;
    }
    at_x_64=to_x_64; at_y_64=to_y_64; 
    at_x=to_x; at_y=to_y; 
    show_in_world_window();
  }
}

#ifndef only_generate_rules_for_shadows_when_needed
//reproduce now returns a pointer to a new shadow organism.
//If reproducing over a parent, must wait until old parent & a
//shadow have been deleted before adding the new shadow to the
//shadow world.  Adding the new shadow here could even cause
//SHADOWworld to overflow, corrupting other parts of memory.
Organism* reproduce(Organism *parentA,Organism *parentB,int child_at,int child_at_x_64,int child_at_y_64)
#else   //only_generate_rules_for_shadows_when_needed
//reproduce now returns a pointer to a new shadow GENOTYPE.
//If reproducing over a parent, must wait until old parent & a
//shadow have been deleted before adding the new shadow to the
//shadow world.  Adding the new shadow here could even cause
//SHADOWworld to overflow, corrupting other parts of memory.
Genotype * reproduce(Organism *parentA,Organism *parentB,int child_at,int child_at_x_64,int child_at_y_64)
#endif  //only_generate_rules_for_shadows_when_needed
{ //note: assumes world[child] == 0
  Organism *parent1,*parent2;
  switch (random(2))
  { case 0: parent1 = parentA; parent2 = parentB; break;
    case 1: parent1 = parentB; parent2 = parentA; break;
  }
  Genotype *SHADOWparent1__genotype,*SHADOWparent2__genotype;
#ifndef only_generate_rules_for_shadows_when_needed
  SHADOWparent1__genotype=SHADOWworld[random(SHADOWstats__num_individuals)]->genotype;
  for (SHADOWparent2__genotype=SHADOWparent1__genotype;SHADOWparent2__genotype==SHADOWparent1__genotype;SHADOWparent2__genotype=SHADOWworld[random(SHADOWstats__num_individuals)]->genotype); 
#else   //only_generate_rules_for_shadows_when_needed
  SHADOWparent1__genotype=SHADOWgenotypes[random(SHADOWstats__num_individuals)];
  for (SHADOWparent2__genotype=SHADOWparent1__genotype;SHADOWparent2__genotype==SHADOWparent1__genotype;SHADOWparent2__genotype=SHADOWgenotypes[random(SHADOWstats__num_individuals)]); 
#endif  //only_generate_rules_for_shadows_when_needed
  //crossover: genes from parent1: (0;<crossover_point1;++) [may be none]
  //                 from parent2: (crossover_point2;...;++) [at least last]
  int crossover_point1 = random(parent1->genotype->length() +1);
  int crossover_point2 = crossover_point1;
  int SHADOWcrossover_point1 = random(SHADOWparent1__genotype->length() +1);
  int SHADOWcrossover_point2 = SHADOWcrossover_point1;
  if (random(100) < genotype_length_change_prob)
  { if (random(2)) crossover_point2-=genotype_length_change_max;
    else           crossover_point2+=genotype_length_change_max;
  }
#ifndef ShadowLengthsFollowReals
  if (random(100) < genotype_length_change_prob)
  { if (random(2)) SHADOWcrossover_point2-=genotype_length_change_max;
    else           SHADOWcrossover_point2+=genotype_length_change_max;
  }
#else   //ShadowLengthsFollowReals
  if ( (      crossover_point1 + (      parent2->genotype->length()) -       crossover_point2)
      >(SHADOWcrossover_point1 + (SHADOWparent2__genotype->length()) - SHADOWcrossover_point2)
     )
    SHADOWcrossover_point2-=genotype_length_change_max;
  else
    SHADOWcrossover_point2+=genotype_length_change_max;
#endif  //ShadowLengthsFollowReals
  if (crossover_point2 < 0) crossover_point2 = 0;
  if (crossover_point2 >= parent2->genotype->length())
    crossover_point2 = parent2->genotype->length() - 1;  // last gene
  if (SHADOWcrossover_point2 < 0) SHADOWcrossover_point2 = 0;
  if (SHADOWcrossover_point2 >= SHADOWparent2__genotype->length())
    SHADOWcrossover_point2 = SHADOWparent2__genotype->length() - 1;  // last gene
  int new_genotype_length = crossover_point1
                          + (parent2->genotype->length()) - crossover_point2;
  int SHADOWnew_genotype_length = SHADOWcrossover_point1
                          + (SHADOWparent2__genotype->length()) - SHADOWcrossover_point2;
  Genotype *new_genotype = new Genotype;
  Genotype *SHADOWnew_genotype = new Genotype;
  new_genotype->first_gene = new Gene [new_genotype_length];
  new_genotype->last_gene  = new_genotype->first_gene + new_genotype_length -1;
  SHADOWnew_genotype->first_gene = new Gene [SHADOWnew_genotype_length];
  SHADOWnew_genotype->last_gene  = SHADOWnew_genotype->first_gene + SHADOWnew_genotype_length -1;
  Gene *parent_gene,*new_gene;
  for (parent_gene=parent1->genotype->first_gene,
         new_gene=new_genotype->first_gene;
       crossover_point1;
       crossover_point1--,parent_gene++,new_gene++)
    *new_gene = *parent_gene;
  Gene *SHADOWparent_gene,*SHADOWnew_gene;
  for (SHADOWparent_gene=SHADOWparent1__genotype->first_gene,
         SHADOWnew_gene=SHADOWnew_genotype->first_gene;
       SHADOWcrossover_point1;
       SHADOWcrossover_point1--,SHADOWparent_gene++,SHADOWnew_gene++)
    *SHADOWnew_gene = *SHADOWparent_gene;
  for (parent_gene=(parent2->genotype->first_gene)+crossover_point2;
       parent_gene <= parent2->genotype->last_gene;
       parent_gene++,new_gene++)
    *new_gene = *parent_gene;
  for (SHADOWparent_gene=(SHADOWparent2__genotype->first_gene)+SHADOWcrossover_point2;
       SHADOWparent_gene <= SHADOWparent2__genotype->last_gene;
       SHADOWparent_gene++,SHADOWnew_gene++)
    *SHADOWnew_gene = *SHADOWparent_gene;
  //mutation:
  for (int mutation=0;mutation<num_mutations;mutation++)
  { Gene *mutate_gene = new_genotype->first_gene + random(new_genotype_length);
    if (*mutate_gene == 0) *mutate_gene = 1;
    else                   *mutate_gene = 0;
  }
  for (int SHADOWmutation=0;SHADOWmutation<num_mutations;SHADOWmutation++)
  { Gene *SHADOWmutate_gene = SHADOWnew_genotype->first_gene + random(SHADOWnew_genotype_length);
    if (*SHADOWmutate_gene == 0) *SHADOWmutate_gene = 1;
    else                         *SHADOWmutate_gene = 0;
  }
  world[child_at] = new Organism(new_genotype,child_at_x_64,child_at_y_64);
  stats__num_reproductions++;
#ifndef only_generate_rules_for_shadows_when_needed
  return new Organism(SHADOWnew_genotype,-1,-1); //SHADOW organisms don't have a position; -1,-1 used by Organism::~Organism to denote SHADOW
#else   //only_generate_rules_for_shadows_when_needed
  return SHADOWnew_genotype;
#endif  //only_generate_rules_for_shadows_when_needed
}

#ifndef test__use_four_inputs
void initialise_action_characters()
{ reproduce_character.create_as_character("01");
  kill_character_start.create_as_character("100");
  turn_anti_clockwise_character_start.create_as_character("101");
  turn_clockwise_character_start.create_as_character("110");
  move_forward_character_start.create_as_character("111");
}
#else   //test__use_four_inputs
void initialise_action_characters()
{ reproduce_character.create_as_character("11");
  kill_character_start.create_as_character("000");
  turn_anti_clockwise_character_start.create_as_character("001");
  turn_clockwise_character_start.create_as_character("010");
  move_forward_character_start.create_as_character("011");
}
#endif  //test__use_four_inputs

void reset_not_processed_arrays() //setup for a new cycle
{ GA_time++;  //start of a new cycle
  num_not_processed = 0;
  for (int individual_at=0;individual_at<world_size;individual_at++)
    if (world[individual_at]) // if there is an Organism at this position
    { not_processed_positions[num_not_processed] = individual_at;
      not_processed_organisms[num_not_processed] = world[individual_at];
      num_not_processed++;
    }
}

void ga_initialise()
{ geb_seed = randomize(geb_seed); // if zero then get a new seed (based on the system clock)
  for (int theta=0;theta<360;theta++)
  { cosdeg64[theta] = (int)(64.0*cos(0.017453293*theta));
    sindeg64[theta] = (int)(64.0*sin(0.017453293*theta));
  }
  // PI/180 approx== 0.017453293
  world_size = world_length * world_length;
  not_processed_positions = new int[world_size];
  not_processed_organisms = new Organism*[world_size];
  world = new Organism *[world_size];
#ifndef only_generate_rules_for_shadows_when_needed
  SHADOWworld = new Organism *[world_size];
#else   //only_generate_rules_for_shadows_when_needed
  SHADOWgenotypes = new Genotype *[world_size];
  SHADOWstats__ages = new int[world_size];
  SHADOWbirth_times = new int[world_size];
#endif  //only_generate_rules_for_shadows_when_needed
  if (limits__min_num_individuals == -1)
    limits__min_num_individuals = 0;
  if (  (limits__max_num_individuals == -1)
      ||(limits__max_num_individuals > world_size)
     )
    limits__max_num_individuals = world_size;
  if (  (limits__start_num_individuals == -1)
      ||(limits__start_num_individuals > limits__max_num_individuals/*world_size*/)
     )
    limits__start_num_individuals = limits__max_num_individuals/*world_size*/;
  if (2*copy_to_length+1 > world_length) copy_to_length = (world_length-1)/2;
  //Next line needed because of copy_net_inputs_... in network priming during program initialisation
  int at;
  for (at=0;at<world_size;at++) world[at] = 0;
  GA_time = -1; // So that "birth_time"s are set to -1, so that on first pass (GA_time==0) organisms are not discounted as newborns
  for (at=0;at<limits__start_num_individuals/*world_size*/;at++)
  { Genotype *genotype = new Genotype;
    genotype->last_gene = genotype->first_gene = new Gene [1]; //initial individuals
    *(genotype->first_gene) = 0;                               // have just 1 gene
    world[at] = new Organism(genotype,((at%world_length)<<6)+32,
                                      ((at/world_length)<<6)+32);
    Genotype *SHADOWgenotype = new Genotype;
    SHADOWgenotype->last_gene = SHADOWgenotype->first_gene = new Gene [1]; //initial individuals
    *(SHADOWgenotype->first_gene) = 0;                                     // have just 1 gene
#ifndef only_generate_rules_for_shadows_when_needed
    SHADOWworld[at] = new Organism(SHADOWgenotype,-1,-1); //SHADOW organisms don't have a position; -1,-1 used by Organism::~Organism to denote SHADOW
#else   //only_generate_rules_for_shadows_when_needed
    SHADOWgenotypes[at] = SHADOWgenotype;
    SHADOWstats__ages[at] = 0;
    SHADOWbirth_times[at] = GA_time;
#endif  //only_generate_rules_for_shadows_when_needed
  }
  stats__num_individuals = limits__start_num_individuals;
  SHADOWstats__num_individuals = limits__start_num_individuals;
  for (at=limits__start_num_individuals;at<world_size;at++)
    world[at] = 0;
#ifndef protect_one
#else   //protect_one
  protected_one = world[0];
#endif  //protect_one
  reset_not_processed_arrays();
  // GA_time now== 0;
  stats__num_reproductions = 0;
  initialise_action_characters();
}

void ga_main_loop()
{ if (num_not_processed == 0) return; // if no individuals left do nothing
  int r = random(num_not_processed);
  Organism *individual = world[not_processed_positions[r]];
  // if indivdual has not been killed (inc. replaced) in this cycle (yet) ...
  // NB: Not enough to check 'if (individual == not_processed_organisms[r])', as
  //     the individual in this world-position at the start of this time-step could
  //     have been killed and later replaced by one with (by chance) the same memory
  //     address.  This seems to happens a few times every 100 time steps (on average).
  if ((individual == not_processed_organisms[r]) && (individual->birth_time != GA_time))
  { if (  (limits__die_at_age != -1)
        &&(individual->stats__age == limits__die_at_age)
        &&(not_processed_positions[r] >= limits__never_kill_or_move_first_)
       )
    { individual->unshow_in_world_window();
      delete individual;
      world[not_processed_positions[r]] = 0;
    }
    else
    { individual->copy_net_inputs_from_organism_in_front();
#ifndef morphogenesis_during_life
#else   //morphogenesis_during_life
      individual->morphogenesis_step();
#endif  //morphogenesis_during_life
      individual->update_outputs();
      individual->actions();
      (individual->stats__age)++; //statistic
    }
  }
  num_not_processed--;
  if (num_not_processed) // if there are still unprocessed indiviuals
  { not_processed_positions[r] = not_processed_positions[num_not_processed];
    not_processed_organisms[r] = not_processed_organisms[num_not_processed];
  }
  else
  { for (int SHADOWat=0;SHADOWat<SHADOWstats__num_individuals;SHADOWat++)
#ifndef only_generate_rules_for_shadows_when_needed
      if (SHADOWworld[SHADOWat]->birth_time != GA_time)
      { 
#ifndef compute_SHADOW_used_component_Complexities
#else   //compute_SHADOW_used_component_Complexities
#ifndef morphogenesis_during_life
#else   //morphogenesis_during_life
        SHADOWworld[SHADOWat]->morphogenesis_step();
#endif  //morphogenesis_during_life
        (SHADOWworld[SHADOWat]->stats__age)++;
#endif  //compute_SHADOW_used_component_Complexities
      }
#else   //only_generate_rules_for_shadows_when_needed
      if (SHADOWbirth_times[SHADOWat] != GA_time)
        SHADOWstats__ages[SHADOWat]++;
#endif  //only_generate_rules_for_shadows_when_needed
    reset_not_processed_arrays(); // new cycle
  }
}

void ga_terminate()
{ for (int d=0;d<world_size;d++) if (world[d]) delete world[d];
  // Organism destructor also deletes a random member of SHADOWworld / SHADOWgenotypes.
  // Hence no need for a loop such as this below for SHADOWworld / SHADOWgenotypes.
  delete [] world;
  //new when added option "memory_defragmentation_by_rereading_on_autofile":
  delete [] not_processed_positions;
  delete [] not_processed_organisms;
#ifndef only_generate_rules_for_shadows_when_needed
  delete [] SHADOWworld;
#else   //only_generate_rules_for_shadows_when_needed
  delete [] SHADOWgenotypes;
  delete [] SHADOWstats__ages;
  delete [] SHADOWbirth_times;
#endif  //only_generate_rules_for_shadows_when_needed
  delete [] reproduce_character.first_gene;
  delete [] kill_character_start.first_gene;
  delete [] turn_anti_clockwise_character_start.first_gene;
  delete [] turn_clockwise_character_start.first_gene;
  delete [] move_forward_character_start.first_gene;
}

void Fgets(char *s,int size,FILE *file)
{ char *ret=fgets(s,size,file);
  if (ret != s) {fprintf(stderr,"Error: Unexpected return value from fgets\n"); fflush(stderr);}
}

void Fscanf(FILE *file,const char *format) {int *p1=0; Fscanf(file,format,p1);}
