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

#include "verible/verilog/analysis/checkers/if-missing-final-else-rule.h"

#include <set>
#include <string_view>

#include "verible/common/analysis/lint-rule-status.h"
#include "verible/common/text/concrete-syntax-leaf.h"
#include "verible/common/text/concrete-syntax-tree.h"
#include "verible/common/text/symbol.h"
#include "verible/common/text/syntax-tree-context.h"
#include "verible/common/text/tree-utils.h"
#include "verible/verilog/CST/statement.h"
#include "verible/verilog/CST/verilog-nonterminals.h"
#include "verible/verilog/analysis/descriptions.h"
#include "verible/verilog/analysis/lint-rule-registry.h"
#include "verible/verilog/parser/verilog-token-enum.h"

namespace verilog {
namespace analysis {

using verible::LintRuleStatus;
using verible::LintViolation;
using verible::SyntaxTreeContext;
VERILOG_REGISTER_LINT_RULE(IfMissingFinalElseRule);

static constexpr std::string_view kMessage =
    "if/else chain is missing a final 'else' clause. Add a final else so an "
    "'x' condition falls through to a defined x-assignment.";

const LintRuleDescriptor &IfMissingFinalElseRule::GetDescriptor() {
  static const LintRuleDescriptor d{
      .name = "if-missing-final-else",
      .topic = "x-propagation",
      .desc = "Checks that every if/else chain terminates in a plain `else` "
              "clause.",
  };
  return d;
}



//   kConditionalStatement
//     kIfClause
//       kIfHeader                  <- the if's condition 
//       kIfBody
//     kElseClause                  <- missing when there is no "else" or "else if"
//       kElseBody
//         kConditionalStatement    <- present when node is an "else if"
//
// So if the tree doesn't have an kElseClause it means it neither has "else" or "else if"
// it's a direct violation. If tree has kElseClause, check if kElseBody has kConditionalStatement as an immediate child which
// signifies that it's an "else if" and not an "else" or an "else" with another nested "if".
//  If kElseClause doesn't have a grandchild kConditionalStatement, it's an "else" statement.

// -----------------------------------------------------------------------------
void IfMissingFinalElseRule::HandleSymbol(const verible::Symbol &symbol,
                                          const SyntaxTreeContext &context) {

  if (symbol.Kind() != verible::SymbolKind::kNode) return;
  const verible::SyntaxTreeNode &node = verible::SymbolCastToNode(symbol);
  if (!node.MatchesTag(NodeEnum::kConditionalStatement)) return;

  // Finds else clause of a conditional statement node
  const verible::SyntaxTreeNode *else_clause =
      GetConditionalStatementElseClause(symbol);
  // Return violation if no else clause exists, GetConditionalStatementElseClause
  // returns nullptr when none exist
  if (else_clause == nullptr) {
     violations_.insert(verible::LintViolation(symbol, kMessage, context));
     return;
  }
}

LintRuleStatus IfMissingFinalElseRule::Report() const {
  return LintRuleStatus(violations_, GetDescriptor());
}

}  // namespace analysis
}  // namespace verilog
