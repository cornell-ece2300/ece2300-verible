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

#include "verible/verilog/analysis/checkers/final-else-x-only-rule.h"

#include <set>
#include <string_view>
#include <vector>

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

VERILOG_REGISTER_LINT_RULE(FinalElseXOnlyRule);

static constexpr std::string_view kMessage =
    "Non-'x assignment in the final 'else' clause. The final else must assign "
    "only 'x (write: signal = 'x;).";

const LintRuleDescriptor &FinalElseXOnlyRule::GetDescriptor() {
  static const LintRuleDescriptor d{
      .name = "final-else-x-only",
      .topic = "x-propagation",
      .desc = "Checks that a final `else` clause assigns only `'x` values.",
  };
  return d;
}
// STEP 4 — flag each assignment whose RHS is not 'x. Anchor the violation on
//   the assignment node (or its LHS identifier) so the caret lands on the
//   offending statement rather than the whole else clause.
// -----------------------------------------------------------------------------

// Recursively collect every assignment node under 
static void FindAssignments(
    const verible::Symbol &symbol,
    std::vector<const verible::SyntaxTreeNode *> *found) {
  // Leaves can't be assignments and have no children -- dead end.
  if (symbol.Kind() != verible::SymbolKind::kNode) return;
  const verible::SyntaxTreeNode &node = verible::SymbolCastToNode(symbol);

  // Check if node is an assignment
  if (node.MatchesTag(NodeEnum::kNetVariableAssignment) ||
      node.MatchesTag(NodeEnum::kNonblockingAssignmentStatement)) {
    found->push_back(&node);
    return;  
  }

  // Recurse to children if not an assignment
  for (const auto &child : node.children()) {
    if (child != nullptr) FindAssignments(*child, found);
  }
}

// Check if found literal is an x-assignment
static bool IsXLiteral(const verible::SyntaxTreeNode &expression) {
  const verible::SyntaxTreeNode *number = verible::GetSubtreeAsNode(
      expression, NodeEnum::kExpression, 0);

  if (number == nullptr || !number->MatchesTag(NodeEnum::kNumber)) {
    return false;
  }

  const verible::SyntaxTreeLeaf *digits =
      verible::GetRightmostLeaf(*number);
  if (digits == nullptr) return false;

  const auto token =
      verilog_tokentype(digits->get().token_enum());
  const std::string_view text = digits->get().text();

  if (token == verilog_tokentype::TK_UnBasedNumber) {
    return text == "'x" || text == "'X";
  }

  if (token != verilog_tokentype::TK_BinDigits &&
      token != verilog_tokentype::TK_OctDigits &&
      token != verilog_tokentype::TK_HexDigits &&
      token != verilog_tokentype::TK_XZDigits) {
    return false;
  }

  bool found_x = false;
  for (const char digit : text) {
    if (digit == 'x' || digit == 'X') {
      found_x = true;
    } else if (digit != '_') {
      return false;
    }
  }

  return found_x;
}

void FinalElseXOnlyRule::HandleSymbol(const verible::Symbol &symbol,
                                      const SyntaxTreeContext &context) {
  // Only interested in kConditionalStatement nodes.
  if (symbol.Kind() != verible::SymbolKind::kNode) return;
  const verible::SyntaxTreeNode &node = verible::SymbolCastToNode(symbol);
  if (!node.MatchesTag(NodeEnum::kConditionalStatement)) return;

  // Get to final else
  const verible::SyntaxTreeNode *else_clause =
      GetConditionalStatementElseClause(symbol);
  if (else_clause == nullptr) return;      // no else, handled in R204
  const verible::SyntaxTreeNode *body =
      GetElseClauseStatementBody(*else_clause);
  if (body == nullptr) return; // return if no else
  if (body->MatchesTag(NodeEnum::kConditionalStatement)) return; // kConditionalStatement shows found else was an "else if"
  // non-returned kConditionalStatement is a final else node

  // Create assignment node vector and call recursive assignment finder
  std::vector<const verible::SyntaxTreeNode *> assignments;
  FindAssignments(*body, &assignments);

  // Loop through found assignments
  for (const verible::SyntaxTreeNode *assignment : assignments) {
  const verible::SyntaxTreeNode *expr = nullptr;

  // Find the RHS expression
  for (const auto &child : assignment->children()) {
    if (child == nullptr ||
        child->Kind() != verible::SymbolKind::kNode) {
      continue;
    }

    const verible::SyntaxTreeNode &child_node =
        verible::SymbolCastToNode(*child);

    if (child_node.MatchesTag(NodeEnum::kExpression)) {
      expr = &child_node;
      break;
    }
  }

  // Missing RHS or RHS is not entirely X
  if (expr == nullptr || !IsXLiteral(*expr)) {
    violations_.insert(
        LintViolation(*assignment, kMessage, context));
  }
}
  
}

LintRuleStatus FinalElseXOnlyRule::Report() const {
  return LintRuleStatus(violations_, GetDescriptor());
}

}  // namespace analysis
}  // namespace verilog
