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

#include "verible/verilog/analysis/checkers/forbid-generic-always-rule.h"

#include <set>
#include <string_view>

#include "verible/common/analysis/lint-rule-status.h"
#include "verible/common/text/concrete-syntax-leaf.h"     // SyntaxTreeLeaf
#include "verible/common/text/concrete-syntax-tree.h"      // SyntaxTreeNode, SymbolCastToNode
#include "verible/common/text/symbol.h"
#include "verible/common/text/syntax-tree-context.h"
#include "verible/common/text/tree-utils.h"                // GetLeftmostLeaf
#include "verible/verilog/CST/verilog-nonterminals.h"      // NodeEnum
#include "verible/verilog/analysis/descriptions.h"
#include "verible/verilog/analysis/lint-rule-registry.h"
#include "verible/verilog/parser/verilog-token-enum.h"     // TK_always

namespace verilog {
namespace analysis {

using verible::LintRuleStatus;
using verible::LintViolation;
using verible::SyntaxTreeContext;

// Registers the rule so `verible-verilog-lint --rules=forbid-generic-always` works.
VERILOG_REGISTER_LINT_RULE(ForbidGenericAlwaysRule);

static constexpr std::string_view kMessage =
    "Generic 'always @(...)' block found. Use 'always_comb', 'always_ff', or "
    "'always_latch' instead.";

const LintRuleDescriptor &ForbidGenericAlwaysRule::GetDescriptor() {
  static const LintRuleDescriptor d{
      .name = "forbid-generic-always",
      .topic = "combinational-logic",
      .desc = "Disallows generic 'always @(...)' blocks; use always_comb, "
              "always_ff, or always_latch instead.",
  };
  return d;
}

void ForbidGenericAlwaysRule::HandleSymbol(const verible::Symbol &symbol,
                                           const SyntaxTreeContext &context) {
  
  // Skip if symbol isn't of type node during runtime
  if (symbol.Kind() != verible::SymbolKind::kNode) return;
    
  // Cast static type symbol to node after check
  const verible::SyntaxTreeNode &node = verible::SymbolCastToNode(symbol);
  if (!node.MatchesTag(NodeEnum::kAlwaysStatement)) return;

  // Leaf @0 of a kAlwaysStatement is the keyword:
  //   always / always_ff / always_comb / always_latch
  const verible::SyntaxTreeLeaf *keyword = verible::GetLeftmostLeaf(symbol);
  if (keyword == nullptr) return;

  if (keyword->get().token_enum() == TK_always) {
    // Point the diagnostic at the keyword token itself (gives file:line:col).
    violations_.insert(LintViolation(keyword->get(), kMessage, context));
  }
}

LintRuleStatus ForbidGenericAlwaysRule::Report() const {
  return LintRuleStatus(violations_, GetDescriptor());
}

}  // namespace analysis
}  // namespace verilog
