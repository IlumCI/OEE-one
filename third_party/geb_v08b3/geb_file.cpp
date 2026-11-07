/* Copyright (c) 2008 Alastair Channon
* 
* Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
* 
* The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

/* geb_file.cpp */
#include "geb_file.h"
#include "geb_core.h"
#include "geb_rs.h"
#include <stdio.h>
#include <string.h>

//defined (or not) in geb_core.h: only_generate_rules_for_shadows_when_needed
//#ifndef only_generate_rules_for_shadows_when_needed
//#warning NOT USING only_generate_rules_for_shadows_when_needed
//#else   //only_generate_rules_for_shadows_when_needed
//#endif  //only_generate_rules_for_shadows_when_needed

//#define dont_read_outputs //for development work
#ifndef dont_read_outputs
#else   //dont_read_outputs
#warning: Using dont_read_outputs
#endif  //dont_read_outputs

void geb_file__write_organism(FILE *file,Organism *organism)
{ fprintf(file,"%s %d\n","genotype->length()",organism->genotype->length());
  fprintf(file,"Genotype ");
  for (Gene *gene=organism->genotype->first_gene;
       gene<=organism->genotype->last_gene;gene++)
    fprintf(file,"%d",(int) *gene);
  fprintf(file,"\n");
  fprintf(file,"%s %d\n","at_x_64",organism->at_x_64);
  fprintf(file,"%s %d\n","at_y_64",organism->at_y_64);
  fprintf(file,"%s %d\n","direction_deg",organism->direction_deg);
  fprintf(file,"%s %d\n","stats__age",organism->stats__age);
  fprintf(file,"%s %d\n","birth_time",organism->birth_time);
  fprintf(file,"%s %d\n","pen_colour",organism->pen_colour);
  fprintf(file,"%s %d\n","num_morphogenesis_steps_so_far",
          max_num_morphogenesis_steps-organism->num_morphogenesis_steps_left);
  //note: "num_morphogenesis_steps_so_far" used so that will work with
  //       morphogenesis either during or at start of life,
  //       and with a changed max_num_morphogenesis_steps
  fprintf(file,"Outputs");
  for (Node *node=organism->nodes->next;node;node=node->next) //not dummy
    fprintf(file," %d %d",node->inhibitory_output,node->excitatory_output);
  fprintf(file,"\n");
}

int geb_file__write(const char *file_name)
{ FILE *file = fopen(file_name,"w");
  if (!file)
  { fprintf(stderr,"%s%s%s","Failed to open file ",file_name," for writing snapshot\n"); fflush(stderr);
    return(1);
  }
  int n;
  for (n=0;n<num_not_processed;n++)                                              // filter not_processed_ arrays
    if (world[not_processed_positions[n]] != not_processed_organisms[n])         // filter not_processed_ arrays
    { num_not_processed--;                                                       // filter not_processed_ arrays
      if (num_not_processed)                                                     // filter not_processed_ arrays
      { not_processed_positions[n] = not_processed_positions[num_not_processed]; // filter not_processed_ arrays
        not_processed_organisms[n] = not_processed_organisms[num_not_processed]; // filter not_processed_ arrays
      }                                                                          // filter not_processed_ arrays
      else {reset_not_processed_arrays(); n=num_not_processed;}                  // filter not_processed_ arrays
    }                                                                            // filter not_processed_ arrays
  fprintf(file,"%s %d\n","world_length",world_length);
  fprintf(file,"%s %ld\n","GA_time",GA_time);
  fprintf(file,"%s %d\n","stats__num_individuals",stats__num_individuals);
  fprintf(file,"%s %ld\n","stats__num_reproductions",stats__num_reproductions);
  fprintf(file,"%s %d\n","num_not_processed",num_not_processed);
  fprintf(file,"%s%d]","not_processed_positions[0..",num_not_processed-1);
  for (n=0;n<num_not_processed;n++)
    fprintf(file," %d",not_processed_positions[n]);
  fprintf(file,"\n");
  fprintf(file,"%s ","protected_one->at");
  if (protected_one) fprintf(file,"%d\n",protected_one->at);
  else               fprintf(file,"%d\n",-1);
  int individual_num=0;
  for (int at=0;at<world_size;at++)
    if (world[at])
    { individual_num++;
      fprintf(file,"\nOrganism %d\n",individual_num);
      geb_file__write_organism(file,world[at]);
    }
  fprintf(file,"\n"); fprintf(file,"%s %d\n","SHADOWstats__num_individuals",SHADOWstats__num_individuals);
  for (int SHADOWat=0;SHADOWat<SHADOWstats__num_individuals;SHADOWat++)
  { fprintf(file,"\nSHADOWOrganism %d\n",1+SHADOWat);
#ifndef only_generate_rules_for_shadows_when_needed
    geb_file__write_organism(file,SHADOWworld[SHADOWat]);
#else   //only_generate_rules_for_shadows_when_needed
    Organism* tempSHADOWorganism = new Organism(SHADOWgenotypes[SHADOWat],-1,-1); //SHADOW organisms don't have a position; -1,-1 used by Organism::~Organism to denote SHADOW
    tempSHADOWorganism->stats__age = SHADOWstats__ages[SHADOWat];
    tempSHADOWorganism->birth_time = SHADOWbirth_times[SHADOWat];
    geb_file__write_organism(file,tempSHADOWorganism);
    tempSHADOWorganism->genotype = new Genotype; tempSHADOWorganism->genotype->last_gene = tempSHADOWorganism->genotype->first_gene = new Gene [1]; // dummy genotype for Ann::~Ann
    delete tempSHADOWorganism;
#endif  //only_generate_rules_for_shadows_when_needed
  }
  if (rulestat_on) geb_rulestat__write_snapshot(file);
  int err = rnd_file__save(file); //last thing to write so take err from here
  fclose(file);
  ////copy to stdout:
  //file = fopen(file_name,"r");
  //if (!file)
  //{ fprintf(stderr,"%s%s%s","Failed to open file ",file_name," for read to stdout\n"); fflush(stderr);
  //  return(2);
  //}
  //char c;
  //while ((c=fgetc(file))!=EOF) fputc(c,stdout); fflush(stdout);
  //fclose(file);
  return err;
  //return(0);
}

void geb_file__read_and_initialise_organism(FILE *file,int SHADOWat=-999) //default (-999) indicates a normal organism (not a SHADOW organism)
{ int genotype_length;
  Fscanf(file,"%*s %d",&genotype_length);
  Fscanf(file,"%*s ");
  Genotype *new_genotype = new Genotype;
  new_genotype->first_gene = new Gene [genotype_length];
  new_genotype->last_gene  = new_genotype->first_gene + genotype_length -1;
  for (Gene *gene=new_genotype->first_gene;
       gene <= new_genotype->last_gene;gene++)
    *gene = fgetc(file) - '0';
  int at_x_64,at_y_64;
  Fscanf(file,"%*s %d",&at_x_64);
  Fscanf(file,"%*s %d",&at_y_64);
  //old (pre-shadows): Organism *organism = world[(at_x_64>>6)+(at_y_64>>6)*world_length]
  //old (pre-shadows):                    = new Organism(new_genotype,at_x_64,at_y_64);
  Organism *organism   = new Organism(new_genotype,at_x_64,at_y_64);
  Fscanf(file,"%*s %d",&(organism->direction_deg));
  Fscanf(file,"%*s %d",&(organism->stats__age));
  Fscanf(file,"%*s %d",&(organism->birth_time));
  Fscanf(file,"%*s %d",&(organism->pen_colour));
  int num_morphogenesis_steps_so_far;
  Fscanf(file,"%*s %d",&num_morphogenesis_steps_so_far);
  for (int n=0;n<num_morphogenesis_steps_so_far;n++) organism->morphogenesis_step();
  Fscanf(file,"%*s");
#ifndef dont_read_outputs
  for (Node *node=organism->nodes->next;node;node=node->next) //not dummy
  { Fscanf(file," %d %d",&(node->inhibitory_output),&(node->excitatory_output));
    if (node->net_output_node_ptr)
      node->net_output_node_ptr->io_value = node->excitatory_output;
  }
#else   //dont_read_outputs
  char *dummy_string = new char[1000];
  Fgets(dummy_string,1000,file);
  delete [] dummy_string;
  for (Node *node=organism->nodes->next;node;node=node->next) //not dummy
    node->inhibitory_output = node->excitatory_output = 0;
#endif  //dont_read_outputs
  if (SHADOWat==-999) world[(at_x_64>>6)+(at_y_64>>6)*world_length] = organism; //if not a SHADOW organism
#ifndef only_generate_rules_for_shadows_when_needed
  else                SHADOWworld[SHADOWat]                         = organism; //else   a SHADOW organism
#else   //only_generate_rules_for_shadows_when_needed
  else
  {                   SHADOWgenotypes[SHADOWat]                     = organism->genotype;
                      SHADOWstats__ages[SHADOWat]                   = organism->stats__age;
                      SHADOWbirth_times[SHADOWat]                   = organism->birth_time;
    organism->genotype = new Genotype; organism->genotype->last_gene = organism->genotype->first_gene = new Gene [1]; // dummy genotype for Ann::~Ann
    delete organism;
  }
#endif  //only_generate_rules_for_shadows_when_needed
}


int geb_file__ga_initialise(const char *file_name,int read_rnd_state)
{ FILE *file;
  if (!strcmp(file_name,"-")) file = stdin;
  else
  { file = fopen(file_name,"r");
    if (!file)
    { fprintf(stderr,"%s%s%s","Failed to open file ",file_name," for reading snapshot\n"); fflush(stderr);
      return(1);
    }
  }
  if (file == stdin) fprintf(stdout,"%s","Reading standard input ");
  else fprintf(stdout,"%s%s%s","Reading file ",file_name," ");
  fflush(stderr);
  Fscanf(file,"%*s %d",&world_length);
  limits__start_num_individuals = 0; //not needed but sensible
  ga_initialise();
  for (int d=0;d<world_size;d++)
    if (world[d])
    { delete world[d];
      world[d] = 0;
    }
#ifndef only_generate_rules_for_shadows_when_needed
  for (int SHADOWd=0;SHADOWd<SHADOWstats__num_individuals;SHADOWd++) delete SHADOWworld[SHADOWd];
#else   //only_generate_rules_for_shadows_when_needed
  for (int SHADOWd=0;SHADOWd<SHADOWstats__num_individuals;SHADOWd++) delete SHADOWgenotypes[SHADOWd];
#endif  //only_generate_rules_for_shadows_when_needed
  Fscanf(file,"%*s %ld",&GA_time);
  Fscanf(file,"%*s %d",&stats__num_individuals);
  Fscanf(file,"%*s %ld",&stats__num_reproductions);
  Fscanf(file,"%*s %d",&num_not_processed);
  Fscanf(file,"%*s ");
  int n;
  for (n=0;n<num_not_processed;n++)
    Fscanf(file,"%d",&(not_processed_positions[n]));
  int protected_one_at;
  Fscanf(file,"%*s %d",&protected_one_at);
  int verify_individual_num;
  for (int individual_num=1;individual_num<=stats__num_individuals;individual_num++)
  { Fscanf(file,"%*s %d",&verify_individual_num);
    if (verify_individual_num != individual_num)
    { fclose(file);
      if (file == stdin) fprintf(stderr,"%s%d%s","\nError reading individual number ",individual_num," from standard input \n");
      else fprintf(stderr,"%s%d%s%s%s","\nError reading individual number ",individual_num," from file ",file_name,"\n");
      fprintf(stderr,"%s","Possibly caused by incorrect number of output neurons due to a change in\n");
      fprintf(stderr,"%s","morphogenesis coding - try #define dont_read_outputs in source file geb_file.cpp\n");
      fflush(stderr);
      return(2);
    }
    for (n=((individual_num-1)*10)/stats__num_individuals;n<((individual_num  )*10)/stats__num_individuals;n++)
      {fprintf(stdout,"."); fflush(stdout);}
    geb_file__read_and_initialise_organism(file);
  }
  /*fprintf(stdout,"\n");*/ fflush(stdout);
  //fclose(file);
  for (n=0;n<num_not_processed;n++)
    not_processed_organisms[n] = world[not_processed_positions[n]];
  if (protected_one_at != -1) protected_one = world[protected_one_at];
  Fscanf(file,"%*s %d",&SHADOWstats__num_individuals);
  int SHADOWverify_individual_num;
  for (int SHADOWindividual_num=1;SHADOWindividual_num<=SHADOWstats__num_individuals;SHADOWindividual_num++)
  { Fscanf(file,"%*s %d",&SHADOWverify_individual_num);
    if (SHADOWverify_individual_num != SHADOWindividual_num)
    { fclose(file);
      if (file == stdin) fprintf(stderr,"%s%d%s","\nError reading SHADOWindividual number ",SHADOWindividual_num," from standard input \n");
      else fprintf(stderr,"%s%d%s%s%s","\nError reading SHADOWindividual number ",SHADOWindividual_num," from file ",file_name,"\n");
      fprintf(stderr,"%s","Possibly caused by incorrect number of output neurons due to a change in\n");
      fprintf(stderr,"%s","morphogenesis coding - try #define dont_read_outputs in source file geb_file.cpp\n");
      fflush(stderr);
      return(2);
    }
    for (int SHADOWn=((SHADOWindividual_num-1)*10)/SHADOWstats__num_individuals;SHADOWn<((SHADOWindividual_num  )*10)/SHADOWstats__num_individuals;SHADOWn++)
      {fprintf(stdout,"s"); fflush(stdout);}
    geb_file__read_and_initialise_organism(file,SHADOWindividual_num-1);
  }
  fprintf(stdout,"\n"); fflush(stdout);
  if (rulestat_on) {printf("Reading rulestat data ..."); fflush(stdout); geb_rulestat__read_snapshot_and_initialise(file); printf("... done\n"); fflush(stdout);}
  int err = 0;
  if (read_rnd_state)
  { err = rnd_file__load(file);
    if (!err) {printf("Random number state read from snapshot.\n"); fflush(stdout);}
    else      {fprintf(stderr,"ERROR: Failed to read random number state from snapshot - try specifying a seed.\n"); fflush(stderr);} // old snapshots may not have this state
  }
  fclose(file);
  //return(0);
  return err;
}
