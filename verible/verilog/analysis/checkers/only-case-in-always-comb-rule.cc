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

#include "verible/verilog/analysis/checkers/only-case-in-always-comb-rule.h"

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

VERILOG_REGISTER_LINT_RULE(OnlyCaseInAlwaysCombRule);

static constexpr std::string_view kMessage =
    "An always_comb body can only have a case statement.";

const LintRuleDescriptor &OnlyCaseInAlwaysCombRule::GetDescriptor() {
  static const LintRuleDescriptor d{
      .name = "only-case-in-always-comb",
      .topic = "combinational-logic",
      .desc =
          "Requires each always_comb to only have a case statement.",
  };
  return d;
}

void OnlyCaseInAlwaysCombRule::HandleSymbol(const verible::Symbol &symbol,
                                            const SyntaxTreeContext &context) {
  if (symbol.Kind() != verible::SymbolKind::kNode) return;
  const auto &node = verible::SymbolCastToNode(symbol);
  if (!node.MatchesTag(NodeEnum::kAlwaysStatement)) return;

  const auto *keyword =
      verible::GetSubtreeAsLeaf(node, NodeEnum::kAlwaysStatement, 0);
  if (keyword == nullptr || keyword->get().token_enum() != TK_always_comb)
    return;

  const auto *body = verible::GetSubtreeAsNode(node, NodeEnum::kAlwaysStatement, 1);
  if (body == nullptr) return;

  // Without begin/end, the case statement is the body itself:
  // kAlwaysStatement
  // |--- child 0: "always_comb"
  // |--- child 1: kCaseStatement

  if (body->MatchesTag(NodeEnum::kCaseStatement)) return;
  if (body->MatchesTag(NodeEnum::kSeqBlock)) {
    // Count only the immediate statements between begin and end:
    //   kSeqBlock
    // |--- child 0: kBegin
    // |--- child 1: kBlockItemStatementList
    // │  |--- child 0: kCaseStatement
    // |--- child 2: kEnd

    const auto *statements = verible::GetSubtreeAsNode(*body, NodeEnum::kSeqBlock, 
                                                        1,     NodeEnum::kBlockItemStatementList);
    
    if (statements == nullptr) return;
    if (statements->empty())   return;

    if (statements->size() == 1) {
      const auto *statement = verible::GetSubtreeAsNode(*statements, NodeEnum::kBlockItemStatementList, 0);
      if (statement != nullptr && statement->MatchesTag(NodeEnum::kCaseStatement)) {
        return;
      }
    }
  }


  violations_.insert(LintViolation(node, kMessage, context));
}

LintRuleStatus OnlyCaseInAlwaysCombRule::Report() const {
  return LintRuleStatus(violations_, GetDescriptor());
}

}  // namespace analysis
}  // namespace verilog
