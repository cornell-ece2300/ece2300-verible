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

#include "verible/verilog/analysis/checkers/forbid-always-comb-rule.h"

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

VERILOG_REGISTER_LINT_RULE(ForbidAlwaysCombRule);

static constexpr std::string_view kMessage =
    "'always_comb' blocks are not allowed. Use continuous assignments instead.";

const LintRuleDescriptor &ForbidAlwaysCombRule::GetDescriptor() {
  static const LintRuleDescriptor d{
      .name = "forbid-always-comb",
      .topic = "combinational-logic",
      .desc =
          "Disallows always_comb blocks; use continuous assignments instead.",
  };
  return d;
}

void ForbidAlwaysCombRule::HandleSymbol(const verible::Symbol &symbol,
                                        const SyntaxTreeContext &context) {
  if (symbol.Kind() != verible::SymbolKind::kNode) return;
  const verible::SyntaxTreeNode &node = verible::SymbolCastToNode(symbol);
  if (!node.MatchesTag(NodeEnum::kAlwaysStatement)) return;

  const verible::SyntaxTreeLeaf *keyword = verible::GetLeftmostLeaf(symbol);
  if (keyword == nullptr) return;

  if (keyword->get().token_enum() == TK_always_comb) {
    violations_.insert(LintViolation(keyword->get(), kMessage, context));
  }
}

LintRuleStatus ForbidAlwaysCombRule::Report() const {
  return LintRuleStatus(violations_, GetDescriptor());
}

}  // namespace analysis
}  // namespace verilog
