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

#include "verible/verilog/analysis/checkers/forbid-nested-if-statements-rule.h"

#include <string_view>

#include "verible/common/analysis/lint-rule-status.h"
#include "verible/common/text/concrete-syntax-tree.h"
#include "verible/common/text/symbol.h"
#include "verible/common/text/syntax-tree-context.h"
#include "verible/verilog/CST/verilog-nonterminals.h"
#include "verible/verilog/analysis/descriptions.h"
#include "verible/verilog/analysis/lint-rule-registry.h"

namespace verilog {
namespace analysis {

using verible::LintRuleStatus;
using verible::LintViolation;
using verible::SyntaxTreeContext;

VERILOG_REGISTER_LINT_RULE(ForbidNestedIfStatementsRule);

static constexpr std::string_view kMessage =
    "Nested if statements are not allowed. Use an else-if chain instead.";

const LintRuleDescriptor &ForbidNestedIfStatementsRule::GetDescriptor() {
  static const LintRuleDescriptor d{
      .name = "forbid-nested-if-statements",
      .topic = "modeling-style",
      .desc =
          "Disallows nested procedural if statements; allows else-if chains.",
  };
  return d;
}

void ForbidNestedIfStatementsRule::HandleSymbol(
    const verible::Symbol &symbol, const SyntaxTreeContext &context) {
  if (symbol.Kind() != verible::SymbolKind::kNode) return;
  const auto &node = verible::SymbolCastToNode(symbol);
  if (!node.MatchesTag(NodeEnum::kConditionalStatement)) return;

  // Allow direct "else if" continuations
  if (context.DirectParentIs(NodeEnum::kElseBody)) return;

  // Allow a chain that is not nested inside another conditional
  if (!context.IsInside(NodeEnum::kConditionalStatement)) return;

  violations_.insert(LintViolation(node, kMessage, context));
}

LintRuleStatus ForbidNestedIfStatementsRule::Report() const {
  return LintRuleStatus(violations_, GetDescriptor());
}

}  // namespace analysis
}  // namespace verilog
