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

#include "verible/verilog/analysis/checkers/forbid-async-reset-rule.h"

#include <set>
#include <string_view>

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

// Register ForbidAsyncResetRule so `verible-verilog-lint` can select it by name.
VERILOG_REGISTER_LINT_RULE(ForbidAsyncResetRule);

static constexpr std::string_view kMessage =
    "Asynchronous reset detected in 'always_ff' block. Only 'posedge clk' is "
    "allowed in the sensitivity list.";

const LintRuleDescriptor &ForbidAsyncResetRule::GetDescriptor() {
  static const LintRuleDescriptor d{
      .name = "forbid-async-reset",
      .topic = "sequential-logic",
      .desc = "Disallows asynchronous reset in always_ff blocks; the "
              "sensitivity list must contain only a single posedge clock edge.",
  };
  return d;
}

// Recursively counts the event expression nodes, which signify a sensitivity signal
// ex: two event expressions in: always_ff @(posedge clk or posedge rst)
// Heuristic here is that having more than one event expression would show
// async reset

static int CountEventExpressions(const verible::Symbol &symbol) {
  if (symbol.Kind() != verible::SymbolKind::kNode) return 0;
  const verible::SyntaxTreeNode &node = verible::SymbolCastToNode(symbol);
  int count = node.MatchesTag(NodeEnum::kEventExpression) ? 1 : 0;
  for (const auto &child : node.children()) {
    if (child != nullptr) count += CountEventExpressions(*child);
  }
  return count;
}

void ForbidAsyncResetRule::HandleSymbol(const verible::Symbol &symbol,
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
    if (CountEventExpressions(symbol) > 1) {
      violations_.insert(LintViolation(keyword->get(), kMessage, context));
    }
  }
}

LintRuleStatus ForbidAsyncResetRule::Report() const {
  return LintRuleStatus(violations_, GetDescriptor());
}

}  // namespace analysis
}  // namespace verilog
