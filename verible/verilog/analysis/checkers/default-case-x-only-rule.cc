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

#include "verible/verilog/analysis/checkers/default-case-x-only-rule.h"

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

VERILOG_REGISTER_LINT_RULE(DefaultCaseXOnlyRule);

static constexpr std::string_view kMessage =
    "Non-'x assignment in the 'default' case. The default case must assign "
    "only 'x (write: signal = 'x;).";

const LintRuleDescriptor &DefaultCaseXOnlyRule::GetDescriptor() {
  static const LintRuleDescriptor d{
      .name = "default-case-x-only",
      .topic = "x-propagation",
      .desc = "Checks that a `default` case item assigns only `'x` values.",
  };
  return d;
}

// Collect every assignment node under `symbol`.
static void FindAssignments(
    const verible::Symbol &symbol,
    std::vector<const verible::SyntaxTreeNode *> *found) {
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

// True if the expression is entirely x: 'x, 1'bx, 4'bxxxx, 8'hxx ...
static bool IsXLiteral(const verible::SyntaxTreeNode &expression) {
  const verible::SyntaxTreeNode *number =
      verible::GetSubtreeAsNode(expression, NodeEnum::kExpression, 0);

  if (number == nullptr || !number->MatchesTag(NodeEnum::kNumber)) {
    return false;
  }

  const verible::SyntaxTreeLeaf *digits = verible::GetRightmostLeaf(*number);
  if (digits == nullptr) return false;

  const auto token = verilog_tokentype(digits->get().token_enum());
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

//     kCaseStatement
//       kCaseItemList
//         kCaseItem                        2'd0: y = a;
//         kCaseItem                        2'd1: y = b;
//         kDefaultItem                     we are macthing this tag
//           kSeqBlock                      (only when begin/end is used)
//             kBlockItemStatementList
//               kNetVariableAssignment     y = 'x     x nicely assigned
//               kNetVariableAssignment     z = 1'b0   violation
//

void DefaultCaseXOnlyRule::HandleSymbol(const verible::Symbol &symbol,
                                        const SyntaxTreeContext &context) {

  if (symbol.Kind() != verible::SymbolKind::kNode) return;
  const verible::SyntaxTreeNode &node = verible::SymbolCastToNode(symbol);

  if (!node.MatchesTag(NodeEnum::kDefaultItem)) return;

  // Collect every assignment in the default case. Recursion handles both
  // a one liner and a begin, end inside the default
  std::vector<const verible::SyntaxTreeNode *> assignments;
  FindAssignments(node, &assignments);

  for (const verible::SyntaxTreeNode *assignment : assignments) {
    // Find the RHS. Search by tag, not index: the expression sits at @2 in
    // kNetVariableAssignment but @3 in kNonblockingAssignmentStatement.
    const verible::SyntaxTreeNode *expr = nullptr;

    for (const auto &child : assignment->children()) {
      if (child == nullptr || child->Kind() != verible::SymbolKind::kNode) {
        continue;
      }

      const verible::SyntaxTreeNode &child_node =
          verible::SymbolCastToNode(*child);

      // Find the RHS node indicated by kExpression, this can change
      // positions depending on the code so this matching was necessary
      if (child_node.MatchesTag(NodeEnum::kExpression)) {
        expr = &child_node;
        break;
      }
    }

    // Missing RHS, or RHS is not entirely x is a violation
    if (expr == nullptr || !IsXLiteral(*expr)) {
      violations_.insert(LintViolation(*assignment, kMessage, context));
    }
  }
}

LintRuleStatus DefaultCaseXOnlyRule::Report() const {
  return LintRuleStatus(violations_, GetDescriptor());
}

}  // namespace analysis
}  // namespace verilog
