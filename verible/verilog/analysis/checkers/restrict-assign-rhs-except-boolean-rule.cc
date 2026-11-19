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

#include "verible/verilog/analysis/checkers/restrict-assign-rhs-except-boolean-rule.h"

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

VERILOG_REGISTER_LINT_RULE(RestrictAssignRhsExceptBooleanRule);

static constexpr std::string_view kMessage =
    "Assignment RHS must be a signal, bit-select, part-select, literal, "
    "concatenation, replication, or a simple boolean expression.";

const LintRuleDescriptor &RestrictAssignRhsExceptBooleanRule::GetDescriptor() {
  static const LintRuleDescriptor d{
      .name = "restrict-assign-rhs-except-boolean",
      .topic = "structural-modeling",
      .desc = "Restricts the right-hand side of continuous assignments and net "
              "declaration assignments to wiring expressions and boolean "
              "operators (& | ^ ~^ ^~ && || ~ !, and reductions & | ^ ~& ~| "
              "~^).",
  };
  return d;
}

// This allowed part is same as original restrict-assign-rhs.
static bool IsAllowedRhsNode(NodeEnum tag) {
  switch (tag) {
    case NodeEnum::kExpression:
    case NodeEnum::kFunctionCall:  // Verible wraps plain identifiers in this
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
    case NodeEnum::kParenGroup:
      return true;
    default:
      return false;
  }
}

// a & b, a | b, a ^ b, a ~^ b, a && b, a || b.
static bool IsBooleanBinaryOperator(int token) {
  switch (token) {
    case '&':
    case '|':
    case '^':
    case verilog_tokentype::TK_NXOR:
    case verilog_tokentype::TK_LAND:
    case verilog_tokentype::TK_LOR:
      return true;
    default:
      return false;
  }
}

// ~a, !a, and the reductions &a, |a, ^a, ~&a, ~|a, ~^a (^~a also lexes as
// TK_NXOR). The only unary-prefix operators left out are + and -.
static bool IsBooleanUnaryOperator(int token) {
  switch (token) {
    case '~':
    case '!':
    case '&':
    case '|':
    case '^':
    case verilog_tokentype::TK_NAND:
    case verilog_tokentype::TK_NOR:
    case verilog_tokentype::TK_NXOR:
      return true;
    default:
      return false;
  }
}

// Returns true if node is a binary or unary-prefix expression whose
// operator(s) are all boolean. The operands are checked separately by the
// recursive walk in FindFirstDisallowedNode.
static bool IsBooleanOperatorNode(const verible::SyntaxTreeNode &node) {
  switch (static_cast<NodeEnum>(node.Tag().tag)) {
    case NodeEnum::kBinaryExpression: {
      // Verible flattens operators to one node (a & b & c -> [a, '&', b, '&', c])
      // so every direct leaf child is an
      // operator, and every one of them must be boolean.
      bool saw_operator = false;
      for (const auto &child : node.children()) {
        if (child == nullptr) continue;
        if (child->Kind() != verible::SymbolKind::kLeaf) continue;
        saw_operator = true;
        if (!IsBooleanBinaryOperator(child->Tag().tag)) return false;
      }
      return saw_operator;
    }
    case NodeEnum::kUnaryPrefixExpression: {
      if (node.empty()) return false;
      const verible::SyntaxTreeLeaf *op =
          verible::MaybeLeaf(node.front().get());
      return op != nullptr && IsBooleanUnaryOperator(op->Tag().tag);
    }
    default:
      return false;
  }
}

static const verible::SyntaxTreeNode *FindFirstDisallowedNode(
    const verible::Symbol &symbol) {
  if (symbol.Kind() != verible::SymbolKind::kNode) return nullptr;
  const verible::SyntaxTreeNode &node = verible::SymbolCastToNode(symbol);
  if (!IsAllowedRhsNode(static_cast<NodeEnum>(node.Tag().tag)) &&
      !IsBooleanOperatorNode(node)) {
    return &node;
  }

  for (const auto &child : node.children()) {
    if (child == nullptr) continue;
    if (const auto *found = FindFirstDisallowedNode(*child)) return found;
  }
  return nullptr;
}

void RestrictAssignRhsExceptBooleanRule::HandleSymbol(
    const verible::Symbol &symbol, const SyntaxTreeContext &context) {
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

LintRuleStatus RestrictAssignRhsExceptBooleanRule::Report() const {
  return LintRuleStatus(violations_, GetDescriptor());
}

}  // namespace analysis
}  // namespace verilog
