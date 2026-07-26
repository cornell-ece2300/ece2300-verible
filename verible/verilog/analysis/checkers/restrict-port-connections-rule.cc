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

#include "verible/verilog/analysis/checkers/restrict-port-connections-rule.h"

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

VERILOG_REGISTER_LINT_RULE(RestrictPortConnectionsRule);

static constexpr std::string_view kMessage =
    "port connection contains an operation. Connect a wire/net instead -- a "
    "signal, bit-select, part-select, or concatenation. Put the logic in a "
    "gate primitive.";

const LintRuleDescriptor &RestrictPortConnectionsRule::GetDescriptor() {
  static const LintRuleDescriptor d{
      .name = "restrict-port-connections",
      .topic = "gate-level-modeling",
      .desc = "Disallows operators inside gate or module instance port "
              "connections; concatenations are allowed.",
  };
  return d;
}

//
//   and( y, a, b );                   Foo_GL u1 ( .in(a) );
//   -----------------------           ---------------------------
//   kGateInstantiation                kGateInstance
//     kPrimitiveGateInstance            kParenGroup
//       kParenGroup                       kPortActualList
//         kPortActualList                   kActualNamedPort
//           kActualPositionalPort             Leaf @1  "in"   <- port name
//             kExpression   <- @0             kParenGroup
//               ...                             kExpression   <- expression is one level down
//                                                 ...
//
//   port expression        child of its kExpression      verdict
//   ---------------        ------------------------      -------
//   a                      kFunctionCall                 ALLOW
//   a[3]                   kFunctionCall                 ALLOW
//   a[3:2]                 kFunctionCall                 ALLOW
//   {a, b}                 kConcatenationExpression      ALLOW
//   ~a                     kUnaryPrefixExpression        REJECT
//   a & b                  kBinaryExpression             REJECT
//   sel ? a : b            kConditionExpression          REJECT
//
// so:  and( y, a, b );          no violation
//      and( y, ~a, b );         1  violation
//      Foo_GL u ( .in(a & b) ); 1  violation
//

// Returns true if given tag is a node that does an operation
static bool IsOperatorNode(NodeEnum tag) {
  switch (tag) {
    case NodeEnum::kBinaryExpression:       // a & b, a + b, a ^ b
    case NodeEnum::kUnaryPrefixExpression:  // ~a, !a, -a
    case NodeEnum::kConditionExpression:    // sel ? a : b
    case NodeEnum::kReferenceCallBase:      // f(a)
      return true;
    default:
      return false;
  }
}

// Returns the first operator node under symbol
static const verible::SyntaxTreeNode *FindFirstOperator(
    const verible::Symbol &symbol) {
  if (symbol.Kind() != verible::SymbolKind::kNode) return nullptr;
  const verible::SyntaxTreeNode &node = verible::SymbolCastToNode(symbol);
  if (IsOperatorNode(static_cast<NodeEnum>(node.Tag().tag))) return &node;

  for (const auto &child : node.children()) {
    if (child == nullptr) continue;
    if (const auto *found = FindFirstOperator(*child)) return found;
  }
  return nullptr;
}

void RestrictPortConnectionsRule::HandleSymbol(
    const verible::Symbol &symbol, const SyntaxTreeContext &context) {
  // Only interested in nodes.
  if (symbol.Kind() != verible::SymbolKind::kNode) return;
  const verible::SyntaxTreeNode &node = verible::SymbolCastToNode(symbol);

  // Positional ports come from gate primitives, named ports from module
  // instances in which both are checked the same way: recurse the port to match an operator node
  if (!node.MatchesTag(NodeEnum::kActualPositionalPort) &&
      !node.MatchesTag(NodeEnum::kActualNamedPort)) {
    return;
  }

  const verible::SyntaxTreeNode *op = FindFirstOperator(node);
  if (op == nullptr) return;
  violations_.insert(LintViolation(*op, kMessage, context));
}

LintRuleStatus RestrictPortConnectionsRule::Report() const {
  return LintRuleStatus(violations_, GetDescriptor());
}

}  // namespace analysis
}  // namespace verilog
