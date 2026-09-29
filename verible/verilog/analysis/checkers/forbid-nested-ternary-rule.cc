// Copyright 2026 The Verible Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "verible/verilog/analysis/checkers/forbid-nested-ternary-rule.h"

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

namespace verilog {
namespace analysis {

using verible::LintRuleStatus;
using verible::LintViolation;
using verible::SyntaxTreeContext;

VERILOG_REGISTER_LINT_RULE(ForbidNestedTernaryRule);

static constexpr std::string_view kMessage =
    "Nested ternary expressions (?:) are not allowed. Use a case statement "
    "instead.";

const LintRuleDescriptor &ForbidNestedTernaryRule::GetDescriptor() {
  static const LintRuleDescriptor d{
      .name = "forbid-nested-ternary",
      .topic = "modeling-style",
      .desc =
          "Disallows nested ternary expressions; allows standalone ternary "
          "expressions.",
  };
  return d;
}

void ForbidNestedTernaryRule::HandleSymbol(const verible::Symbol &symbol,
                                           const SyntaxTreeContext &context) {
  if (symbol.Kind() != verible::SymbolKind::kNode) return;
  const verible::SyntaxTreeNode &node = verible::SymbolCastToNode(symbol);
  if (!node.MatchesTag(NodeEnum::kConditionExpression)) return;

  // If current ternary is not inside another kConditionExpression, return
  if (!context.IsInside(NodeEnum::kConditionExpression)) return;

  // A ternary has five children: condition, ?, true value, :, false value
  const verible::SyntaxTreeLeaf *question =
      verible::GetSubtreeAsLeaf(node, NodeEnum::kConditionExpression, 1);
  if (question == nullptr) return;

  violations_.insert(LintViolation(question->get(), kMessage, context));
}

LintRuleStatus ForbidNestedTernaryRule::Report() const {
  return LintRuleStatus(violations_, GetDescriptor());
}

}  // namespace analysis
}  // namespace verilog
