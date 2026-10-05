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

#include "verible/verilog/analysis/checkers/forbid-one-bit-literal-ternary-rule.h"

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

VERILOG_REGISTER_LINT_RULE(ForbidOneBitLiteralTernaryRule);

static constexpr std::string_view kMessage =
    "Ternary expressions with one-bit literals in both result branches "
    "are not allowed.";

const LintRuleDescriptor &ForbidOneBitLiteralTernaryRule::GetDescriptor() {
  static const LintRuleDescriptor d{
      .name = "forbid-one-bit-literal-ternary",
      .topic = "modeling-style",
      .desc =
          "Disallows ternaries whose two result branches are explicitly sized "
          "one-bit literals.",
  };
  return d;
}

// Accept zero or one with optional leading zeros and underscores.
// Other digits, or a digit after a nonzero digit, mean a different value.
static int ZeroOrOneValue(std::string_view digits) {
  int value = 0;
  bool found_digit = false;
  for (char digit : digits) {
    if (digit == '_') continue;
    if (value != 0 || (digit != '0' && digit != '1')) return -1;
    value = digit - '0';
    found_digit = true;
  }
  return found_digit ? value : -1;
}

static bool IsOneBitLiteral(const verible::Symbol *symbol) {
  if (symbol == nullptr || symbol->Kind() != verible::SymbolKind::kNode) {
    return false;
  }
  const auto &node = verible::SymbolCastToNode(*symbol);

  if (node.MatchesTag(NodeEnum::kExpression) && node.size() == 1) {
    return IsOneBitLiteral(node.front().get());
  }
  if (node.MatchesTag(NodeEnum::kParenGroup)) {
    return IsOneBitLiteral(
        verible::GetSubtreeAsSymbol(node, NodeEnum::kParenGroup, 1));
  }
  if (!node.MatchesTag(NodeEnum::kNumber) || node.size() != 2) return false;

  // A sized number consists of its width followed by a base-and-digits node.
  const auto *width = verible::GetSubtreeAsLeaf(node, NodeEnum::kNumber, 0);
  const auto *based = verible::GetSubtreeAsNode(node, NodeEnum::kNumber, 1);
  if (width == nullptr || ZeroOrOneValue(width->get().text()) != 1 ||
      based == nullptr || !based->MatchesTag(NodeEnum::kBaseDigits)) {
    return false;
  }
  const auto *digits =
      verible::GetSubtreeAsLeaf(*based, NodeEnum::kBaseDigits, 1);
  return digits != nullptr && ZeroOrOneValue(digits->get().text()) >= 0;
}

void ForbidOneBitLiteralTernaryRule::HandleSymbol(
    const verible::Symbol &symbol, const SyntaxTreeContext &context) {
  if (symbol.Kind() != verible::SymbolKind::kNode) return;
  const verible::SyntaxTreeNode &node = verible::SymbolCastToNode(symbol);
  if (!node.MatchesTag(NodeEnum::kConditionExpression)) return;

  // A ternary has five children: condition, ?, true value, :, false value
  const auto *true_value =
      verible::GetSubtreeAsSymbol(node, NodeEnum::kConditionExpression, 2);
  const auto *false_value =
      verible::GetSubtreeAsSymbol(node, NodeEnum::kConditionExpression, 4);
  if (!IsOneBitLiteral(true_value) || !IsOneBitLiteral(false_value)) return;

  const verible::SyntaxTreeLeaf *question =
      verible::GetSubtreeAsLeaf(node, NodeEnum::kConditionExpression, 1);
  if (question == nullptr) return;

  violations_.insert(LintViolation(question->get(), kMessage, context));
}

LintRuleStatus ForbidOneBitLiteralTernaryRule::Report() const {
  return LintRuleStatus(violations_, GetDescriptor());
}

}  // namespace analysis
}  // namespace verilog
