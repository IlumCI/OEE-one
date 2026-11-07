// Port of Geb v08b3 ann_core: L-system rule decoding/filtering, development and
// the McCulloch-Pitts-style recurrent network. See docs/GEB_SPEC.md sections 3-4.
#pragma once

#include <list>
#include <memory>
#include <vector>

#include "geb/genes.hpp"
#include "geb/rng.hpp"

namespace oee::geb {

struct Rule {
  GeneSpan gene_string;  // predecessor start .. last link bit (the rulestat component)
  GeneSpan predecessor;
  GeneSpan successor_replacement;  // null if empty
  GeneSpan successor_new;          // null if empty
  Gene reverse_inputs_type_on_new = 0;
  Gene reverse_outputs_type_on_new = 0;
  Gene inhibitory_replacement_to_new = 0;
  Gene excitatory_replacement_to_new = 0;
  Gene inhibitory_new_to_replacement = 0;
  Gene excitatory_new_to_replacement = 0;
  bool used = false;
};

struct Node;

struct IoEntry {
  Node* node = nullptr;
  int io_value = 0;
};

struct Node {
  GeneSpan character;
  // Links are a multiset of neighbour pointers (duplicates count separately).
  std::vector<Node*> inhibitory_inputs, excitatory_inputs;
  std::vector<Node*> inhibitory_outputs, excitatory_outputs;
  IoEntry* net_input = nullptr;
  IoEntry* net_output = nullptr;
  int inhibitory_output = 0, excitatory_output = 0;
  int new_inhibitory_output = 0, new_excitatory_output = 0;
  bool just_constructed = true;
};

constexpr int kExcitatoryScaleShift = 10;
constexpr int kExcitatoryScale = 1 << kExcitatoryScaleShift;
constexpr int kMaxMorphogenesisSteps = 99;

struct NetConstants {
  int t_v = 500;
  int max_noise = 600;
};

// Genotype -> kept production rules (generate_rules, max-optimised variant).
// `axiom_characters` are the initial node characters in node-list order.
std::vector<Rule> generate_rules(const Genotype& genotype,
                                 const std::vector<GeneSpan>& axiom_characters);

class Ann {
 public:
  // Takes ownership of the genotype; builds the axiom network and the rule list.
  Ann(Genotype genotype, int max_num_nodes);
  Ann(const Ann&) = delete;
  Ann& operator=(const Ann&) = delete;

  const Genotype& genotype() const { return *genotype_; }
  const std::vector<Rule>& rules() const { return rules_; }
  std::list<IoEntry>& net_inputs() { return net_inputs_; }
  std::list<IoEntry>& net_outputs() { return net_outputs_; }
  const std::list<IoEntry>& net_outputs() const { return net_outputs_; }
  int num_nodes() const { return num_nodes_; }

  void morphogenesis_step();
  void update_outputs(Ran1& rng, const NetConstants& k);

 private:
  void create_axiom_net();
  Rule* pick_update_rule(const Node& node);
  void run_rule(Rule& rule, std::list<Node>::iterator node_it);
  void delete_node(std::list<Node>::iterator node_it);
  static void add_link(Node* from, Node* to, bool excitatory);
  static void calc_new_outputs(Node& node, Ran1& rng, const NetConstants& k);

  // Heap-held so spans into it stay valid if the Ann object is moved.
  std::unique_ptr<Genotype> genotype_;
  std::vector<Rule> rules_;
  std::list<Node> nodes_;
  std::list<IoEntry> net_inputs_, net_outputs_;
  int num_nodes_ = 0;
  int max_num_nodes_;
  int morphogenesis_steps_left_ = kMaxMorphogenesisSteps;
};

const std::vector<GeneSpan>& axiom_characters();

}  // namespace oee::geb
