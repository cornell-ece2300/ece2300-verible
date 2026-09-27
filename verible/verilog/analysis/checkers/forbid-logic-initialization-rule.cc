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

#include "verible/verilog/analysis/checkers/forbid-logic-initialization-rule.h"

#include <string_view>

#include "verible/common/analysis/lint-rule-status.h"
#include "verible/common/text/concrete-syntax-leaf.h"
#include "verible/common/text/concrete-syntax-tree.h"
#include "verible/common/text/symbol.h"
#include "verible/common/text/syntax-tree-context.h"
#include "verible/common/text/tree-utils.h"
#include "verible/verilog/CST/declaration.h"
#include "verible/verilog/CST/verilog-nonterminals.h"
#include "verible/verilog/analysis/descriptions.h"
#include "verible/verilog/analysis/lint-rule-registry.h"
#include "verible/verilog/parser/verilog-token-enum.h"

namespace verilog {
namespace analysis {

using verible::LintRuleStatus;
using verible::LintViolation;
using verible::SyntaxTreeContext;

VERILOG_REGISTER_LINT_RULE(ForbidLogicInitializationRule);

static constexpr std::string_view kMessage =
    "Logic declaration initialization is not allowed. Declare the signal "
    "then assign separately.";

const LintRuleDescriptor &ForbidLogicInitializationRule::GetDescriptor() {
  static const LintRuleDescriptor d{
      .name = "forbid-logic-initialization",
      .topic = "modeling-style",
      .desc = "Disallows initializers in explicit logic variable declarations.",
  };
  return d;
}

void ForbidLogicInitializationRule::HandleSymbol(
    const verible::Symbol &symbol, const SyntaxTreeContext &context) {
  if (symbol.Kind() != verible::SymbolKind::kNode) return;
  const verible::SyntaxTreeNode &node = verible::SymbolCastToNode(symbol);
  if (!node.MatchesTag(NodeEnum::kDataDeclaration)) return;

  // Check the declared type, not logic tokens elsewhere in the declaration.
  const auto *type = GetInstantiationTypeOfDataDeclaration(node);
  if (type == nullptr) return;
  const auto *keyword = verible::GetLeftmostLeaf(*type);
  if (keyword == nullptr || keyword->get().token_enum() != TK_logic) return;

  // Each variable has its own optional initializer: logic a, b = 0, c = 1;
  const auto *variables = GetInstanceListFromDataDeclaration(node);
  if (variables == nullptr) return;
  for (const auto &child : variables->children()) {
    if (child == nullptr || child->Kind() != verible::SymbolKind::kNode)
      continue;
    const auto &variable = verible::SymbolCastToNode(*child);
    if (!variable.MatchesTag(NodeEnum::kRegisterVariable)) continue;

    const auto *initializer =
        verible::GetSubtreeAsNode(variable, NodeEnum::kRegisterVariable, 2);
    if (initializer == nullptr ||
        !initializer->MatchesTag(NodeEnum::kTrailingAssign))
      continue;
    const auto *equals =
        verible::GetSubtreeAsLeaf(*initializer, NodeEnum::kTrailingAssign, 0);
    if (equals == nullptr) continue;
    violations_.insert(LintViolation(equals->get(), kMessage, context));
  }
}

LintRuleStatus ForbidLogicInitializationRule::Report() const {
  return LintRuleStatus(violations_, GetDescriptor());
}

}  // namespace analysis
}  // namespace verilog
