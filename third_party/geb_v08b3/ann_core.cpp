/* Copyright (c) 2008 Alastair Channon
* 
* Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
* 
* The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

/* ann_core.cpp */
#include "ann_core.h"
#include "rnd_core.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

//usage note: randomize() should be called for random noise in nodes

int max_num_nodes = 20; //default

//#define test__use_four_inputs
#ifndef test__use_four_inputs
#else   //test__use_four_inputs
#warning: Using test__use_four_inputs - check same as geb_core.cpp
#endif  //test__use_four_inputs

//now specified (or not) in ann_core.h: #define dont_use_contexts

#ifndef dont_use_contexts
//now in ann_core.h: #warning: Using contexts (that is okay - just an option)
// //#define use_contexts__use_read_rule_filter //slower(*1.5) but less memory(*.8)
// #ifndef use_contexts__use_read_rule_filter
// #else   //use_contexts__use_read_rule_filter
// #warning: Using use_contexts__use_read_rule_filter
// #endif  //use_contexts__use_read_rule_filter
#else   //dont_use_contexts
//now in ann_core.h: //#warning: Not using contexts (that is okay - just an option)
#define dont_use_contexts__max_optimised_filtered_rules_generation
#ifndef dont_use_contexts__max_optimised_filtered_rules_generation
#warning NOT USING dont_use_contexts__max_optimised_filtered_rules_generation
//#define dont_use_contexts__use_most_basic__filter_rules //not much difference (except not suitable for rulestat)
#ifndef dont_use_contexts__use_most_basic__filter_rules
#define dont_use_contexts__full_filter__use_optimised_filtering
#ifndef dont_use_contexts__full_filter__use_optimised_filtering
#else   //dont_use_contexts__full_filter__use_optimised_filtering
//geb_v06__default: #warning: Using dont_use_contexts__full_filter__use_optimised_filtering
#endif  //dont_use_contexts__full_filter__use_optimised_filtering
#else   //dont_use_contexts__use_most_basic__filter_rules
#warning: Using dont_use_contexts__use_most_basic__filter_rules
#endif  //dont_use_contexts__use_most_basic__filter_rules
#endif  //dont_use_contexts__max_optimised_filtered_rules_generation
#endif  //dont_use_contexts

#define links_between_new_and_replacement_replace_inherited_I_and_or_O_links
//#warning: Using links_between_new_and_replacement_replace_inherited_I_and_or_O_links

//#define temp_use_just_test_genotype


//GENES

int Gene_string::length() const
{ if (!first_gene) return -1;  //so that a context "?," beats no context.
  return (int) (1+last_gene-first_gene);
}

int Gene_string::is_equal_to(const Gene_string &compare_with) const
{ if (length()  != compare_with.length()) return 0;
  if (!first_gene) return (compare_with.first_gene?0:1);
  Gene *this_gene=first_gene;
  Gene *compare_gene=compare_with.first_gene;
  for (;this_gene<=last_gene;this_gene++,compare_gene++)
    if (*this_gene != *compare_gene) return 0;
  return 1;
}

int Gene_string::is_start_of(const Gene_string &compare_with) const
{ if (length()  > compare_with.length()) return 0;
  if (!first_gene) return 1; //to be safe
  Gene *this_gene=first_gene;
  Gene *compare_gene=compare_with.first_gene;
  for (;this_gene<=last_gene;this_gene++,compare_gene++)
    if (*this_gene != *compare_gene) return 0;
  return 1;
}

void Gene_string::create_as_character(const char* asciibits)
{ int asciibits_length = strlen(asciibits);
  first_gene = new Gene [asciibits_length<<1]; // 0,bit, 0,bit, ...
  last_gene = first_gene -1 + (asciibits_length<<1);
  int i; for (i=0;i<asciibits_length;i++)
  { first_gene[(i<<1)  ] = 0;
    first_gene[(i<<1)+1] = 1&(asciibits[i]); // 1&'0'=0,1&'1'=1 (ASCII: '0'=48, '1'=49)
  }
}

Gene_string Genotype::get_segment(Gene *from_gene)
{ Gene_string segment;
  segment.last_gene = segment.first_gene = from_gene;
  while ((segment.last_gene < last_gene) && (*(segment.last_gene) == 0))
    segment.last_gene += 2;
  segment.last_gene -= 1;  // note last_gene may= first_gene - 1
  return segment;
  // genes are read in pairs;  10 and 11 are both terminal-pairs
  // 'for(gene=first;gene<last;gene+=2)' gives the gene-pairs (all 00 or 01)
  // 'for(gene=first+1;gene<=last;gene+=2)' gives the 2nd(of pair)-genes
}


//ANNS

//Note: nodes contain lists of links; nets contain lists of nodes.
//      in each case, the first element in a list is a dummy
//      (this is so that the first real element can be easily removed)

Link::Link(Link *previous_link,Link *next_link)              // constructor
{ previous=previous_link;
  next=next_link;
  other_node=0;
  other_nodes_link=0;
}

Link::~Link()
{ if (previous) previous->next = next;
  if (next)     next->previous = previous;
}

Link *Link::new_link()
{ Link *a_new_link = new Link(this,next);
  if (next) next->previous = a_new_link;
  next = a_new_link;
  return a_new_link;
}

Node_ptr::Node_ptr()                                         // constructor
{ next = 0;
  io_value = 0;
}

Node_ptr *Node_ptr::new_node_ptr()
{ Node_ptr *a_new_node_ptr = new Node_ptr();
  next = a_new_node_ptr;
  return a_new_node_ptr;
}

Node::Node(Node *previous_node,Node *next_node)              // constructor
{ previous=previous_node;
  next=next_node;
  inhibitory_input_links  = new Link(0,0); // dummy link
  excitatory_input_links  = new Link(0,0); // dummy link
  inhibitory_output_links = new Link(0,0); // dummy link
  excitatory_output_links = new Link(0,0); // dummy link
  net_input_node_ptr = net_output_node_ptr = 0;
  inhibitory_output = excitatory_output = 0;
  just_constructed = 1; //for morphogenesis_step()
}

Node::~Node()                                                // destructor
{ delete_links(inhibitory_input_links);
  delete_links(excitatory_input_links);
  delete_links(inhibitory_output_links);
  delete_links(excitatory_output_links);
  if (previous) previous->next = next;
  if (next)     next->previous = previous;
  if (net_input_node_ptr)           //if node is a net input node
  { net_input_node_ptr->node = 0;   //  then blank that input
    net_input_node_ptr->io_value = 0;//this line not necessary [because io_value
  }                                  //  of a deleted net_input never used]
  if (net_output_node_ptr)          //if node is a net output node
  { net_output_node_ptr->node= 0;   //  then blank that output
    net_output_node_ptr->io_value= 0;//this line wanted [in case io_value of a
  }                                  //  deleted net_output used outside ann]
}

Node *Node::new_node()
{ Node *a_new_node = new Node(this,next);
  if (next) next->previous = a_new_node;
  next = a_new_node;
  return a_new_node;
}

void Node::delete_links(Link *links)
{ // could just use: while (links->next) delete (links->next); delete links;
  // but the following is better:
  Link *link,*next_link;
  for (link=links;link;link=next_link) //including dummy
  { next_link=link->next;
    delete link->other_nodes_link;
    delete link;
  }
}

#ifndef dont_use_contexts
int Node::context_match(int left,Gene excitatory,Gene_string context)
{ if (!(context.first_gene)) return 1; // empty context [no tory] always matches
  Link *relevant_links,* link;
  if (left) // if left context
  { if (excitatory) relevant_links = excitatory_input_links;
    else            relevant_links = inhibitory_input_links;
  }
  else      // else right context
  { if (excitatory) relevant_links = excitatory_output_links;
    else            relevant_links = inhibitory_output_links;
  }
  for (link=relevant_links->next;link;link=link->next) //not dummy
    if (context.is_start_of(link->other_node->character))
      return 1;
  return 0;
}
#endif  //dont_use_contexts

void Node::calc_new_outputs()
{ int inhibitory_input, excitatory_input;
  Link *link;
  inhibitory_input = excitatory_input = 0;
  for (link=inhibitory_input_links->next;link;link=link->next)
    if (link->other_node->inhibitory_output)
    { inhibitory_input = 1;  // any inhibitory input
      break;                 // causes inhibition
    }
  for (link=excitatory_input_links->next;link;link=link->next)
    excitatory_input += link->other_node->excitatory_output;
  if (net_input_node_ptr)    // if this is a net input node
    excitatory_input += net_input_node_ptr->io_value;
  if (excitatory_input<t_v) new_inhibitory_output = 0;
  else                      new_inhibitory_output = 1;
  if (inhibitory_input)
    new_excitatory_output = 0;
  else
  { if (random(2)) excitatory_input += random(max_noise); //noise not used in
    else          {excitatory_input -= random(max_noise); //xor network tests
                   if (excitatory_input<0) excitatory_input=0;
                   //NB: In some old versions it was lucky that (-n)>>1==-ve [in fact==-((n+1)>>1)],
                   //    at least on the (Linux&g++&...) systems used.  This is no longer required.
                  }
    new_excitatory_output = excitatory_input>>1; //half
    // bound excitatory output:
    if (new_excitatory_output<0)
      new_excitatory_output = 0;
    else if (new_excitatory_output > excitatory_scale)
      new_excitatory_output = excitatory_scale;
  }
}

Ann::Ann(Genotype *with_genotype)                            // constructor
#ifndef temp_use_just_test_genotype
{ genotype = with_genotype;
#else   //temp_use_just_test_genotype
{ delete with_genotype;
  char *test_genotype = "!!.0.0!.1.1!!YYnYnY";
  //[.,0,n] have even ascii values [46,48,110] so 1& => 0
  //[!,1,Y] have odd  ascii values [33,49,89]  so 1& => 1
  genotype = new Genotype;
  int glength=strlen(test_genotype);
  genotype->first_gene = new Gene [glength];
  genotype->last_gene = genotype->first_gene -1 +glength;
  for (int i=0;i<glength;i++)
    (genotype->first_gene)[i] = 1&test_genotype[i];
#warning: USING FIXED GENOTYPE FOR TESTING
#endif  //temp_use_just_test_genotype
  nodes = new Node(0,0); // dummy node
  last_net_input  = net_inputs  = new Node_ptr(); // dummy node_ptr
  last_net_output = net_outputs = new Node_ptr(); // dummy node_ptr
  create_axiom_net();
  generate_rules();   //note: must come after create_axiom_net() for rule filtering
  num_morphogenesis_steps_left = max_num_morphogenesis_steps;
}

Ann::~Ann()                                                  // destructor
{ delete_nodes();
  delete_node_ptrs(net_inputs);
  delete_node_ptrs(net_outputs);
  //may now be at end of morphogenesis: delete_rules();
  delete_rules();
  delete genotype;
}

void Ann::delete_nodes()
{ Node *node,*next_node;
  for (node=nodes;node;node=next_node) //including dummy
  { next_node=node->next;
    delete node;
  }
  delete [] axiom_input_node_character.first_gene;
  delete [] axiom_hidden_node_character.first_gene;
  delete [] axiom_output_node_character.first_gene;
}

void Ann::delete_node_ptrs(Node_ptr *node_ptrs)
{ Node_ptr *node_ptr,*next_node_ptr;
  for (node_ptr=node_ptrs;node_ptr;node_ptr=next_node_ptr) //including dummy
  { next_node_ptr=node_ptr->next;
    delete node_ptr;
  }
}

#ifndef dont_use_contexts__max_optimised_filtered_rules_generation
void Ann::clear_overlapping_rules(Production_rule *use_rule)
{ Gene *use_rule__first_gene = use_rule->gene_string.first_gene;
  Gene *use_rule__last_gene  = use_rule->gene_string.last_gene;
  //note: uses the fact that rules are stored in order along genotype
  Production_rule *rule,*next_rule,*previous_not_overlapping_rule=0;
  for (rule=first_rule;rule->gene_string.last_gene<use_rule__first_gene;rule=rule->next)
    previous_not_overlapping_rule = rule;
  if (previous_not_overlapping_rule) previous_not_overlapping_rule->next = use_rule;
  else first_rule = use_rule;
  for (;rule->gene_string.first_gene<use_rule__first_gene;rule=next_rule)
  { next_rule = rule->next;
    delete rule;
  }
  for (rule=rule->next;rule && (rule->gene_string.first_gene<=use_rule__last_gene);rule=next_rule)
  { next_rule = rule->next;
    delete rule;
  }
  use_rule->next = rule;
  if (!rule) last_rule = use_rule;
}

void Ann::most_basic__filter_rules()
{ for (Production_rule *rule=first_rule;rule;rule=rule->next)
    rule->keep_rule = maybe;
  //clearing of overlapping rules left until rule is used (for first time)
}

#ifndef dont_use_contexts
void Ann::filter_rules() {most_basic__filter_rules();}
//Because clear_overlapping_rules will be called in run_rule - will happen for real pop. but not shadow pop.
#warning: DO NOT USE RULESTAT when not using dont_use_contexts
#else   //dont_use_contexts
#ifndef dont_use_contexts__use_most_basic__filter_rules
void Ann::filter_rules()
{ for (Production_rule *rule__keep=first_rule;rule__keep;rule__keep=rule__keep->next)
  { rule__keep->keep_rule = no; // don't yet know to keep
    rule__keep->already_checked_successors_during_filtering = 0;
  }
  int num_rules_after_filtering=0;
  for (Node *node=nodes->next;node;node=node->next) //not dummy
  { pick_update_rule_for_node(node);
    //if (node->update_rule)
    if (node->update_rule && !node->update_rule->keep_rule)
    { node->update_rule->keep_rule = definitely; //set to keep
      clear_overlapping_rules(node->update_rule);
      num_rules_after_filtering++;
    }
  }
#ifndef dont_use_contexts__full_filter__use_optimised_filtering
#else   //dont_use_contexts__full_filter__use_optimised_filtering
  Production_rule** rules_array; // } rules_array[number_rules_read]
  int number_rules_read;         // } ... rules are in same order as the linked list.
  int** rules_numbers;                       // } rules_numbers[1+max_predecessor_length]"[number_rules_with_predecessor_length]"
  int max_predecessor_length;                // } ... at any length, rules are in same order as rules_array
  int* number_rules_with_predecessor_length; // } ... and so in the same order as in the linked list.
  Production_rule* rule;  // } repeated-use variables
  int predecessor_length; // } for sequential loops
  // Setup 1st dimensions for rules_array (only 1D anyway) and rules_numbers:
  number_rules_read=max_predecessor_length=0;
  for (rule=first_rule;rule;rule=rule->next)
  { if (rule->predecessor.length() > max_predecessor_length) max_predecessor_length = rule->predecessor.length();
    number_rules_read++;
  }
  if (!number_rules_read) return; // new
  rules_array = new Production_rule* [number_rules_read];
  rules_numbers = new int* [1+max_predecessor_length];
  // Setup rules_numbers's 2nd-index-array (number_rules_with_predecessor_length):
  number_rules_with_predecessor_length = new int [1+max_predecessor_length];
  for (predecessor_length=0;predecessor_length<=max_predecessor_length;predecessor_length++) number_rules_with_predecessor_length[predecessor_length] = 0;
  for (rule=first_rule;rule;rule=rule->next) number_rules_with_predecessor_length[rule->predecessor.length()]++;
  // Setup 2nd dimension for rules_numbers:
  for (predecessor_length=0;predecessor_length<=max_predecessor_length;predecessor_length++)
  { if (!(number_rules_with_predecessor_length[predecessor_length])) continue;
    rules_numbers[predecessor_length] = new int [number_rules_with_predecessor_length[predecessor_length]];
    number_rules_with_predecessor_length[predecessor_length] = 0; // re-zero - will be reused and recalculated just below
  }
  // Setup contents for rules_array and rules_numbers:
  int rule_number;
  for (rule_number=0,rule=first_rule;rule;rule_number++,rule=rule->next)
  { predecessor_length = rule->predecessor.length();
    rules_array[rule_number] = rule;
    rules_numbers[predecessor_length][number_rules_with_predecessor_length[predecessor_length]] = rule_number;
    number_rules_with_predecessor_length[predecessor_length]++;
  }
#endif  //dont_use_contexts__full_filter__use_optimised_filtering
  int previous_num_rules_after_filtering = 0;
  while (num_rules_after_filtering != previous_num_rules_after_filtering)
  { previous_num_rules_after_filtering = num_rules_after_filtering;
    for (Production_rule *kept_rule=first_rule;kept_rule;kept_rule=kept_rule->next)
    { if (!(kept_rule->keep_rule)) continue; //because not yet kept
      if (kept_rule->already_checked_successors_during_filtering) continue;
      kept_rule->already_checked_successors_during_filtering = 1; //will have by end of cycle
      for (Gene_string *character_ptr=&(kept_rule->successor_replacement);character_ptr;
                        character_ptr=(character_ptr==&(kept_rule->successor_replacement))?
                                                      &(kept_rule->successor_new):0)
      { Production_rule *to_keep_rule=0;
#ifndef dont_use_contexts__full_filter__use_optimised_filtering
        int best__predecessor_length=-999;
        for (Production_rule *test_rule=first_rule;test_rule;test_rule=test_rule->next)
        { if (test_rule->predecessor.length() <= best__predecessor_length) continue;
          if (!(test_rule->predecessor.is_start_of(*character_ptr)))       continue;
          best__predecessor_length = test_rule->predecessor.length();
          to_keep_rule=test_rule;
        }
#else   //dont_use_contexts__full_filter__use_optimised_filtering
        int to_keep_rule_rule_number;
        int successor_length = character_ptr->length();
        predecessor_length=(successor_length<max_predecessor_length)?successor_length:max_predecessor_length; // start at the top
        for (;(!to_keep_rule)&&(predecessor_length>=0);predecessor_length--)
          for (int rule_subnumber=0;(!to_keep_rule)&&(rule_subnumber<number_rules_with_predecessor_length[predecessor_length]);rule_subnumber++)
            { Production_rule *test_rule=rules_array[rules_numbers[predecessor_length][rule_subnumber]];
              if (!test_rule)                                                  continue; // check if rule has been deleted
              if (!(test_rule->predecessor.is_start_of(*character_ptr)))       continue;
              to_keep_rule=test_rule;
              to_keep_rule_rule_number=rules_numbers[predecessor_length][rule_subnumber];
            }
#endif  //dont_use_contexts__full_filter__use_optimised_filtering
        if (to_keep_rule && (!(to_keep_rule->keep_rule))) //if a new rule to be kept
        { to_keep_rule->keep_rule = definitely; //set to keep
          clear_overlapping_rules(to_keep_rule);
#ifndef dont_use_contexts__full_filter__use_optimised_filtering
#else   //dont_use_contexts__full_filter__use_optimised_filtering
          int deleted_rule_number=0;
          if (first_rule!=to_keep_rule)
          { while (!(rules_array[deleted_rule_number]) || (rules_array[deleted_rule_number]->next!=to_keep_rule)) deleted_rule_number++;
            deleted_rule_number++;
          }
          for (;deleted_rule_number<to_keep_rule_rule_number;deleted_rule_number++)
            rules_array[deleted_rule_number]=0;
          deleted_rule_number++; // skip over to_keep_rule
          if (to_keep_rule!=last_rule)
            for (;rules_array[deleted_rule_number]!=to_keep_rule->next;deleted_rule_number++) rules_array[deleted_rule_number]=0;
          else
            for (;deleted_rule_number<number_rules_read;deleted_rule_number++) rules_array[deleted_rule_number]=0;
#endif  //dont_use_contexts__full_filter__use_optimised_filtering
          num_rules_after_filtering++;
        }
      }
    }
  }
  //delete rules that will not be used:
  Production_rule *next_rule,*previous_rule=0;
  for (Production_rule *rule__del=first_rule;rule__del;rule__del=next_rule)
  { next_rule=rule__del->next;
    if (rule__del->keep_rule) previous_rule=rule__del;
    else //delete the rule:
    { if (previous_rule) previous_rule->next = next_rule;
      else                        first_rule = next_rule;
      if (rule__del == last_rule) last_rule = previous_rule;
      delete rule__del;
    }
  }
#ifndef dont_use_contexts__full_filter__use_optimised_filtering
#else   //dont_use_contexts__full_filter__use_optimised_filtering
  for (predecessor_length=0;predecessor_length<=max_predecessor_length;predecessor_length++)
    if (number_rules_with_predecessor_length[predecessor_length]) delete [] rules_numbers[predecessor_length];
  delete [] rules_array;
  delete [] rules_numbers;
  delete [] number_rules_with_predecessor_length;
#endif  //dont_use_contexts__full_filter__use_optimised_filtering
}
#else   //dont_use_contexts__use_most_basic__filter_rules
void Ann::filter_rules() {most_basic__filter_rules();}
//Because clear_overlapping_rules will be called in run_rule - will happen for real pop. but not shadow pop.
#warning: DO NOT USE RULESTAT when using dont_use_contexts__use_most_basic__filter_rules
#endif  //use_mostmost_basic__filter_rules
#endif  //dont_use_contexts

void Ann::generate_rules()
{ first_rule = last_rule = 0;
#ifndef dont_use_contexts
  for (Gene *gene=genotype->first_gene;(genotype->last_gene-gene)>11;gene++)
  // min rule length=2['11']+5['1?'s]+6=13,so want gene<last-11
#else   //dont_use_contexts
  for (Gene *gene=genotype->first_gene;(genotype->last_gene-gene)>9;gene++)
  // min rule length=2['11']+3['1?'s]+6=11,so want gene<last-9
#endif  //dont_use_contexts
    try_to_read_rule(gene);
  filter_rules();
}
#else   //dont_use_contexts__max_optimised_filtered_rules_generation
void Ann::clear_overlapping_rules(Production_rule *use_rule) {fprintf(stderr,"Error - this function should not have been called\n"); exit(11);}
void Ann::most_basic__filter_rules()                         {fprintf(stderr,"Error - this function should not have been called\n"); exit(12);}
void Ann::filter_rules()                                     {fprintf(stderr,"Error - this function should not have been called\n"); exit(13);}
class Predecessor_node // node as in member of a list
{ public:    Gene_string predecessor;
             Predecessor_node* next;
             Predecessor_node* previous;
             Production_rule* production_rule;
             Predecessor_node* next_kept;
};
void Ann::generate_rules()
{ first_rule = last_rule = 0;
  Predecessor_node *first__possible_predecessor_node, *last__possible_predecessor_node;
  first__possible_predecessor_node = last__possible_predecessor_node = 0;
  Gene *gene;
  Predecessor_node* new_predecessor_node = new Predecessor_node; // always have one and delete one at end
  for (gene=genotype->first_gene;(genotype->last_gene-gene)>9;gene++)
  // min rule length=2['11']+3['1?'s]+6=11,so want gene<last-9
    // try to read rule-start and predecessor from gene onwards:
    if (*gene && *(gene+1)) // check is start of a possible rule
    { new_predecessor_node->predecessor = genotype->get_segment(gene+2);
      if ((new_predecessor_node->predecessor.last_gene + 9) <= genotype->last_gene) // if might be enough genes for a rule
      { //add predecessor:
        new_predecessor_node->next = new_predecessor_node->previous = 0;
        new_predecessor_node->production_rule = 0;
        if (!first__possible_predecessor_node) first__possible_predecessor_node = last__possible_predecessor_node = new_predecessor_node;
        else
        { last__possible_predecessor_node->next = new_predecessor_node;
          new_predecessor_node->previous = last__possible_predecessor_node;
          last__possible_predecessor_node = new_predecessor_node;
        }
        new_predecessor_node = new Predecessor_node;
      }
    }
  delete new_predecessor_node; // ^-"always have one and delete one at end"-^
  Predecessor_node** predecessor_nodes_array;            // } predecessor_nodes_array[number_predecessors_read]
  int number_predecessors_read;                          // } ... predecessors are in same order as the linked list.
  int** predecessor_nodes_numbers;                       // } predecessor_nodes_numbers[1+max_predecessor_length]"[number_predecessor_nodes_with_predecessor_length]"
  int max_predecessor_length;                            // } ... at any length, predecessors are in same order as predecessor_nodes_array
  int* number_predecessor_nodes_with_predecessor_length; // } ... and so in the same order as in the linked list.
  Predecessor_node* predecessor_node; // } repeated-use variables
  int predecessor_length;             // } for sequential loops
  // Setup 1st dimensions for predecessor_nodes_array (only 1D anyway) and predecessor_nodes_numbers:
  number_predecessors_read=max_predecessor_length=0;
  for (predecessor_node=first__possible_predecessor_node;predecessor_node;predecessor_node=predecessor_node->next)
  { if (predecessor_node->predecessor.length() > max_predecessor_length) max_predecessor_length = predecessor_node->predecessor.length();
    number_predecessors_read++;
  }
  if (!number_predecessors_read) return; // new
  predecessor_nodes_array = new Predecessor_node* [number_predecessors_read];
  predecessor_nodes_numbers = new int* [1+max_predecessor_length];
  // Setup predecessor_nodes_numbers's 2nd-index-array (number_predecessor_nodes_with_predecessor_length):
  number_predecessor_nodes_with_predecessor_length = new int [1+max_predecessor_length];
  for (predecessor_length=0;predecessor_length<=max_predecessor_length;predecessor_length++)
    number_predecessor_nodes_with_predecessor_length[predecessor_length] = 0;
  for (predecessor_node=first__possible_predecessor_node;predecessor_node;predecessor_node=predecessor_node->next)
    number_predecessor_nodes_with_predecessor_length[predecessor_node->predecessor.length()]++;
  // Setup 2nd dimension for predecessor_nodes_numbers:
  for (predecessor_length=0;predecessor_length<=max_predecessor_length;predecessor_length++)
  { if (!(number_predecessor_nodes_with_predecessor_length[predecessor_length])) continue;
    predecessor_nodes_numbers[predecessor_length] = new int [number_predecessor_nodes_with_predecessor_length[predecessor_length]];
    number_predecessor_nodes_with_predecessor_length[predecessor_length] = 0; // re-zero - will be reused and recalculated just below
  }
  // Setup contents for predecessor_nodes_array and predecessor_nodes_numbers:
  int predecessor_node_number;
  for (predecessor_node_number=0,predecessor_node=first__possible_predecessor_node;predecessor_node;predecessor_node_number++,predecessor_node=predecessor_node->next)
  { predecessor_length = predecessor_node->predecessor.length();
    predecessor_nodes_array[predecessor_node_number] = predecessor_node;
    predecessor_nodes_numbers[predecessor_length][number_predecessor_nodes_with_predecessor_length[predecessor_length]] = predecessor_node_number;
    number_predecessor_nodes_with_predecessor_length[predecessor_length]++;
  }
  Predecessor_node* first__kept_predecessor_node = 0;
  int num_rules_after_filtering=0;
  for (Node *node=nodes->next;node;node=node->next) //not dummy
  { // pick_update_rule_for_node(node);
    Predecessor_node *best__predecessor_node=0;
    // int best__predecessor_length=-999;
    // for (predecessor_node=first__possible_predecessor_node;predecessor_node;predecessor_node=predecessor_node->next)
    //   { if (!(predecessor_node->predecessor.length() > best__predecessor_length)) continue;
    predecessor_length=(node->character.length()<max_predecessor_length)?node->character.length():max_predecessor_length; // start at the top
    for (;(!best__predecessor_node)&&(predecessor_length>=0);predecessor_length--)
      for (int predecessor_node_subnumber=0;(!best__predecessor_node)&&(predecessor_node_subnumber<number_predecessor_nodes_with_predecessor_length[predecessor_length]);predecessor_node_subnumber++)
      { Predecessor_node* predecessor_node=predecessor_nodes_array[predecessor_nodes_numbers[predecessor_length][predecessor_node_subnumber]];
        //no - this method checks for overlapping rules rather than clearing them: // if (!rule) continue; // check if rule has been deleted
        if (!(predecessor_node->predecessor.is_start_of(node->character))) continue;
        if (!predecessor_node->production_rule)
        { try_to_read_rule(predecessor_node->predecessor.first_gene-2);
          if (first_rule)
          { predecessor_node->production_rule = first_rule;
            first_rule = last_rule = 0;
            predecessor_node->production_rule->keep_rule = no; // don't yet know to keep
            predecessor_node->production_rule->already_checked_successors_during_filtering = 0;
          }
        }
        if (predecessor_node->production_rule)
        { // production_rule overlapping a (different) "definitely" kept rule?
          int overlapping = 0;
          for (Predecessor_node* check_predecessor_node=first__kept_predecessor_node;(!overlapping)&&check_predecessor_node;check_predecessor_node=check_predecessor_node->next_kept)
            if (   (check_predecessor_node != predecessor_node)
                && (check_predecessor_node->production_rule->gene_string.first_gene <= predecessor_node->production_rule->gene_string.last_gene )
                && (check_predecessor_node->production_rule->gene_string.last_gene  >= predecessor_node->production_rule->gene_string.first_gene)
               )
              overlapping=1;
          if (!overlapping)
            // best__predecessor_length = predecessor_node->predecessor.length();
            best__predecessor_node = predecessor_node;
        }
      }
    if (best__predecessor_node && !(best__predecessor_node->production_rule->keep_rule)) //if a new rule to be kept
    { node->update_rule = best__predecessor_node->production_rule;
      node->update_rule->keep_rule = definitely; //set to keep
      num_rules_after_filtering++;
      if (first__kept_predecessor_node && (first__kept_predecessor_node->predecessor.first_gene < best__predecessor_node->predecessor.first_gene))
      { Predecessor_node* previous_kept_predecessor_node;
        for (  previous_kept_predecessor_node = first__kept_predecessor_node
             ; (previous_kept_predecessor_node->next_kept) && (previous_kept_predecessor_node->next_kept->predecessor.first_gene < best__predecessor_node->predecessor.first_gene)
             ; previous_kept_predecessor_node=previous_kept_predecessor_node->next_kept
            );
        best__predecessor_node->next_kept = previous_kept_predecessor_node->next_kept;
        previous_kept_predecessor_node->next_kept = best__predecessor_node;
      }
      else
      { best__predecessor_node->next_kept = first__kept_predecessor_node;
        first__kept_predecessor_node = best__predecessor_node;
      }
    }
  }
  int previous_num_rules_after_filtering = 0;
  while (num_rules_after_filtering != previous_num_rules_after_filtering)
  { previous_num_rules_after_filtering = num_rules_after_filtering;
    Predecessor_node* kept_predecessor_node;
    for (kept_predecessor_node=first__kept_predecessor_node;kept_predecessor_node;kept_predecessor_node=kept_predecessor_node->next_kept) // some may be added as we go
    { Production_rule* kept_rule = kept_predecessor_node->production_rule;
      if (kept_rule->already_checked_successors_during_filtering) continue;
      kept_rule->already_checked_successors_during_filtering = 1; //will have by end of cycle
      int successor_number;
      for (successor_number=0;successor_number<2;successor_number++) // 0=>successor_replacement , 1=>successor_new
      { Gene_string *character_ptr = successor_number ? &(kept_rule->successor_new) : &(kept_rule->successor_replacement);
        Predecessor_node *to_keep_predecessor_node=0;
        int successor_length = character_ptr->length();
        predecessor_length=(successor_length<max_predecessor_length)?successor_length:max_predecessor_length; // start at the top
        for (;(!to_keep_predecessor_node)&&(predecessor_length>=0);predecessor_length--)
          for (int predecessor_node_subnumber=0;(!to_keep_predecessor_node)&&(predecessor_node_subnumber<number_predecessor_nodes_with_predecessor_length[predecessor_length]);predecessor_node_subnumber++)
          { Predecessor_node* test_predecessor_node=predecessor_nodes_array[predecessor_nodes_numbers[predecessor_length][predecessor_node_subnumber]];
            //no - this method checks for overlapping rules rather than clearing them: // if (!test_rule) continue; // check if rule has been deleted
            if (!(test_predecessor_node->predecessor.is_start_of(*character_ptr)))       continue;
            if (!test_predecessor_node->production_rule)
            { try_to_read_rule(test_predecessor_node->predecessor.first_gene-2);
              if (first_rule)
              { test_predecessor_node->production_rule = first_rule;
                first_rule = last_rule = 0;
                test_predecessor_node->production_rule->keep_rule = no; // don't yet know to keep
                test_predecessor_node->production_rule->already_checked_successors_during_filtering = 0;
              }
            }
            if (test_predecessor_node->production_rule)
            { // production_rule overlapping a (different) "definitely" kept rule?
              int overlapping = 0;
              for (Predecessor_node* check_predecessor_node=first__kept_predecessor_node;(!overlapping)&&check_predecessor_node;check_predecessor_node=check_predecessor_node->next_kept)
                if (   (check_predecessor_node != test_predecessor_node)
                    && (check_predecessor_node->production_rule->gene_string.first_gene <= test_predecessor_node->production_rule->gene_string.last_gene )
                    && (check_predecessor_node->production_rule->gene_string.last_gene  >= test_predecessor_node->production_rule->gene_string.first_gene)
                   )
                  overlapping=1;
              if (!overlapping)
                to_keep_predecessor_node=test_predecessor_node;
            }
          }
        if (to_keep_predecessor_node && !(to_keep_predecessor_node->production_rule->keep_rule)) //if a new rule to be kept
        { to_keep_predecessor_node->production_rule->keep_rule = definitely; //set to keep
          num_rules_after_filtering++;
          //no - this method checks for overlapping rules rather than clearing them:
          // clear_overlapping_rules(to_keep_rule);
          // int deleted_rule_number=0;
          // if (first_rule!=to_keep_rule)
          // { while (!(predecessor_nodes_array[deleted_rule_number]) || (predecessor_nodes_array[deleted_rule_number]->next!=to_keep_rule)) deleted_rule_number++;
          //   deleted_rule_number++;
          // }
          // for (;deleted_rule_number<to_keep_rule_rule_number;deleted_rule_number++)
          //   predecessor_nodes_array[deleted_rule_number]=0;
          // deleted_rule_number++; // skip over to_keep_rule
          // if (to_keep_rule!=last_rule)
          //   for (;predecessor_nodes_array[deleted_rule_number]!=to_keep_rule->next;deleted_rule_number++) predecessor_nodes_array[deleted_rule_number]=0;
          // else
          //   for (;deleted_rule_number<number_predecessors_read;deleted_rule_number++) predecessor_nodes_array[deleted_rule_number]=0;
          if (/* already know: first__kept_predecessor_node && */ (first__kept_predecessor_node->predecessor.first_gene < to_keep_predecessor_node->predecessor.first_gene))
          { Predecessor_node* previous_kept_predecessor_node;
            for (  previous_kept_predecessor_node = first__kept_predecessor_node
                 ; (previous_kept_predecessor_node->next_kept) && (previous_kept_predecessor_node->next_kept->predecessor.first_gene < to_keep_predecessor_node->predecessor.first_gene)
                 ; previous_kept_predecessor_node=previous_kept_predecessor_node->next_kept
                );
            to_keep_predecessor_node->next_kept = previous_kept_predecessor_node->next_kept;
            previous_kept_predecessor_node->next_kept = to_keep_predecessor_node;
          }
          else
          { to_keep_predecessor_node->next_kept = first__kept_predecessor_node;
            first__kept_predecessor_node = to_keep_predecessor_node;
          }
        }
      }
    }
  }
  //delete rules that will not be used and add the others (the rules to kept) to the list of kept rules:
  for (predecessor_node=first__possible_predecessor_node;predecessor_node;predecessor_node=predecessor_node->next)
  { if (predecessor_node->production_rule)
    { if (predecessor_node->production_rule->keep_rule) // then add the rule to the list of kept rules:
      { // if (predecessor_node->production_rule->keep_rule != definitely) {fprintf(stderr,"This should never happen\n"); exit(14);}
        //add rule:
        if (!first_rule) first_rule = last_rule = predecessor_node->production_rule;
        else
        { last_rule->next = predecessor_node->production_rule;
          last_rule = predecessor_node->production_rule;
        }
      }
      else delete predecessor_node->production_rule;    // else delete the rule
    }
  }
  for (predecessor_length=0;predecessor_length<=max_predecessor_length;predecessor_length++)
    if (number_predecessor_nodes_with_predecessor_length[predecessor_length]) delete [] predecessor_nodes_numbers[predecessor_length];
  delete [] predecessor_nodes_array;
  delete [] predecessor_nodes_numbers;
  delete [] number_predecessor_nodes_with_predecessor_length;
  // delete the predecessor_nodes:
  Predecessor_node *next_predecessor_node;
  for (predecessor_node=first__possible_predecessor_node;predecessor_node;predecessor_node=next_predecessor_node)
  { next_predecessor_node=predecessor_node->next;
    delete predecessor_node;
  }
}
#endif  //dont_use_contexts__max_optimised_filtered_rules_generation

void Ann::try_to_read_rule(Gene *from_gene)
{ if (!(*from_gene && *(from_gene+1))) return; // check is start of a possible rule
  from_gene += 2;
  Production_rule *new_rule = new Production_rule;
  new_rule->gene_string.first_gene = from_gene; //for checking for overlapping rules
#ifndef dont_use_contexts
  new_rule->left_segment.first_gene = from_gene;
  Gene_string segment;
  //left context:
  segment = genotype->get_segment(from_gene);
  if (segment.length() >= 2)
  { new_rule->left_context_tory = *(segment.first_gene + 1);
    (new_rule->left_context).first_gene  = segment.first_gene + 2;
    (new_rule->left_context).last_gene   = segment.last_gene;//may have 0 length
  }
  else
// #endif  //dont_use_contexts
    (new_rule->left_context).first_gene =(new_rule->left_context).last_gene  =0;
// #ifndef dont_use_contexts
  from_gene = segment.last_gene + 2;
#endif  //dont_use_contexts
  //predecessor:
  new_rule->predecessor = genotype->get_segment(from_gene);
  from_gene = (new_rule->predecessor).last_gene + 2;
#ifndef dont_use_contexts
  //right context:
  segment = genotype->get_segment(from_gene);
  new_rule->left_segment.last_gene = segment.last_gene;;
  if (segment.length() >= 2)
  { new_rule->right_context_tory = *(segment.first_gene + 1);
    (new_rule->right_context).first_gene = segment.first_gene + 2;
    (new_rule->right_context).last_gene  = segment.last_gene;//may have 0 length
  }
  else
// #endif  //dont_use_contexts
    (new_rule->right_context).first_gene=(new_rule->right_context).last_gene =0;
// #ifndef dont_use_contexts
  from_gene = segment.last_gene + 2;
#endif  //dont_use_contexts
  //successors:
  new_rule->successor_replacement = genotype->get_segment(from_gene);
  from_gene = (new_rule->successor_replacement).last_gene + 2;
  if (new_rule->successor_replacement.length() == 0)
    new_rule->successor_replacement.first_gene =
    new_rule->successor_replacement.last_gene  = 0;
  new_rule->successor_new         = genotype->get_segment(from_gene);
  from_gene = (new_rule->successor_new        ).last_gene + 2;
  if (new_rule->successor_new.length() == 0)
    new_rule->successor_new.first_gene =
    new_rule->successor_new.last_gene  = 0;
  //check have a full rule:
  if ((from_gene + 5) > genotype->last_gene) // if not enough genes for a rule
  { delete new_rule;
    return;
  }                                         // else:
//#ifndef dont_use_contexts
//#ifndef      use_contexts__use_read_rule_filter
//#else   //     use_contexts__use_read_rule_filter
//  for (Production_rule *compare_rule=first_rule;compare_rule;compare_rule=compare_rule->next)
//    if (new_rule->left_segment.is_equal_to(compare_rule->left_segment))
//    { delete new_rule;
//      return;
//    }
//#endif  //     use_contexts__use_read_rule_filter
//#else   //dont_use_contexts
//  //note: this bit speeds up the program a little
//  //      (and saves half the memory if using most_basic__filter_rules)
//  for (Production_rule *compare_rule=first_rule;compare_rule;compare_rule=compare_rule->next)
//    if (new_rule->predecessor.is_equal_to(compare_rule->predecessor))
//    { delete new_rule;
//      return;
//    }
//#endif  //dont_use_contexts
  //end bits:
  new_rule->reverse_inputs_type_on_new    = *from_gene; from_gene++;
  new_rule->reverse_outputs_type_on_new   = *from_gene; from_gene++;
  new_rule->inhibitory_replacement_to_new = *from_gene; from_gene++;
  new_rule->excitatory_replacement_to_new = *from_gene; from_gene++;
  new_rule->inhibitory_new_to_replacement = *from_gene; from_gene++;
  new_rule->excitatory_new_to_replacement = *from_gene;
  new_rule->gene_string.last_gene = from_gene; //for checking for overlapping rules
  //add rule:
  if (!first_rule) first_rule = last_rule = new_rule;
  else
  { last_rule->next = new_rule;
    last_rule = new_rule;
  }
  return;
}

void Ann::add_link_pair(Node *nodeA,Link *linksA,Node *nodeB,Link *linksB)
{ Link *linkA = linksA->new_link();
  Link *linkB = linksB->new_link();
  linkA->other_node = nodeB;
  linkA->other_nodes_link = linkB;
  linkB->other_node = nodeA;
  linkB->other_nodes_link = linkA;
}

void Ann::delete_link_pair(Link *link)
{ delete link->other_nodes_link;
  delete link;
}

void Ann::create_axiom_net()
{ // Set up 'characters' for Axiom nodes
#ifndef test__use_four_inputs
  axiom_input_node_character.create_as_character( "001");
  axiom_hidden_node_character.create_as_character("000");
  axiom_output_node_character.create_as_character("01");
#else   //test__use_four_inputs
  axiom_input_node_character.create_as_character( "0111");
  axiom_hidden_node_character.create_as_character("100");
  axiom_output_node_character.create_as_character("11");
#endif  //test__use_four_inputs
  // Set up Axiom network (i_node --excitatory--> h_node --ex.--> o_node)
  Node *i_node = nodes->new_node(); i_node->character = axiom_input_node_character;
  Node *h_node = nodes->new_node(); h_node->character = axiom_hidden_node_character;
  Node *o_node = nodes->new_node(); o_node->character = axiom_output_node_character;
  add_link_pair(i_node,i_node->excitatory_output_links, h_node,h_node->excitatory_input_links); // i_node -ex.-> h_node
  add_link_pair(h_node,h_node->excitatory_output_links, o_node,o_node->excitatory_input_links); // h_node -ex.-> o_node
  last_net_input = last_net_input->new_node_ptr();      // i_node is a
  i_node->net_input_node_ptr = last_net_input;          //   network input
  last_net_input->node = i_node;                        //   node
  last_net_output = last_net_output->new_node_ptr();    // o_node is a
  o_node->net_output_node_ptr = last_net_output;        //   network output
  last_net_output->node = o_node;                       //   node
  num_nodes=3;
}

void Ann::morphogenesis()
{ while (num_morphogenesis_steps_left)
    morphogenesis_step();
}

//NEW:
void Ann::morphogenesis_step()
{ if (!num_morphogenesis_steps_left) return;
  Node *node,*next_node;
  for (node=nodes->next;node;node=node->next) //not dummy
    node->just_constructed = 0;
  for (node=nodes->next;node;node=next_node) //not dummy
  { next_node = node->next; // before if block because run_rule may delete node
    if (!(node->just_constructed))
    { pick_update_rule_for_node(node);
      if (node->update_rule) run_rule(node->update_rule,node);
      if (num_nodes == max_num_nodes) break;
    }
  }
  if (num_nodes == max_num_nodes) num_morphogenesis_steps_left = 0;
  else num_morphogenesis_steps_left--;
}

//NEW vs. OLD: Because the new rule filtering of (overlapping) rules in the
//             context-sensitive case (mainly) takes place when rules are run,
//             rules are now picked just before use (might not exist later).  

//OLD:
//void Ann::morphogenesis_step()
//{ if (!num_morphogenesis_steps_left) return;
//  Node *node,*next_node;
//  Production_rule *rule;
//  for (node=nodes->next;node;node=node->next) //not dummy
//    pick_update_rule_for_node(node);
//  for (node=nodes->next;node;node=next_node) //not dummy
//  { next_node = node->next;
//    rule = node->update_rule;
//    if (rule) run_rule(rule,node);
//    if (num_nodes == max_num_nodes) break;
//  }
//  if (num_nodes == max_num_nodes) num_morphogenesis_steps_left = 0;
//  else num_morphogenesis_steps_left--;
//}

void Ann::pick_update_rule_for_node(Node *node)
{ node->update_rule = 0;
  int best__predecessor_length=-999;
#ifndef dont_use_contexts
  int best__left_context_length=-999,best__right_context_length=-999;
#endif  //dont_use_contexts
  for (Production_rule *rule=first_rule;rule;rule=rule->next)
  { if    (rule->predecessor.length() < best__predecessor_length) continue;
    if    (!(rule->predecessor.is_start_of(node->character)))     continue;
    if (   (   (rule->predecessor.length() > best__predecessor_length)
#ifndef dont_use_contexts
            || (   rule->left_context.length()  >  best__left_context_length
                && rule->right_context.length() >= best__right_context_length
               )
            || (   rule->left_context.length()  >= best__left_context_length
                && rule->right_context.length() >  best__right_context_length
               )
#endif  //dont_use_contexts
           )
#ifndef dont_use_contexts
        && node->context_match(1,rule->left_context_tory,rule->left_context)
        && node->context_match(0,rule->right_context_tory,rule->right_context)
#endif  //dont_use_contexts
       )
    { node->update_rule = rule;
      best__predecessor_length   = rule->predecessor.length();
#ifndef dont_use_contexts
      best__left_context_length  = rule->left_context.length();
      best__right_context_length = rule->right_context.length();
#endif  //dont_use_contexts
    }
  }
}

void Ann::run_rule(Production_rule *rule,Node *node)
{ Node *newnode;
  Link *link;
  if (rule->successor_new.first_gene)
  { newnode = node->new_node();
    num_nodes++;
    newnode->character = rule->successor_new;
#ifndef links_between_new_and_replacement_replace_inherited_I_and_or_O_links
#else   //links_between_new_and_replacement_replace_inherited_I_and_or_O_links
   if (   !(rule->successor_replacement.first_gene)
       || (   !(rule->inhibitory_replacement_to_new)
           && !(rule->excitatory_replacement_to_new)
          )
      )
   {
#endif  //links_between_new_and_replacement_replace_inherited_I_and_or_O_links
    if (rule->reverse_inputs_type_on_new) //note: dummy links not copied:
    { for(link=node->inhibitory_input_links->next;link;link=link->next)
        add_link_pair(newnode,newnode->excitatory_input_links,
        link->other_node,link->other_node->excitatory_output_links);
      for(link=node->excitatory_input_links->next;link;link=link->next)
        add_link_pair(newnode,newnode->inhibitory_input_links,
        link->other_node,link->other_node->inhibitory_output_links);
    }
    else
    { for(link=node->inhibitory_input_links->next;link;link=link->next)
        add_link_pair(newnode,newnode->inhibitory_input_links,
        link->other_node,link->other_node->inhibitory_output_links);
      for(link=node->excitatory_input_links->next;link;link=link->next)
        add_link_pair(newnode,newnode->excitatory_input_links,
        link->other_node,link->other_node->excitatory_output_links);
    }
#ifndef links_between_new_and_replacement_replace_inherited_I_and_or_O_links
#else   //links_between_new_and_replacement_replace_inherited_I_and_or_O_links
   }
   if (   !(rule->successor_replacement.first_gene)
       || (   !(rule->inhibitory_new_to_replacement)
           && !(rule->excitatory_new_to_replacement)
          )
      )
   {
#endif  //links_between_new_and_replacement_replace_inherited_I_and_or_O_links
    if (rule->reverse_outputs_type_on_new)
    { for(link=node->inhibitory_output_links->next;link;link=link->next)
        add_link_pair(newnode,newnode->excitatory_output_links,
        link->other_node,link->other_node->excitatory_input_links);
      for(link=node->excitatory_output_links->next;link;link=link->next)
        add_link_pair(newnode,newnode->inhibitory_output_links,
        link->other_node,link->other_node->inhibitory_input_links);
    }
    else
    { for(link=node->inhibitory_output_links->next;link;link=link->next)
        add_link_pair(newnode,newnode->inhibitory_output_links,
        link->other_node,link->other_node->inhibitory_input_links);
      for(link=node->excitatory_output_links->next;link;link=link->next)
        add_link_pair(newnode,newnode->excitatory_output_links,
        link->other_node,link->other_node->excitatory_input_links);
    }
#ifndef links_between_new_and_replacement_replace_inherited_I_and_or_O_links
#else   //links_between_new_and_replacement_replace_inherited_I_and_or_O_links
   }
#endif  //links_between_new_and_replacement_replace_inherited_I_and_or_O_links
  }
  Node_ptr *node__net_input_node_ptr = node->net_input_node_ptr;
  Node_ptr *node__net_output_node_ptr = node->net_output_node_ptr;
  if (rule->successor_replacement.first_gene)
  { node->character = rule->successor_replacement;
    if (rule->successor_new.first_gene)
    { if (rule->inhibitory_replacement_to_new)
        add_link_pair(   node,   node->inhibitory_output_links,
                      newnode,newnode->inhibitory_input_links);
      if (rule->excitatory_replacement_to_new)
        add_link_pair(   node,   node->excitatory_output_links,
                      newnode,newnode->excitatory_input_links);

      if (rule->inhibitory_new_to_replacement)
        add_link_pair(newnode,newnode->inhibitory_output_links,
                         node,   node->inhibitory_input_links);
      if (rule->excitatory_new_to_replacement)
        add_link_pair(newnode,newnode->excitatory_output_links,
                         node,   node->excitatory_input_links);
    }
  }
  else
  { //Shuffle down i_o_s:
    if (node->net_input_node_ptr)           //if node is a net input node
    { Node_ptr *node_ptr;
      for (node_ptr=net_inputs;                         //including dummy
           node_ptr->next != node->net_input_node_ptr;  //find one before
           node_ptr=node_ptr->next);
      node_ptr->next = node_ptr->next->next; //==node->net_intput_node_ptr->next
      if (node->net_input_node_ptr ==last_net_input) last_net_input = node_ptr;
      delete (node->net_input_node_ptr);
      node->net_input_node_ptr = 0;
    }
    if (node->net_output_node_ptr)           //if node is a net output node
    { Node_ptr *node_ptr;
      for (node_ptr=net_outputs;                        //including dummy
           node_ptr->next != node->net_output_node_ptr; //find one before
           node_ptr=node_ptr->next);
      node_ptr->next = node_ptr->next->next; //==node->net_output_node_ptr->next
      if (node->net_output_node_ptr ==last_net_output) last_net_output = node_ptr;
      delete (node->net_output_node_ptr);
      node->net_output_node_ptr = 0;
    }
    delete node; //no successor_replacement means delete predecessor
    num_nodes--;
  }
  if (rule->successor_new.first_gene)
  { Node_ptr *new_io_node_ptr;
    if (node__net_input_node_ptr)
    { new_io_node_ptr = last_net_input->new_node_ptr();  //then set as a
      new_io_node_ptr->node = newnode;                   //  network input node
      newnode->net_input_node_ptr = new_io_node_ptr;
      last_net_input = new_io_node_ptr;
    }
    if (node__net_output_node_ptr)
    { new_io_node_ptr = last_net_output->new_node_ptr(); //then set as a
      new_io_node_ptr->node = newnode;                   //  network output node
      newnode->net_output_node_ptr = new_io_node_ptr;
      last_net_output = new_io_node_ptr;
    }
  }
  if (rule->keep_rule == maybe)    //filtering out overlapping rules - see filter_rules()
  { rule->keep_rule = definitely;
    clear_overlapping_rules(rule);
  }
  rule->used = 1;
}

void Ann::delete_rules()
{ Production_rule *next_rule;
  for (Production_rule *rule=first_rule;rule;rule=next_rule)
  { next_rule=rule->next;
    delete rule;
  }
}

void Ann::update_outputs()
{ Node *node;
  for (node=nodes->next;node;node=node->next) //not dummy
    node->calc_new_outputs();
  for (node=nodes->next;node;node=node->next) //not dummy
  { node->inhibitory_output = node->new_inhibitory_output;
    node->excitatory_output = node->new_excitatory_output;
    if (node->net_output_node_ptr)
      node->net_output_node_ptr->io_value   = node->excitatory_output;
  }
}
