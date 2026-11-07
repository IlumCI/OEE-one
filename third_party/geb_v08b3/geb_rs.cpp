/* Copyright (c) 2008 Alastair Channon
* 
* Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
* 
* The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

/* geb_rs.cpp */
#include "geb_rs.h"
#include "geb_core.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define  compute_complexities

//As of version 07b, one of reset_shadow_to_real_for_each_pass, dont_reset_shadow_to_real_for_each_pass should be defined on the compile line.  
#ifndef reset_shadow_to_real_for_each_pass
#ifndef dont_reset_shadow_to_real_for_each_pass
#warning ERROR: One of reset_shadow_to_real_for_each_pass, dont_reset_shadow_to_real_for_each_pass should be defined on the compile line.
#endif  //dont_reset_shadow_to_real_for_each_pass
#endif  //reset_shadow_to_real_for_each_pass
//old (leaving in or commenting out this line used to decide which method was used): #define reset_shadow_to_real_for_each_pass

// Note: This is a time-slice method: many components (both real and shadow) will come and go without ever being noticed

const char *rulestat_file_name = "rulestat";
FILE *rulestat_file;
const char *SHADOWrulestat_file_name = "SHADOWrulestat";
FILE *SHADOWrulestat_file;

const char *rulestat_verifier_file_name = "rulestat.verifier";
FILE *rulestat_verifier_file;
const char *SHADOWrulestat_verifier_file_name = "SHADOWrulestat.verifier";
FILE *SHADOWrulestat_verifier_file;

//defined (or not) in geb_core.h: only_generate_rules_for_shadows_when_needed
//#ifndef only_generate_rules_for_shadows_when_needed
//#warning NOT USING only_generate_rules_for_shadows_when_needed
//#else   //only_generate_rules_for_shadows_when_needed
//#endif  //only_generate_rules_for_shadows_when_needed

int rulestat_files_already_open = 0; //new when added option "memory_defragmentation_by_rereading_on_autofile"

void geb_rulestat_initialise_just_rulestat_file()
{ if (rulestat_files_already_open) return;
  if (!(rulestat_file = fopen(rulestat_file_name,"w")))      // note: file closure is left to the closing / killing of the program
    { fprintf(stderr,"%s%s%s","Failed to open file ",rulestat_file_name," for writing rulestat output\n"); fflush(stderr); return; }
  fprintf(rulestat_file,"Geb runtime production rule abundance statistics\n");
  fprintf(rulestat_file,"Output every %d timesteps\n",rulestat_every);
  fprintf(rulestat_file,"For processing by actiview\n");
  fprintf(rulestat_file,"= DATA =\n");
  fprintf(rulestat_file,"component data format: population\n");
  if (!(SHADOWrulestat_file = fopen(SHADOWrulestat_file_name,"w")))      // note: file closure is left to the closing / killing of the program
    { fprintf(stderr,"%s%s%s","Failed to open file ",SHADOWrulestat_file_name," for writing SHADOWrulestat output\n"); fflush(stderr); return; }
  fprintf(SHADOWrulestat_file,"Geb SHADOWruntime production rule abundance statistics\n");
  fprintf(SHADOWrulestat_file,"Output every %d timesteps\n",rulestat_every);
  fprintf(SHADOWrulestat_file,"For processing by actiview\n");
  fprintf(SHADOWrulestat_file,"= DATA =\n");
  fprintf(SHADOWrulestat_file,"component data format: population\n");
  if (!(rulestat_verifier_file = fopen(rulestat_verifier_file_name,"w")))      // note: file closure is left to the closing / killing of the program
    { fprintf(stderr,"%s%s%s","Failed to open file ",rulestat_verifier_file_name," for writing rulestat verifier output\n"); fflush(stderr); return; }
  if (!(SHADOWrulestat_verifier_file = fopen(SHADOWrulestat_verifier_file_name,"w")))      // note: file closure is left to the closing / killing of the program
    { fprintf(stderr,"%s%s%s","Failed to open file ",SHADOWrulestat_verifier_file_name," for writing SHADOWrulestat verifier output\n"); fflush(stderr); return; }
  rulestat_files_already_open = 1;
}

// ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#ifndef reset_shadow_to_real_for_each_pass
//#warning NOT USING reset_shadow_to_real_for_each_pass

// Component corresponds to a production rule
class Component
{ public:    Component() {next=0;}                     // constructor
             Component *next;
             Gene_string gene_string;
             int abundance_of_component;
};

class Component_list
{ public:    Component_list() {first_component=0;}
             Component *first_component;
};

Component_list *components;
Component_list *SHADOWcomponents;
const int initial_max_component_length = 10;
const int max_component_length_increment_on_insufficiency = 10;
int max_component_length;
int SHADOWmax_component_length;

void geb_rulestat_initialise()
{ max_component_length = initial_max_component_length;
  SHADOWmax_component_length = initial_max_component_length;
  components = new Component_list [1+max_component_length];
  SHADOWcomponents = new Component_list [1+SHADOWmax_component_length];
  geb_rulestat_initialise_just_rulestat_file();
}

//new when added option "memory_defragmentation_by_rereading_on_autofile":
void geb_rulestat_terminate(int close_rulestat_files)
{ int length; for (length=0;length<=max_component_length;length++)
  { Component* component,* next_component;
    for (component=components[length].first_component;component;component=next_component)
    { next_component=component->next;
      delete [] component->gene_string.first_gene;
      delete component;
    }
  }
  delete [] components;
  int SHADOWlength; for (SHADOWlength=0;SHADOWlength<=SHADOWmax_component_length;SHADOWlength++)
  { Component* SHADOWcomponent,* SHADOWnext_component;
    for (SHADOWcomponent=SHADOWcomponents[SHADOWlength].first_component;SHADOWcomponent;SHADOWcomponent=SHADOWnext_component)
    { SHADOWnext_component=SHADOWcomponent->next;
      delete [] SHADOWcomponent->gene_string.first_gene;
      delete SHADOWcomponent;
    }
  }
  delete [] SHADOWcomponents;
  if (close_rulestat_files)
  { fclose(rulestat_file);
    fclose(SHADOWrulestat_file);
    fclose(rulestat_verifier_file);
    fclose(SHADOWrulestat_verifier_file);
  }
}

void geb_rulestat()
{ for (int _component_length=0;_component_length<=max_component_length;_component_length++)
    for (Component *component=components[_component_length].first_component;component;component=component->next)
      (component->abundance_of_component)=0;
#ifdef   compute_complexities
  int max_component_count=0, max_sum_component_lengths=0,
      sum_component_count=0, sum_sum_component_lengths=0,
      num_individuals=0;
  int max_used_component_count=0, max_sum_used_component_lengths=0,
      sum_used_component_count=0, sum_sum_used_component_lengths=0;
#endif //compute_complexities
  for (int individual_at=0;individual_at<world_size;individual_at++)                                 // }For each individual
  { Organism *individual = world[individual_at];                                                     // }
    if (individual)                                                                                  // }
#ifdef   compute_complexities
    { num_individuals++;
      int component_count=0,sum_component_lengths=0;
      int used_component_count=0,sum_used_component_lengths=0;
#endif //compute_complexities
      for (Production_rule *rule=individual->first_rule;rule;rule=rule->next)                        //    For each of individual's production rules
      { int component_length = rule->gene_string.length();                                           //      Get component's length (component=production rule)
#ifdef   compute_complexities
        component_count++; sum_component_lengths += component_length;
        if (rule->used) {used_component_count++; sum_used_component_lengths += component_length;}
#endif //compute_complexities
        if (component_length > max_component_length)                                                 //     }Ensure that component array is sufficiently long
        { int old_max_component_length = max_component_length;                                       //     }
          while (component_length > max_component_length)                                            //     }
            max_component_length += max_component_length_increment_on_insufficiency;                 //     }
          Component_list *components__new = new Component_list [1+max_component_length];             //     }
          for (int n=0;n<=old_max_component_length;n++)                                              //     }
            components__new[n].first_component = components[n].first_component;                      //     }
          delete [] components;                                                                      //     }
          components = components__new;                                                              //     }
        }                                                                                            //     }
        Component **_compare_rule=&(components[component_length].first_component);                   //     }Make sure that component is in component array
        for (;(*_compare_rule) && !((*_compare_rule)->gene_string.is_equal_to(rule->gene_string));   //     }
             _compare_rule=&((*_compare_rule)->next));                                               //     }
        if (*_compare_rule) ((*_compare_rule)->abundance_of_component)++;                            //     } }
        else                                                                                         //     }  \_ and update its abundance_of_component
        { Component *new_component = (*_compare_rule) = new Component;                               //     }  /    (only used within component array)
          (new_component->abundance_of_component)=1;                                                 //     } }
          new_component->gene_string.first_gene = new Gene [component_length];                       //     }
          Gene *rule_gene,*new_component_gene;                                                       //     }
          for (rule_gene=rule->gene_string.first_gene,                                               //     }
                 new_component_gene=new_component->gene_string.first_gene;                           //     }
               rule_gene <= rule->gene_string.last_gene;                                             //     }
               rule_gene++,new_component_gene++)                                                     //     }
            *new_component_gene = *rule_gene;                                                        //     }
          new_component->gene_string.last_gene = new_component_gene - 1;                             //     }
        }                                                                                            //     }
      }                                                                                              //
#ifdef   compute_complexities
      if (component_count>max_component_count) max_component_count=component_count;
      sum_component_count += component_count;
      if (sum_component_lengths>max_sum_component_lengths) max_sum_component_lengths=sum_component_lengths;
      sum_sum_component_lengths += sum_component_lengths;
      //
      if (used_component_count>max_used_component_count) max_used_component_count=used_component_count;
      sum_used_component_count += used_component_count;
      if (sum_used_component_lengths>max_sum_used_component_lengths) max_sum_used_component_lengths=sum_used_component_lengths;
      sum_sum_used_component_lengths += sum_used_component_lengths;
    }
#endif //compute_complexities
  }                                                                                                  //
#ifdef   compute_complexities
  double mean_component_count = ((double)sum_component_count) / ((double)num_individuals);
  double mean_sum_component_lengths = ((double)sum_sum_component_lengths) / ((double)num_individuals);
  printf("Complexities at time=%ld: max_component_count=%d mean_component_count=%lf max_sum_component_lengths=%d mean_sum_component_lengths=%lf\n",
          GA_time,max_component_count,mean_component_count,max_sum_component_lengths,mean_sum_component_lengths);
  double mean_used_component_count = ((double)sum_used_component_count) / ((double)num_individuals);
  double mean_sum_used_component_lengths = ((double)sum_sum_used_component_lengths) / ((double)num_individuals);
  printf("Complexities of used components at time=%ld: max_used_component_count=%d mean_used_component_count=%lf max_sum_used_component_lengths=%d mean_sum_used_component_lengths=%lf\n",
          GA_time,max_used_component_count,mean_used_component_count,max_sum_used_component_lengths,mean_sum_used_component_lengths);
#endif //compute_complexities
  fprintf(rulestat_file,"%ld:",GA_time);                                                             //  Write timestep to rulestat file
  fprintf(rulestat_verifier_file,"%ld:",GA_time);                                                    //  Write timestep to rulestat verifier file
  long int verifier0=0,verifier1=0,verifier2=0,verifier3=0;                                          //  Zero verifiers
  for (int component_length=0;component_length<=max_component_length;component_length++)             // }For all component lists
  { int rule_num=1;                                                                                  //   }For all currently existing components
    for (Component *component=components[component_length].first_component  ;                        //   }
       component;rule_num++,component=component->next)                                               //   }
      if (component->abundance_of_component)                                                         //   }
      { fprintf(rulestat_file," %d.%d",component_length,rule_num);                                   //     }Write component's name and abundance_of_component
        fprintf(rulestat_file," %d;",component->abundance_of_component);                             //     }  to rulestat file
        verifier0+=component_length; verifier1+=rule_num;                                            //     }Update verifiers
        verifier2++; verifier3+=component->abundance_of_component;                                   //     }
      }                                                                                              //
  }                                                                                                  //
  fprintf(rulestat_file,"\n"); fflush(rulestat_file);                                                //  Write newline to rulestat file and flush
  fprintf(rulestat_verifier_file," %ld %ld %ld %ld",verifier0,verifier1,verifier2,verifier3);        //  Write verifiers to rulestat verifier file
  fprintf(rulestat_verifier_file,"\n"); fflush(rulestat_verifier_file);                              //  Write newline to rulestat verifier file and flush
  for (int _SHADOWcomponent_length=0;_SHADOWcomponent_length<=SHADOWmax_component_length;_SHADOWcomponent_length++)
    for (Component *SHADOWcomponent=SHADOWcomponents[_SHADOWcomponent_length].first_component;SHADOWcomponent;SHADOWcomponent=SHADOWcomponent->next)
      (SHADOWcomponent->abundance_of_component)=0;
#ifdef   compute_complexities
  int SHADOWmax_component_count=0, SHADOWmax_sum_component_lengths=0,
      SHADOWsum_component_count=0, SHADOWsum_sum_component_lengths=0;
  int SHADOWmax_used_component_count=0, SHADOWmax_sum_used_component_lengths=0,
      SHADOWsum_used_component_count=0, SHADOWsum_sum_used_component_lengths=0;
#endif //compute_complexities
  for (int SHADOWindividual_at=0;SHADOWindividual_at<SHADOWstats__num_individuals;SHADOWindividual_at++)               // }For each individual
#ifndef only_generate_rules_for_shadows_when_needed
  { Organism *SHADOWindividual = SHADOWworld[SHADOWindividual_at];                                                     // }
    //if (SHADOWindividual)                                                                                            // }
#else   //only_generate_rules_for_shadows_when_needed
    //if (SHADOWindividual)                                                                                            // }
  { Organism *SHADOWindividual = new Organism(SHADOWgenotypes[SHADOWindividual_at],-1,-1); //SHADOW organisms don't have a position; -1,-1 used by Organism::~Organism to denote SHADOW
    //stats__age not used here: SHADOWindividual->stats__age = SHADOWstats__ages[SHADOWat];
    //birth_time not used here: SHADOWindividual->birth_time = SHADOWbirth_times[SHADOWat];
#endif  //only_generate_rules_for_shadows_when_needed
#ifdef   compute_complexities
    { int SHADOWcomponent_count=0,SHADOWsum_component_lengths=0;
      int SHADOWused_component_count=0,SHADOWsum_used_component_lengths=0;
#endif //compute_complexities
      for (Production_rule *SHADOWrule=SHADOWindividual->first_rule;SHADOWrule;SHADOWrule=SHADOWrule->next)            //    For each of individual's production rules
      { int SHADOWcomponent_length = SHADOWrule->gene_string.length();                                                 //      Get component's length (component=production rule)
#ifdef   compute_complexities
        SHADOWcomponent_count++; SHADOWsum_component_lengths += SHADOWcomponent_length;
        if (SHADOWrule->used) {SHADOWused_component_count++; SHADOWsum_used_component_lengths += SHADOWcomponent_length;}
#endif //compute_complexities
        if (SHADOWcomponent_length > SHADOWmax_component_length)                                                       //     }Ensure that component array is sufficiently long
        { int SHADOWold_max_component_length = SHADOWmax_component_length;                                             //     }
          while (SHADOWcomponent_length > SHADOWmax_component_length)                                                  //     }
            SHADOWmax_component_length += max_component_length_increment_on_insufficiency;                             //     }
          Component_list *SHADOWcomponents__new = new Component_list [1+SHADOWmax_component_length];                   //     }
          for (int SHADOWn=0;SHADOWn<=SHADOWold_max_component_length;SHADOWn++)                                        //     }
            SHADOWcomponents__new[SHADOWn].first_component = SHADOWcomponents[SHADOWn].first_component;                //     }
          delete [] SHADOWcomponents;                                                                                  //     }
          SHADOWcomponents = SHADOWcomponents__new;                                                                    //     }
        }                                                                                                              //     }
        Component **SHADOW_compare_rule=&(SHADOWcomponents[SHADOWcomponent_length].first_component);                   //     }Make sure that component is in component array
        for (;(*SHADOW_compare_rule) && !((*SHADOW_compare_rule)->gene_string.is_equal_to(SHADOWrule->gene_string));   //     }
             SHADOW_compare_rule=&((*SHADOW_compare_rule)->next));                                                     //     }
        if (*SHADOW_compare_rule) ((*SHADOW_compare_rule)->abundance_of_component)++;                                  //     } }
        else                                                                                                           //     }  \_ and update its abundance_of_component
        { Component *SHADOWnew_component = (*SHADOW_compare_rule) = new Component;                                     //     }  /    (only used within component array)
          (SHADOWnew_component->abundance_of_component)=1;                                                             //     } }
          SHADOWnew_component->gene_string.first_gene = new Gene [SHADOWcomponent_length];                             //     }
          Gene *SHADOWrule_gene,*SHADOWnew_component_gene;                                                             //     }
          for (SHADOWrule_gene=SHADOWrule->gene_string.first_gene,                                                     //     }
                 SHADOWnew_component_gene=SHADOWnew_component->gene_string.first_gene;                                 //     }
               SHADOWrule_gene <= SHADOWrule->gene_string.last_gene;                                                   //     }
               SHADOWrule_gene++,SHADOWnew_component_gene++)                                                           //     }
            *SHADOWnew_component_gene = *SHADOWrule_gene;                                                              //     }
          SHADOWnew_component->gene_string.last_gene = SHADOWnew_component_gene - 1;                                   //     }
        }                                                                                                              //     }
      }                                                                                                                //
#ifndef only_generate_rules_for_shadows_when_needed
#else   //only_generate_rules_for_shadows_when_needed
    SHADOWindividual->genotype = new Genotype; SHADOWindividual->genotype->last_gene = SHADOWindividual->genotype->first_gene = new Gene [1]; // dummy genotype for Ann::~Ann
    delete SHADOWindividual;
#endif  //only_generate_rules_for_shadows_when_needed
#ifdef   compute_complexities
      if (SHADOWcomponent_count>SHADOWmax_component_count) SHADOWmax_component_count=SHADOWcomponent_count;
      SHADOWsum_component_count += SHADOWcomponent_count;
      if (SHADOWsum_component_lengths>SHADOWmax_sum_component_lengths) SHADOWmax_sum_component_lengths=SHADOWsum_component_lengths;
      SHADOWsum_sum_component_lengths += SHADOWsum_component_lengths;
      //
      if (SHADOWused_component_count>SHADOWmax_used_component_count) SHADOWmax_used_component_count=SHADOWused_component_count;
      SHADOWsum_used_component_count += SHADOWused_component_count;
      if (SHADOWsum_used_component_lengths>SHADOWmax_sum_used_component_lengths) SHADOWmax_sum_used_component_lengths=SHADOWsum_used_component_lengths;
      SHADOWsum_sum_used_component_lengths += SHADOWsum_used_component_lengths;
    }
#endif //compute_complexities
  }                                                                                                                    //
#ifdef   compute_complexities
  double SHADOWmean_component_count = ((double)SHADOWsum_component_count) / ((double)SHADOWstats__num_individuals);
  double SHADOWmean_sum_component_lengths = ((double)SHADOWsum_sum_component_lengths) / ((double)SHADOWstats__num_individuals);
  printf("Complexities at time=%ld: SHADOWmax_component_count=%d SHADOWmean_component_count=%lf SHADOWmax_sum_component_lengths=%d SHADOWmean_sum_component_lengths=%lf\n",
          GA_time,SHADOWmax_component_count,SHADOWmean_component_count,SHADOWmax_sum_component_lengths,SHADOWmean_sum_component_lengths);
  double SHADOWmean_used_component_count = ((double)SHADOWsum_used_component_count) / ((double)SHADOWstats__num_individuals);
  double SHADOWmean_sum_used_component_lengths = ((double)SHADOWsum_sum_used_component_lengths) / ((double)SHADOWstats__num_individuals);
  printf("Complexities of used components at time=%ld: SHADOWmax_used_component_count=%d SHADOWmean_used_component_count=%lf SHADOWmax_sum_used_component_lengths=%d SHADOWmean_sum_used_component_lengths=%lf\n",
          GA_time,SHADOWmax_used_component_count,SHADOWmean_used_component_count,SHADOWmax_sum_used_component_lengths,SHADOWmean_sum_used_component_lengths);
#endif //compute_complexities
  fprintf(SHADOWrulestat_file,"%ld:",GA_time);                                                                         //  Write timestep to rulestat file
  fprintf(SHADOWrulestat_verifier_file,"%ld:",GA_time);                                                                //  Write timestep to rulestat verifier file
  long int SHADOWverifier0=0,SHADOWverifier1=0,SHADOWverifier2=0,SHADOWverifier3=0;                                    //  Zero verifiers
  for (int SHADOWcomponent_length=0;SHADOWcomponent_length<=SHADOWmax_component_length;SHADOWcomponent_length++)       // }For all component lists
  { int SHADOWrule_num=1;                                                                                              //   }For all currently existing components
    for (Component *SHADOWcomponent=SHADOWcomponents[SHADOWcomponent_length].first_component;                          //   }
       SHADOWcomponent;SHADOWrule_num++,SHADOWcomponent=SHADOWcomponent->next)                                         //   }
      if (SHADOWcomponent->abundance_of_component)                                                                     //   }
      { fprintf(SHADOWrulestat_file," %d.%d",SHADOWcomponent_length,SHADOWrule_num);                                   //     }Write component's name and abundance_of_component
        fprintf(SHADOWrulestat_file," %d;",SHADOWcomponent->abundance_of_component);                                   //     }  to rulestat file
        SHADOWverifier0+=SHADOWcomponent_length; SHADOWverifier1+=SHADOWrule_num;                                      //     }Update verifiers
        SHADOWverifier2++; SHADOWverifier3+=SHADOWcomponent->abundance_of_component;                                   //     }
      }                                                                                                                //
  }                                                                                                                    //
  fprintf(SHADOWrulestat_file,"\n"); fflush(SHADOWrulestat_file);                                                      //  Write newline to rulestat file and flush
  fprintf(SHADOWrulestat_verifier_file," %ld %ld %ld %ld",SHADOWverifier0,SHADOWverifier1,SHADOWverifier2,SHADOWverifier3); //  Write verifiers to rulestat verifier file
  fprintf(SHADOWrulestat_verifier_file,"\n"); fflush(SHADOWrulestat_verifier_file);                                         //  Write newline to rulestat verifier file and flush
}

void geb_rulestat__write_snapshot(FILE *file)
{ fprintf(file,"\nRulestat snapshot:\n");                                                            //    Write newline
  fprintf(file,"%s %d\n","max_component_length",max_component_length);                               //    Write max_component_length
  for (int component_length=0;component_length<=max_component_length;component_length++)             //   }For all components
    if (components[component_length].first_component)                                                //   }
    { int rule_num=1;                                                                                //   }
      for (Component *component=components[component_length].first_component;                        //   }
         component;rule_num++,component=component->next)                                             //   }
      {  fprintf(file,"%d %d ",component_length,rule_num);                                           //      Write component's name
         for (Gene *gene=component->gene_string.first_gene;                                          //     }Write genes of rule
              gene<=component->gene_string.last_gene;gene++)                                         //     }
           fprintf(file,"%d",(int) *gene);                                                           //     }
         fprintf(file,"\n");                                                                         //      Write newline
      }
    }
  fprintf(file,"\nSHADOWRulestat snapshot:\n");                                                                  //    Write newline
  fprintf(file,"%s %d\n","SHADOWmax_component_length",SHADOWmax_component_length);                               //    Write max_component_length
  for (int SHADOWcomponent_length=0;SHADOWcomponent_length<=SHADOWmax_component_length;SHADOWcomponent_length++) //   }For all components
    if (SHADOWcomponents[SHADOWcomponent_length].first_component)                                                //   }
    { int SHADOWrule_num=1;                                                                                      //   }
      for (Component *SHADOWcomponent=SHADOWcomponents[SHADOWcomponent_length].first_component;                  //   }
         SHADOWcomponent;SHADOWrule_num++,SHADOWcomponent=SHADOWcomponent->next)                                 //   }
      {  fprintf(file,"%d %d ",SHADOWcomponent_length,SHADOWrule_num);                                           //      Write component's name
         for (Gene *SHADOWgene=SHADOWcomponent->gene_string.first_gene;                                          //     }Write genes of rule
              SHADOWgene<=SHADOWcomponent->gene_string.last_gene;SHADOWgene++)                                   //     }
           fprintf(file,"%d",(int) *SHADOWgene);                                                                 //     }
         fprintf(file,"\n");                                                                                     //      Write newline
      }
    }
  fprintf(file,"-999\n"); // new for random number state save&load: explicitly write an end-of-section marker here
}

void geb_rulestat__read_snapshot_and_initialise(FILE *file)
{ char *dummy_string = new char[1000];
  Fgets(dummy_string,1000,file); Fgets(dummy_string,1000,file); //get passed newlines
  Fgets(dummy_string,1000,file);
  if (strcmp(dummy_string,"Rulestat snapshot:\n"))
    {fprintf(stderr,"Error: Could not find start of rulestat snapshot\n"); fflush(stderr); exit(81);}
  delete [] dummy_string;
  Fscanf(file,"%*s %d",&max_component_length);
  components = new Component_list [1+max_component_length];
  Component *component;
  for (int component_length;component_length=-999,Fscanf(file,"%d",&component_length),component_length!=-999;)
  { if ((component_length<1) || (component_length>max_component_length))
      {fprintf(stderr,"Error: Bad component number in rulestat snapshot\n"); fflush(stderr); exit(82);}
    int number; Fscanf(file," %d ",&number);
    if (   ((number == 1) &&  components[component_length].first_component)
        || ((number != 1) && !components[component_length].first_component)
       )
      {fprintf(stderr,"Error: Bad component number sequence in rulestat snapshot\n"); fflush(stderr); exit(83);}
    if (number == 1) component = components[component_length].first_component = new Component;
    else             component = component->next = new Component;
    component->gene_string.first_gene = new Gene [component_length];
    int n; Gene *gene;
    for (n=1,gene=component->gene_string.first_gene;n<=component_length;n++,gene++) {*gene = fgetc(file) - '0';}
    component->gene_string.last_gene = gene - 1;
  }
  char *SHADOWdummy_string = new char[1000];
  //Fgets(SHADOWdummy_string,1000,file); Fgets(SHADOWdummy_string,1000,file); //get passed newlines
  Fgets(SHADOWdummy_string,1000,file);
  if (strcmp(SHADOWdummy_string,"SHADOWRulestat snapshot:\n"))
    {fprintf(stderr,"Error: Could not find start of SHADOWrulestat snapshot\n"); fflush(stderr); exit(84);}
  delete [] SHADOWdummy_string;
  Fscanf(file,"%*s %d",&SHADOWmax_component_length);
  SHADOWcomponents = new Component_list [1+SHADOWmax_component_length];
  Component *SHADOWcomponent;
  for (int SHADOWcomponent_length;SHADOWcomponent_length=-999,Fscanf(file,"%d",&SHADOWcomponent_length),SHADOWcomponent_length!=-999;)
  { if ((SHADOWcomponent_length<1) || (SHADOWcomponent_length>SHADOWmax_component_length))
      {fprintf(stderr,"Error: Bad component number in SHADOWrulestat snapshot\n"); fflush(stderr); exit(85);}
    int SHADOWnumber; Fscanf(file," %d ",&SHADOWnumber);
    if (   ((SHADOWnumber == 1) &&  SHADOWcomponents[SHADOWcomponent_length].first_component)
        || ((SHADOWnumber != 1) && !SHADOWcomponents[SHADOWcomponent_length].first_component)
       )
      {fprintf(stderr,"Error: Bad component number sequence in SHADOWrulestat snapshot\n"); fflush(stderr); exit(86);}
    if (SHADOWnumber == 1) SHADOWcomponent = SHADOWcomponents[SHADOWcomponent_length].first_component = new Component;
    else                   SHADOWcomponent = SHADOWcomponent->next = new Component;
    SHADOWcomponent->gene_string.first_gene = new Gene [SHADOWcomponent_length];
    int SHADOWn; Gene *SHADOWgene;
    for (SHADOWn=1,SHADOWgene=SHADOWcomponent->gene_string.first_gene;SHADOWn<=SHADOWcomponent_length;SHADOWn++,SHADOWgene++) {*SHADOWgene = fgetc(file) - '0';}
    SHADOWcomponent->gene_string.last_gene = SHADOWgene - 1;
  }
  geb_rulestat_initialise_just_rulestat_file();
}

// ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#else   //reset_shadow_to_real_for_each_pass
//#warning USING reset_shadow_to_real_for_each_pass

// Component corresponds to a production rule
class Component
{ public:    Component() {next=0;}                     // constructor
             Component *next;
             Gene_string gene_string;
             int abundance_of_component;
             int SHADOWabundance_of_component; //for reset_shadow_to_real_for_each_pass
};

class Component_list
{ public:    Component_list() {first_component=shadow_components_from=0;}
             Component *first_component;
             Component *shadow_components_from; // scheme will always be RealComponents,ShadowComponents; ShadowComponents stripped of at start of geb_rulestat()
};
// No                   components: first_component=0                                       ,shadow_components_from=0
// Only Real            components: first_component=AddressOfFirstRealComponent             ,shadow_components_from=0
// Only          Shadow components: first_component=AddressOfFirstShadowComponent **NOT 0** ,shadow_components_from=AddressOfFirstShadowComponent
// Both Real and Shadow components: first_component=AddressOfFirstRealComponent             ,shadow_components_from=AddressOfFirstShadowComponent

Component_list *components;
const int initial_max_component_length = 10;
const int max_component_length_increment_on_insufficiency = 10;
int max_component_length;

void geb_rulestat_initialise()
{ max_component_length = initial_max_component_length;
  components = new Component_list [1+max_component_length];
  geb_rulestat_initialise_just_rulestat_file();
}

//new when added option "memory_defragmentation_by_rereading_on_autofile":
void geb_rulestat_terminate(int close_rulestat_files)
{ int length; for (length=0;length<=max_component_length;length++)
  { Component* component,* next_component;
    for (component=components[length].first_component;component;component=next_component)
    { next_component=component->next;
      delete [] component->gene_string.first_gene;
      delete component;
    }
  }
  delete [] components;
  // not with reset_shadow_to_real_for_each_pass :  int SHADOWlength; for (SHADOWlength=0;SHADOWlength<=SHADOWmax_component_length;SHADOWlength++)
  // not with reset_shadow_to_real_for_each_pass :  { Component* SHADOWcomponent,* SHADOWnext_component;
  // not with reset_shadow_to_real_for_each_pass :    for (SHADOWcomponent=SHADOWcomponents[SHADOWlength].first_component;SHADOWcomponent;SHADOWcomponent=SHADOWnext_component)
  // not with reset_shadow_to_real_for_each_pass :    { SHADOWnext_component=SHADOWcomponent->next;
  // not with reset_shadow_to_real_for_each_pass :      delete [] SHADOWcomponent->gene_string.first_gene;
  // not with reset_shadow_to_real_for_each_pass :      delete SHADOWcomponent;
  // not with reset_shadow_to_real_for_each_pass :    }
  // not with reset_shadow_to_real_for_each_pass :  }
  // not with reset_shadow_to_real_for_each_pass :  delete [] SHADOWcomponents;
  if (close_rulestat_files)
  { fclose(rulestat_file);
    fclose(SHADOWrulestat_file);
    fclose(rulestat_verifier_file);
    fclose(SHADOWrulestat_verifier_file);
  }
}

void geb_rulestat()
{ //Strip off old ShadowComponents:
  for (int __component_length=0;__component_length<=max_component_length;__component_length++)
    if (components[__component_length].shadow_components_from)
    { if (components[__component_length].first_component == components[__component_length].shadow_components_from) components[__component_length].first_component = 0;
      else
      { Component* last_real_component=components[__component_length].first_component;
        for (;last_real_component->next!=components[__component_length].shadow_components_from;last_real_component=last_real_component->next);
        last_real_component->next=0;
      }
      Component* shadow_component_to_delete=components[__component_length].shadow_components_from;
      while (shadow_component_to_delete)
      { Component* delete_this=shadow_component_to_delete;
        shadow_component_to_delete=shadow_component_to_delete->next;
        delete [] delete_this->gene_string.first_gene; // gene_string.first_gene is not automatically deleted when Component is deleted - this plugs the memory leak
        delete delete_this;
      }
      components[__component_length].shadow_components_from = 0;
    }
  for (int _component_length=0;_component_length<=max_component_length;_component_length++)
    for (Component *component=components[_component_length].first_component;component;component=component->next)
      (component->abundance_of_component)=(component->SHADOWabundance_of_component)=0; // <-NB: SHADOWabundance_of_component zeroed
#ifdef   compute_complexities
  int max_component_count=0, max_sum_component_lengths=0,
      sum_component_count=0, sum_sum_component_lengths=0,
      num_individuals=0;
  int max_used_component_count=0, max_sum_used_component_lengths=0,
      sum_used_component_count=0, sum_sum_used_component_lengths=0;
#endif //compute_complexities
  for (int individual_at=0;individual_at<world_size;individual_at++)                                 // }For each individual
  { Organism *individual = world[individual_at];                                                     // }
    if (individual)                                                                                  // }
#ifdef   compute_complexities
    { num_individuals++;
      int component_count=0,sum_component_lengths=0;
      int used_component_count=0,sum_used_component_lengths=0;
#endif //compute_complexities
      for (Production_rule *rule=individual->first_rule;rule;rule=rule->next)                        //    For each of individual's production rules
      { int component_length = rule->gene_string.length();                                           //      Get component's length (component=production rule)
#ifdef   compute_complexities
        component_count++; sum_component_lengths += component_length;
        if (rule->used) {used_component_count++; sum_used_component_lengths += component_length;}
#endif //compute_complexities
        if (component_length > max_component_length)                                                 //     }Ensure that component array is sufficiently long
        { int old_max_component_length = max_component_length;                                       //     }
          while (component_length > max_component_length)                                            //     }
            max_component_length += max_component_length_increment_on_insufficiency;                 //     }
          Component_list *components__new = new Component_list [1+max_component_length];             //     }
          for (int n=0;n<=old_max_component_length;n++)                                              //     }
            components__new[n].first_component = components[n].first_component;                      //     }
          delete [] components;                                                                      //     }
          components = components__new;                                                              //     }
        }                                                                                            //     }
        Component **_compare_rule=&(components[component_length].first_component);                   //     }Make sure that component is in component array
        for (;(*_compare_rule) && !((*_compare_rule)->gene_string.is_equal_to(rule->gene_string));   //     }
             _compare_rule=&((*_compare_rule)->next));                                               //     }
        if (*_compare_rule) ((*_compare_rule)->abundance_of_component)++;                            //     } }
        else                                                                                         //     }  \_ and update its abundance_of_component
        { Component *new_component = (*_compare_rule) = new Component;                               //     }  /    (only used within component array)
          (new_component->abundance_of_component)=1;                                                 //     } }
          (new_component->SHADOWabundance_of_component)=0;                                           //     } }   <-NB: SHADOWabundance_of_component zeroed
          new_component->gene_string.first_gene = new Gene [component_length];                       //     }
          Gene *rule_gene,*new_component_gene;                                                       //     }
          for (rule_gene=rule->gene_string.first_gene,                                               //     }
                 new_component_gene=new_component->gene_string.first_gene;                           //     }
               rule_gene <= rule->gene_string.last_gene;                                             //     }
               rule_gene++,new_component_gene++)                                                     //     }
            *new_component_gene = *rule_gene;                                                        //     }
          new_component->gene_string.last_gene = new_component_gene - 1;                             //     }
        }                                                                                            //     }
      }                                                                                              //
#ifdef   compute_complexities
      if (component_count>max_component_count) max_component_count=component_count;
      sum_component_count += component_count;
      if (sum_component_lengths>max_sum_component_lengths) max_sum_component_lengths=sum_component_lengths;
      sum_sum_component_lengths += sum_component_lengths;
      //
      if (used_component_count>max_used_component_count) max_used_component_count=used_component_count;
      sum_used_component_count += used_component_count;
      if (sum_used_component_lengths>max_sum_used_component_lengths) max_sum_used_component_lengths=sum_used_component_lengths;
      sum_sum_used_component_lengths += sum_used_component_lengths;
    }
#endif //compute_complexities
  }                                                                                                  //
#ifdef   compute_complexities
  double mean_component_count = ((double)sum_component_count) / ((double)num_individuals);
  double mean_sum_component_lengths = ((double)sum_sum_component_lengths) / ((double)num_individuals);
  printf("Complexities at time=%ld: max_component_count=%d mean_component_count=%lf max_sum_component_lengths=%d mean_sum_component_lengths=%lf\n",
          GA_time,max_component_count,mean_component_count,max_sum_component_lengths,mean_sum_component_lengths);
  double mean_used_component_count = ((double)sum_used_component_count) / ((double)num_individuals);
  double mean_sum_used_component_lengths = ((double)sum_sum_used_component_lengths) / ((double)num_individuals);
  printf("Complexities of used components at time=%ld: max_used_component_count=%d mean_used_component_count=%lf max_sum_used_component_lengths=%d mean_sum_used_component_lengths=%lf\n",
          GA_time,max_used_component_count,mean_used_component_count,max_sum_used_component_lengths,mean_sum_used_component_lengths);
#endif //compute_complexities
  fprintf(rulestat_file,"%ld:",GA_time);                                                             //  Write timestep to rulestat file
  fprintf(rulestat_verifier_file,"%ld:",GA_time);                                                    //  Write timestep to rulestat verifier file
  long int verifier0=0,verifier1=0,verifier2=0,verifier3=0;                                          //  Zero verifiers
  for (int component_length=0;component_length<=max_component_length;component_length++)             // }For all component lists
  { int rule_num=1;                                                                                  //   }For all currently existing components
    for (Component *component=components[component_length].first_component;                          //   }
       component;rule_num++,component=component->next)                                               //   }
      if (component->abundance_of_component)                                                         //   }
      { fprintf(rulestat_file," %d.%d",component_length,rule_num);                                   //     }Write component's name and abundance_of_component
        fprintf(rulestat_file," %d;",component->abundance_of_component);                             //     }  to rulestat file
        verifier0+=component_length; verifier1+=rule_num;                                            //     }Update verifiers
        verifier2++; verifier3+=component->abundance_of_component;                                   //     }
      }                                                                                              //
  }                                                                                                  //
  fprintf(rulestat_file,"\n"); fflush(rulestat_file);                                                //  Write newline to rulestat file and flush
  fprintf(rulestat_verifier_file," %ld %ld %ld %ld",verifier0,verifier1,verifier2,verifier3);        //  Write verifiers to rulestat verifier file
  fprintf(rulestat_verifier_file,"\n"); fflush(rulestat_verifier_file);                              //  Write newline to rulestat verifier file and flush
  // NB: (SHADOWabundance_of_component)s have already been zeroed
#ifdef   compute_complexities
  int SHADOWmax_component_count=0, SHADOWmax_sum_component_lengths=0,
      SHADOWsum_component_count=0, SHADOWsum_sum_component_lengths=0;
  int SHADOWmax_used_component_count=0, SHADOWmax_sum_used_component_lengths=0,
      SHADOWsum_used_component_count=0, SHADOWsum_sum_used_component_lengths=0;
#endif //compute_complexities
  for (int SHADOWindividual_at=0;SHADOWindividual_at<SHADOWstats__num_individuals;SHADOWindividual_at++)         // }For each individual
#ifndef only_generate_rules_for_shadows_when_needed
  { Organism *SHADOWindividual = SHADOWworld[SHADOWindividual_at];                                               // }
    //if (SHADOWindividual)                                                                                      // }
#else   //only_generate_rules_for_shadows_when_needed
    //if (SHADOWindividual)                                                                                      // }
  { Organism *SHADOWindividual = new Organism(SHADOWgenotypes[SHADOWindividual_at],-1,-1); //SHADOW organisms don't have a position; -1,-1 used by Organism::~Organism to denote SHADOW
    //stats__age not used here: SHADOWindividual->stats__age = SHADOWstats__ages[SHADOWat];
    //birth_time not used here: SHADOWindividual->birth_time = SHADOWbirth_times[SHADOWat];
#endif  //only_generate_rules_for_shadows_when_needed
#ifdef   compute_complexities
    { int SHADOWcomponent_count=0,SHADOWsum_component_lengths=0;
      int SHADOWused_component_count=0,SHADOWsum_used_component_lengths=0;
#endif //compute_complexities
      for (Production_rule *SHADOWrule=SHADOWindividual->first_rule;SHADOWrule;SHADOWrule=SHADOWrule->next)      //    For each of individual's production rules
      { int SHADOWcomponent_length = SHADOWrule->gene_string.length();                                           //      Get component's length (component=production rule)
#ifdef   compute_complexities
        SHADOWcomponent_count++; SHADOWsum_component_lengths += SHADOWcomponent_length;
        if (SHADOWrule->used) {SHADOWused_component_count++; SHADOWsum_used_component_lengths += SHADOWcomponent_length;}
#endif //compute_complexities
        if (SHADOWcomponent_length > max_component_length)                                                       //     }Ensure that component array is sufficiently long
        { int old_max_component_length = max_component_length;                                                   //     }
          while (SHADOWcomponent_length > max_component_length)                                                  //     }
            max_component_length += max_component_length_increment_on_insufficiency;                             //     }
          Component_list *components__new = new Component_list [1+max_component_length];                         //     }
          for (int n=0;n<=old_max_component_length;n++)                                                          //     }
            components__new[n].first_component = components[n].first_component;                                  //     }
          delete [] components;                                                                                  //     }
          components = components__new;                                                                          //     }
        }                                                                                                        //     }
        Component **_compare_rule=&(components[SHADOWcomponent_length].first_component);                         //     }Make sure that component is in component array
        for (;(*_compare_rule) && !((*_compare_rule)->gene_string.is_equal_to(SHADOWrule->gene_string));         //     }
             _compare_rule=&((*_compare_rule)->next));                                                           //     }
        if (*_compare_rule) ((*_compare_rule)->SHADOWabundance_of_component)++;                                  //     } }
        else                                                                                                     //     }  \_ and update its SHADOWabundance_of_component
        { Component *SHADOWnew_component = (*_compare_rule) = new Component;                                     //     }  /    (only used within component array)
          (SHADOWnew_component->SHADOWabundance_of_component)=1;                                                 //     } }
          (SHADOWnew_component->abundance_of_component)=0;                                                       //     } }   <-NB: abundance_of_component zeroed
          SHADOWnew_component->gene_string.first_gene = new Gene [SHADOWcomponent_length];                       //     }
          Gene *SHADOWrule_gene,*SHADOWnew_component_gene;                                                       //     }
          for (SHADOWrule_gene=SHADOWrule->gene_string.first_gene,                                               //     }
                 SHADOWnew_component_gene=SHADOWnew_component->gene_string.first_gene;                           //     }
               SHADOWrule_gene <= SHADOWrule->gene_string.last_gene;                                             //     }
               SHADOWrule_gene++,SHADOWnew_component_gene++)                                                     //     }
            *SHADOWnew_component_gene = *SHADOWrule_gene;                                                        //     }
          SHADOWnew_component->gene_string.last_gene = SHADOWnew_component_gene - 1;                             //     }
          if (!(components[SHADOWcomponent_length].shadow_components_from))                                      //     }   }<-NB: if first shadow component of this length
            components[SHADOWcomponent_length].shadow_components_from = SHADOWnew_component;                     //     }   }<-    then set shadow_components_from
        }                                                                                                        //     }
      }                                                                                                          //
#ifndef only_generate_rules_for_shadows_when_needed
#else   //only_generate_rules_for_shadows_when_needed
    SHADOWindividual->genotype = new Genotype; SHADOWindividual->genotype->last_gene = SHADOWindividual->genotype->first_gene = new Gene [1]; // dummy genotype for Ann::~Ann
    delete SHADOWindividual;
#endif  //only_generate_rules_for_shadows_when_needed
#ifdef   compute_complexities
      if (SHADOWcomponent_count>SHADOWmax_component_count) SHADOWmax_component_count=SHADOWcomponent_count;
      SHADOWsum_component_count += SHADOWcomponent_count;
      if (SHADOWsum_component_lengths>SHADOWmax_sum_component_lengths) SHADOWmax_sum_component_lengths=SHADOWsum_component_lengths;
      SHADOWsum_sum_component_lengths += SHADOWsum_component_lengths;
      //
      if (SHADOWused_component_count>SHADOWmax_used_component_count) SHADOWmax_used_component_count=SHADOWused_component_count;
      SHADOWsum_used_component_count += SHADOWused_component_count;
      if (SHADOWsum_used_component_lengths>SHADOWmax_sum_used_component_lengths) SHADOWmax_sum_used_component_lengths=SHADOWsum_used_component_lengths;
      SHADOWsum_sum_used_component_lengths += SHADOWsum_used_component_lengths;
    }
#endif //compute_complexities
  }                                                                                                              //
#ifdef   compute_complexities
  double SHADOWmean_component_count = ((double)SHADOWsum_component_count) / ((double)SHADOWstats__num_individuals);
  double SHADOWmean_sum_component_lengths = ((double)SHADOWsum_sum_component_lengths) / ((double)SHADOWstats__num_individuals);
  printf("Complexities at time=%ld: SHADOWmax_component_count=%d SHADOWmean_component_count=%lf SHADOWmax_sum_component_lengths=%d SHADOWmean_sum_component_lengths=%lf\n",
          GA_time,SHADOWmax_component_count,SHADOWmean_component_count,SHADOWmax_sum_component_lengths,SHADOWmean_sum_component_lengths);
  double SHADOWmean_used_component_count = ((double)SHADOWsum_used_component_count) / ((double)SHADOWstats__num_individuals);
  double SHADOWmean_sum_used_component_lengths = ((double)SHADOWsum_sum_used_component_lengths) / ((double)SHADOWstats__num_individuals);
  printf("Complexities of used components at time=%ld: SHADOWmax_used_component_count=%d SHADOWmean_used_component_count=%lf SHADOWmax_sum_used_component_lengths=%d SHADOWmean_sum_used_component_lengths=%lf\n",
          GA_time,SHADOWmax_used_component_count,SHADOWmean_used_component_count,SHADOWmax_sum_used_component_lengths,SHADOWmean_sum_used_component_lengths);
#endif //compute_complexities
  fprintf(SHADOWrulestat_file,"%ld:",GA_time);                                                                   //  Write timestep to rulestat file
  fprintf(SHADOWrulestat_verifier_file,"%ld:",GA_time);                                                          //  Write timestep to rulestat verifier file
  long int SHADOWverifier0=0,SHADOWverifier1=0,SHADOWverifier2=0,SHADOWverifier3=0;                              //  Zero verifiers
 {for (int component_length=0;component_length<=max_component_length;component_length++)                         // }For all component lists
  { if (components[component_length].first_component)                                                            //   }
    { int rule_num=1;                                                                                            //   }For all currently existing components
      for (Component *component=components[component_length].first_component;                                    //   }
         component!=components[component_length].shadow_components_from;                                         //   } <-NB: except the shadow components
         rule_num++,component=component->next)                                                                   //   }
        if (component->SHADOWabundance_of_component)                                                             //   }
        { fprintf(SHADOWrulestat_file," %d.%d",component_length,rule_num);                                       //     }Write component's name and
          fprintf(SHADOWrulestat_file," %d;",component->SHADOWabundance_of_component);                           //     }  SHADOWabundance_of_component to rulestat file
          SHADOWverifier0+=component_length; SHADOWverifier1+=rule_num;                                          //     }Update verifiers
          SHADOWverifier2++; SHADOWverifier3+=component->SHADOWabundance_of_component;                           //     }
        }                                                                                                        //
    }                                                                                                            //
    if (components[component_length].shadow_components_from)                                                     //   }
    { int SHADOWrule_num=1;                                                                                      //   }
      for (Component *SHADOWcomponent=components[component_length].shadow_components_from;                       //   }
         SHADOWcomponent;SHADOWrule_num++,SHADOWcomponent=SHADOWcomponent->next)                                 //   } <-NB: only the shadow components
        if (SHADOWcomponent->SHADOWabundance_of_component)                                                       //   }
        { fprintf(SHADOWrulestat_file," %d.%ds%ld",component_length,SHADOWrule_num,GA_time/rulestat_every);      //     }Write component's name, *including "s"GA_time/...*, and
          fprintf(SHADOWrulestat_file," %d;",SHADOWcomponent->SHADOWabundance_of_component);                     //     }  SHADOWabundance_of_component to rulestat file
          SHADOWverifier0+=component_length; SHADOWverifier1+=SHADOWrule_num;                                    //     }Update verifiers
          SHADOWverifier2++; SHADOWverifier3+=SHADOWcomponent->SHADOWabundance_of_component;                     //     }
        }                                                                                                        //
    }                                                                                                            //
  }                                                                                                              //
 }
  fprintf(SHADOWrulestat_file,"\n"); fflush(SHADOWrulestat_file);                                                //  Write newline to rulestat file and flush
  fprintf(SHADOWrulestat_verifier_file," %ld %ld %ld %ld",SHADOWverifier0,SHADOWverifier1,SHADOWverifier2,SHADOWverifier3); //  Write verifiers to rulestat verifier file
  fprintf(SHADOWrulestat_verifier_file,"\n"); fflush(SHADOWrulestat_verifier_file);                                         //  Write newline to rulestat verifier file and flush
  // And now reset the actual shadows to the reals, for this next pass :
  int SHADOWindividual_num=-1;
  for (int real_individual_at=0;real_individual_at<world_size;real_individual_at++)                     // }For each real individual
  { Organism *real_individual = world[real_individual_at];                                              // }
    if (real_individual)                                                                                // }
    { SHADOWindividual_num++;                                                                           //    Using corresponding shadow individual number:
#ifndef only_generate_rules_for_shadows_when_needed
      delete SHADOWworld[SHADOWindividual_num];                                                         //    Kill the shadow individual
#else   //only_generate_rules_for_shadows_when_needed
      delete SHADOWgenotypes[SHADOWindividual_num];                                                     //    Kill the shadow genotype
#endif  //only_generate_rules_for_shadows_when_needed
      int SHADOWnew_genotype_length = real_individual->genotype->length();                              //   }Clone the
      Genotype *SHADOWnew_genotype = new Genotype;                                                      //   }real individual
      SHADOWnew_genotype->first_gene = new Gene [SHADOWnew_genotype_length];                            //   }into the shadow
      SHADOWnew_genotype->last_gene  = SHADOWnew_genotype->first_gene + SHADOWnew_genotype_length -1;   //   }individual
      Gene *real_gene,*shadow_gene;                                                                     //   }
      for (real_gene=real_individual->genotype->first_gene,                                             //   }
             shadow_gene=SHADOWnew_genotype->first_gene;                                                //   }
           real_gene <= real_individual->genotype->last_gene;                                           //   }
           real_gene++,shadow_gene++)                                                                   //   }
        *shadow_gene = *real_gene;                                                                      //   }
#ifndef only_generate_rules_for_shadows_when_needed
      SHADOWworld[SHADOWindividual_num] = new Organism(SHADOWnew_genotype,-1,-1);                       //   }
      SHADOWworld[SHADOWindividual_num]->stats__age = real_individual->stats__age;                      //   }
      SHADOWworld[SHADOWindividual_num]->birth_time = real_individual->birth_time;                      //   }
      //SHADOW organisms don't have a position; -1,-1 used by Organism::~Organism denote SHADOW         //   }
#ifndef compute_SHADOW_used_component_Complexities                                                      //   }
#else   //compute_SHADOW_used_component_Complexities                                                    //   }
#ifndef morphogenesis_during_life                                                                       //   }
#else   //morphogenesis_during_life                                                                     //   }
      for (int loop=0;loop<SHADOWworld[SHADOWindividual_num]->stats__age;loop++)                        //   }
        SHADOWworld[SHADOWindividual_num]->morphogenesis_step();                                        //   }
#endif  //morphogenesis_during_life                                                                     //   }
#endif  //compute_SHADOW_used_component_Complexities                                                    //   }
#else   //only_generate_rules_for_shadows_when_needed
      SHADOWgenotypes[SHADOWindividual_num] = SHADOWnew_genotype;                                       //   }
      SHADOWstats__ages[SHADOWindividual_num] = real_individual->stats__age;                            //   }
      SHADOWbirth_times[SHADOWindividual_num] = real_individual->birth_time;                            //   }
#endif  //only_generate_rules_for_shadows_when_needed
    }
  }
}

void geb_rulestat__write_snapshot(FILE *file)
{ fprintf(file,"\nRulestat snapshot:\n");                                                            //    Write newline
  fprintf(file,"%s %d\n","max_component_length",max_component_length);                               //    Write max_component_length
  for (int component_length=0;component_length<=max_component_length;component_length++)             //   }For all components
    if (components[component_length].first_component)                                                //   }
    { int rule_num=1;                                                                                //   }
      for (Component *component=components[component_length].first_component;                        //   }
         component!=components[component_length].shadow_components_from;                             //   } <-NB: except the shadow components
         rule_num++,component=component->next)                                                       //   }
      {  fprintf(file,"%d %d ",component_length,rule_num);                                           //      Write component's name
         for (Gene *gene=component->gene_string.first_gene;                                          //     }Write genes of rule
              gene<=component->gene_string.last_gene;gene++)                                         //     }
           fprintf(file,"%d",(int) *gene);                                                           //     }
         fprintf(file,"\n");                                                                         //      Write newline
      }
    }
  //Shadow components for information only - not needed for a restart:
  fprintf(file,"\nSHADOWRulestat snapshot, extends those above (informational only - not needed for a restart):\n"); //    Write newline
  for (int SHADOWcomponent_length=0;SHADOWcomponent_length<=max_component_length;SHADOWcomponent_length++)           //   }For all components
    if (components[SHADOWcomponent_length].shadow_components_from)                                                   //   }
    { int SHADOWrule_num=1;                                                                                          //   }
      for (Component *SHADOWcomponent=components[SHADOWcomponent_length].shadow_components_from;                     //   } <-NB: only the shadow components
         SHADOWcomponent;SHADOWrule_num++,SHADOWcomponent=SHADOWcomponent->next)                                     //   }
      {  fprintf(file,"%d %d ",SHADOWcomponent_length,SHADOWrule_num);                                               //      Write component's name
         for (Gene *SHADOWgene=SHADOWcomponent->gene_string.first_gene;                                              //     }Write genes of rule
              SHADOWgene<=SHADOWcomponent->gene_string.last_gene;SHADOWgene++)                                       //     }
           fprintf(file,"%d",(int) *SHADOWgene);                                                                     //     }
         fprintf(file,"\n");                                                                                         //      Write newline
      }
    }
  fprintf(file,"-999\n"); // new for random number state save&load: explicitly write an end-of-section marker here
}

void geb_rulestat__read_snapshot_and_initialise(FILE *file)
{ char *dummy_string = new char[1000];
  Fgets(dummy_string,1000,file); Fgets(dummy_string,1000,file); //get passed newlines
  Fgets(dummy_string,1000,file);
  if (strcmp(dummy_string,"Rulestat snapshot:\n"))
    {fprintf(stderr,"Error: Could not find start of rulestat snapshot\n"); fflush(stderr); exit(91);}
  delete [] dummy_string;
  Fscanf(file,"%*s %d",&max_component_length);
  components = new Component_list [1+max_component_length];
  Component *component;
  for (int component_length;component_length=-999,(fscanf(file,"%d",&component_length)==1 && component_length!=-999);)
  { if ((component_length<1) || (component_length>max_component_length))
      {fprintf(stderr,"Error: Bad component number in rulestat snapshot\n"); fflush(stderr); exit(92);}
    int number; Fscanf(file," %d ",&number);
    if (   ((number == 1) &&  components[component_length].first_component)
        || ((number != 1) && !components[component_length].first_component)
       )
      {fprintf(stderr,"Error: Bad component number sequence in rulestat snapshot\n"); fflush(stderr); exit(93);}
    if (number == 1) component = components[component_length].first_component = new Component;
    else             component = component->next = new Component;
    component->gene_string.first_gene = new Gene [component_length];
    int n; Gene *gene;
    for (n=1,gene=component->gene_string.first_gene;n<=component_length;n++,gene++) {*gene = fgetc(file) - '0';}
    component->gene_string.last_gene = gene - 1;
  }
  //Shadow components are informational only, but read them in anyway in case another snapshot write is requested before the next pass
  char *SHADOWdummy_string = new char[1000];
  //Fgets(SHADOWdummy_string,1000,file); Fgets(SHADOWdummy_string,1000,file); //get passed newlines
  Fgets(SHADOWdummy_string,1000,file);
  if (strcmp(SHADOWdummy_string,"SHADOWRulestat snapshot, extends those above (informational only - not needed for a restart):\n"))
    {fprintf(stderr,"Error: Could not find start of SHADOWrulestat snapshot\n"); fflush(stderr); exit(94);}
  delete [] SHADOWdummy_string;
  Component *SHADOWcomponent;
  for (int SHADOWcomponent_length;SHADOWcomponent_length=-999,(fscanf(file,"%d",&SHADOWcomponent_length)==1 && SHADOWcomponent_length!=-999);)
  { if ((SHADOWcomponent_length<1) || (SHADOWcomponent_length>max_component_length))
      {fprintf(stderr,"Error: Bad component number in SHADOWrulestat snapshot\n"); fflush(stderr); exit(95);}
    int SHADOWnumber; Fscanf(file," %d ",&SHADOWnumber);
    if (   ((SHADOWnumber == 1) &&  components[SHADOWcomponent_length].shadow_components_from)
        || ((SHADOWnumber != 1) && !components[SHADOWcomponent_length].shadow_components_from)
       )
      {fprintf(stderr,"Error: Bad component number sequence in SHADOWrulestat snapshot\n"); fflush(stderr); exit(96);}
    if (SHADOWnumber == 1)
    { SHADOWcomponent = components[SHADOWcomponent_length].shadow_components_from = new Component;
      if (components[SHADOWcomponent_length].first_component)
      { Component* last_real_component=components[SHADOWcomponent_length].first_component;
        for (;last_real_component->next;last_real_component=last_real_component->next);
        last_real_component->next=SHADOWcomponent;
      }
      else components[SHADOWcomponent_length].first_component=components[SHADOWcomponent_length].shadow_components_from;
    }
    else             SHADOWcomponent = SHADOWcomponent->next = new Component;
    SHADOWcomponent->gene_string.first_gene = new Gene [SHADOWcomponent_length];
    int SHADOWn; Gene *SHADOWgene;
    for (SHADOWn=1,SHADOWgene=SHADOWcomponent->gene_string.first_gene;SHADOWn<=SHADOWcomponent_length;SHADOWn++,SHADOWgene++) {*SHADOWgene = fgetc(file) - '0';}
    SHADOWcomponent->gene_string.last_gene = SHADOWgene - 1;
  }
  geb_rulestat_initialise_just_rulestat_file();
}

// ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#endif  //reset_shadow_to_real_for_each_pass
