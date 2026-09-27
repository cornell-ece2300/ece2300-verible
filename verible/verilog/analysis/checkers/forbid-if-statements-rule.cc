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

#include "verible/verilog/analysis/checkers/forbid-if-statements-rule.h"

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

VERILOG_REGISTER_LINT_RULE(ForbidIfStatementsRule);

static constexpr std::string_view kMessage =
    "If statements are not allowed. Use a case statement instead.";

const LintRuleDescriptor &ForbidIfStatementsRule::GetDescriptor() {
  static const LintRuleDescriptor d{
      .name = "forbid-if-statements",
      .topic = "modeling-style",
      .desc = "Disallows procedural and generate if statements.",
  };
  return d;
}

void ForbidIfStatementsRule::HandleSymbol(const verible::Symbol &symbol,
                                          const SyntaxTreeContext &context) {
  if (symbol.Kind() != verible::SymbolKind::kNode) return;
  const verible::SyntaxTreeNode &node = verible::SymbolCastToNode(symbol);
  const verible::SyntaxTreeLeaf *keyword = nullptr;
  if (node.MatchesTag(NodeEnum::kIfHeader)) {
    // Child 0 is an optional priority qualifier; child 1 is if
    keyword = verible::GetSubtreeAsLeaf(node, NodeEnum::kIfHeader, 1);
  } else if (node.MatchesTag(NodeEnum::kGenerateIf)) {
    keyword = verible::GetSubtreeAsLeaf(node, NodeEnum::kGenerateIf, 0);
  }
  if (keyword == nullptr) return;

  violations_.insert(LintViolation(keyword->get(), kMessage, context));
}

LintRuleStatus ForbidIfStatementsRule::Report() const {
  return LintRuleStatus(violations_, GetDescriptor());
}

}  // namespace analysis
}  // namespace verilog
