/* Copyright (c) 2008 Alastair Channon
* 
* Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
* 
* The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

/* ann_core.h */
//usage note: the application that uses ann_core should use one of (in .cpp):
//                  a) #include "ann_core.h"
//                     const int max_noise = 0;
//                     const int t_v = _veto;        //_veto between 1 and excitatory_scale
//                  b) #include "ann_core.h"
//                     const int max_noise = _noise; //_noise between 1 and excitatory_scale
//                     const int t_v = _veto;        //_veto between 1 and excitatory_scale
//                     //in initialisation:
//                     randomize(); //should be called for random noise in nodes
#ifndef ann_core_h
#define ann_core_h
#include "rnd_core.h"

#define dont_use_contexts
#ifndef dont_use_contexts
#warning: Using contexts (that is okay - just an option)
#else   //dont_use_contexts
//#warning: Not using contexts (that is okay - just an option)
#endif  //dont_use_contexts

extern int max_num_nodes;
const int max_num_morphogenesis_steps = 99; //just in case of developmental loops

//GENES

//old (uses to much memory): enum Gene {allele0=0,allele1=1};
//new (on standard PC enum = int = 4bytes, short = 2bytes, char = 1byte);
typedef char Gene;
//old: const char allele0 = 0;
//old: const char allele1 = 1;

class Gene_string
{ public:    Gene_string() {} // keep the empty default constructor
             Gene_string(const char* asciibits) {create_as_character(asciibits);}
             Gene *first_gene;
             Gene *last_gene;
             int length() const;
             int is_equal_to(const Gene_string &compare_with) const;
             int is_start_of(const Gene_string &compare_with) const;
             void output_seconds_of_pairs();
             void create_as_character(const char* asciibits);
};

class Genotype : public Gene_string
{ public:    Gene_string get_segment(Gene *from_gene);
    virtual ~Genotype() {delete [] first_gene;}
};


//L-SYSTEMS

enum Keep_rule_status {no=0,maybe=1,definitely=2};

class Production_rule
{ public:    Production_rule() {next=0;used=0;}              // constructor
#ifndef dont_use_contexts
             Gene left_context_tory;
             Gene_string left_context;
#endif  //dont_use_contexts
             Gene_string predecessor;
#ifndef dont_use_contexts
             Gene right_context_tory;
             Gene_string right_context;
#endif  //dont_use_contexts
             Gene_string successor_replacement;
             Gene_string successor_new;
             Gene reverse_inputs_type_on_new;
             Gene reverse_outputs_type_on_new;
             Gene inhibitory_replacement_to_new;
             Gene excitatory_replacement_to_new;
             Gene inhibitory_new_to_replacement;
             Gene excitatory_new_to_replacement;
             void output_rule();
             Production_rule *next;
             Keep_rule_status keep_rule;
             int already_checked_successors_during_filtering;
             Gene_string gene_string;
             Gene_string left_segment;
             int used;
};


//ANNS

//Note: nodes contain lists of links; nets contain lists of nodes.
//      in each case, the first element in a list is a dummy
//      (this is so that the first real element can be easily removed)

class Node;  // forward declaration

class Link
{ public:    Link(Link *previous_link=0,Link *next_link=0);  // constructor
    virtual ~Link();                                         // destructor
             Link *new_link();
             Node *other_node;
             Link *other_nodes_link; // both nodes have a record of the link
             Link *previous;
             Link *next;
};

class Node_ptr // used to keep [simple] lists of net i/o nodes
{ public:    Node_ptr();                                     // constructor
             Node_ptr *new_node_ptr();
             Node *node;
             int io_value;
             Node_ptr *next;
};

class Node
{ public:    Node(Node *previous_node=0,Node *next_node=0);  // constructor
    virtual ~Node();                                         // destructor
             Node *new_node();
             Gene_string character;
             Link *inhibitory_input_links;
             Link *excitatory_input_links;
             Link *inhibitory_output_links;
             Link *excitatory_output_links;
             Node *previous;
             Node *next;
             Node_ptr *net_input_node_ptr;
             Node_ptr *net_output_node_ptr;
             int context_match(int left,Gene excitatory,Gene_string context);
             Production_rule *update_rule;
             //now in .cpp: int     inhibitory_input ,     excitatory_input ;
             int     inhibitory_output,     excitatory_output;
             int new_inhibitory_output, new_excitatory_output;
             void calc_new_outputs();
             int just_constructed; //for morphogenesis_step()
  protected: void delete_links(Link *first_link);
};

const int excitatory_scale_shift = 10;
const int excitatory_scale       = 1<<excitatory_scale_shift;
extern const int t_v;       // to be set in application that uses ann_core
extern const int max_noise; // to be set in application that uses ann_core

class Ann
{ public:    Ann(Genotype *with_genotype);                   // constructor
    virtual ~Ann();                                          // destructor
             Genotype *genotype;
             Production_rule *first_rule;
             void output_rules();
             Node *nodes;
             Node_ptr *net_inputs ,*last_net_input;
             Node_ptr *net_outputs,*last_net_output;
             void morphogenesis();
             void morphogenesis_step();
             void update_outputs();
             int node_num(Node *node);
             void output_node(Node *node);
             void output();
             int num_morphogenesis_steps_left;
  protected: void delete_nodes();
             void delete_node_ptrs(Node_ptr *node_ptrs);
             Production_rule *last_rule;
             void generate_rules();
             void try_to_read_rule(Gene *from_gene);
             void add_link_pair(Node *nodeA,Link *linksA,
                                Node *nodeB,Link *linksB);
             void delete_link_pair(Link *link);
             Gene_string axiom_input_node_character;
             Gene_string axiom_hidden_node_character;
             Gene_string axiom_output_node_character;
             void create_axiom_net();
             void pick_update_rule_for_node(Node *node);
             void run_rule(Production_rule *rule,Node *node);
             void delete_rules();
             int num_nodes;
             //int num_morphogenesis_steps_left; //now public - for geb_file
             void clear_overlapping_rules(Production_rule *use_rule);
             void most_basic__filter_rules();
             void filter_rules();
};

#endif
