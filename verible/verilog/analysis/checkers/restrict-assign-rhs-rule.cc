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

#include "verible/verilog/analysis/checkers/restrict-assign-rhs-rule.h"

#include <set>
#include <string_view>

#include "absl/strings/str_cat.h"
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

VERILOG_REGISTER_LINT_RULE(RestrictAssignRhsRule);

static constexpr std::string_view kMessage =
    "Assignment RHS must be a signal, bit-select, part-select, literal, "
    "concatenation, or replication. Build logic from gate primitives instead.";

const LintRuleDescriptor &RestrictAssignRhsRule::GetDescriptor() {
  static const LintRuleDescriptor d{
      .name = "restrict-assign-rhs",
      .topic = "gate-level-modeling",
      .desc = "Restricts the right-hand side of continuous assignments and net "
              "declaration assignments to wiring expressions.",
  };
  return d;
}

// See printtree for: assign y = a, c = b;
// kContinuousAssignmentStatement
//   Leaf @0  "assign"
//   kAssignmentList                       (can have multiple LHS, RHS pairs)
//     kNetVariableAssignment          
//       @0 kLPValue        "y"              
//       @1 Leaf  '='
//       @2 kExpression     "a"           
//     Leaf @1  ','
//     kNetVariableAssignment        
//       @0 kLPValue        "c"
//       @1 Leaf  '='
//       @2 kExpression     "b"

static bool IsAllowedRhsNode(NodeEnum tag) {
  switch (tag) {
    case NodeEnum::kExpression:
    case NodeEnum::kFunctionCall:
    case NodeEnum::kReference:
    case NodeEnum::kLocalRoot:
    case NodeEnum::kUnqualifiedId:
    case NodeEnum::kNumber:
    case NodeEnum::kBaseDigits:
    case NodeEnum::kDimensionScalar:
    case NodeEnum::kDimensionRange:
    case NodeEnum::kExpressionList:
    case NodeEnum::kConcatenationExpression:
    case NodeEnum::kOpenRangeList:
      return true;
    default:
      return false;
  }
}

static const verible::SyntaxTreeNode *FindFirstDisallowedNode(
    const verible::Symbol &symbol) {
  if (symbol.Kind() != verible::SymbolKind::kNode) return nullptr;
  const verible::SyntaxTreeNode &node = verible::SymbolCastToNode(symbol);
  if (!IsAllowedRhsNode(static_cast<NodeEnum>(node.Tag().tag))) return &node;

  for (const auto &child : node.children()) {
    if (child == nullptr) continue;
    if (const auto *found = FindFirstDisallowedNode(*child)) return found;
  }
  return nullptr;
}

void RestrictAssignRhsRule::HandleSymbol(const verible::Symbol &symbol,
                                         const SyntaxTreeContext &context) {
  if (symbol.Kind() != verible::SymbolKind::kNode) return;
  const verible::SyntaxTreeNode &node = verible::SymbolCastToNode(symbol);

  const NodeEnum assignment_tag = static_cast<NodeEnum>(node.Tag().tag);

  switch (assignment_tag) {
    case NodeEnum::kNetVariableAssignment:
      if (!context.IsInside(NodeEnum::kContinuousAssignmentStatement)) return;
      break;
    case NodeEnum::kNetDeclarationAssignment:
      break;
    default:
      return;
  }

  const verible::Symbol *rhs =
      verible::GetSubtreeAsSymbol(node, assignment_tag, 2);
  if (rhs == nullptr) return;
  const verible::SyntaxTreeNode *disallowed = FindFirstDisallowedNode(*rhs);
  if (disallowed == nullptr) return;
  violations_.insert(LintViolation(*disallowed, kMessage, context));
}

LintRuleStatus RestrictAssignRhsRule::Report() const {
  return LintRuleStatus(violations_, GetDescriptor());
}

}  // namespace analysis
}  // namespace verilog
