// Copyright 2017-2023 The Verible Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "verible/verilog/analysis/checkers/forbid-negedge-rule.h"

#include <set>
#include <string_view>
#include <vector>

#include "verible/common/analysis/lint-rule-status.h"
#include "verible/common/text/concrete-syntax-leaf.h"
#include "verible/common/text/concrete-syntax-tree.h"
#include "verible/common/text/symbol.h"
#include "verible/common/text/syntax-tree-context.h"
#include "verible/common/text/tree-utils.h"
#include "verible/verilog/CST/verilog-nonterminals.h"
#include "verible/verilog/analysis/descriptions.h"
#include "verible/verilog/analysis/lint-rule-registry.h"
#include "verible/verilog/parser/verilog-token-enum.h"

namespace verilog {
namespace analysis {

using verible::LintRuleStatus;
using verible::LintViolation;
using verible::SyntaxTreeContext;

// Register ForbidNegedgeRule so `verible-verilog-lint` can select it by name.
VERILOG_REGISTER_LINT_RULE(ForbidNegedgeRule);

static constexpr std::string_view kMessage =
    "Negative edge sensitivity ('negedge') in 'always_ff' block is not "
    "allowed. Only 'posedge clk' is permitted.";

const LintRuleDescriptor &ForbidNegedgeRule::GetDescriptor() {
  static const LintRuleDescriptor d{
      .name = "forbid-negedge",
      .topic = "sequential-logic",
      .desc = "Disallows `negedge` sensitivity in always_ff blocks; only "
              "`posedge` is permitted.",
  };
  return d;
}

// Recursively collect every `negedge` leaf under `symbol`.
static void FindNegedgeLeaves(
    const verible::Symbol &symbol,
    std::vector<const verible::SyntaxTreeLeaf *> *found) {
  // Base case: a leaf — check its token, then stop.
  if (symbol.Kind() == verible::SymbolKind::kLeaf) {
    const verible::SyntaxTreeLeaf &leaf = verible::SymbolCastToLeaf(symbol);
    if (leaf.get().token_enum() == TK_negedge) found->push_back(&leaf);
    return;
  }
  // Recursive case: a node — descend into each child.
  const verible::SyntaxTreeNode &node = verible::SymbolCastToNode(symbol);
  for (const auto &child : node.children()) {
    if (child != nullptr) FindNegedgeLeaves(*child, found);  // skip empty slots
  }
}

void ForbidNegedgeRule::HandleSymbol(const verible::Symbol &symbol,
                                     const SyntaxTreeContext &context) {
  // Only interested in kAlwaysStatement nodes.
  if (symbol.Kind() != verible::SymbolKind::kNode) return;
  const verible::SyntaxTreeNode &node = verible::SymbolCastToNode(symbol);
  if (!node.MatchesTag(NodeEnum::kAlwaysStatement)) return;

  // Leaf @0 of a kAlwaysStatement is the keyword:
  //   always / always_ff / always_comb / always_latch
  const verible::SyntaxTreeLeaf *keyword = verible::GetLeftmostLeaf(symbol);
  if (keyword == nullptr) return;

  if (keyword->get().token_enum() == TK_always_ff) {
    // negedge leaf vector 
    std::vector<const verible::SyntaxTreeLeaf *> negedges;
    FindNegedgeLeaves(symbol, &negedges);   // search always_ff's subtree for negedge
    for (const auto *neg : negedges) {
      violations_.insert(LintViolation(neg->get(), kMessage, context));
    }
  }
}

LintRuleStatus ForbidNegedgeRule::Report() const {
  return LintRuleStatus(violations_, GetDescriptor());
}

}  // namespace analysis
}  // namespace verilog
