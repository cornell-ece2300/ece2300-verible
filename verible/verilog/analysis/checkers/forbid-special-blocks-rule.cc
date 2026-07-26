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

#include "verible/verilog/analysis/checkers/forbid-special-blocks-rule.h"

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

VERILOG_REGISTER_LINT_RULE(ForbidSpecialBlocksRule);

static constexpr std::string_view kMessage =
    "RTL construct is not allowed in a gate-level/structural module. Use gate "
    "primitives and continuous assignments only.";

const LintRuleDescriptor &ForbidSpecialBlocksRule::GetDescriptor() {
  static const LintRuleDescriptor d{
      .name = "forbid-special-blocks",
      .topic = "gate-level-modeling",
      .desc = "Disallows always, initial, function, task, generate, and system "
              "task calls in gate-level and structural modules.",
  };
  return d;
}

// -----------------------------------------------------------------------------
// TAGS, verified with --printtree`:
//
//   always / always_comb / always_ff / always_latch  -> kAlwaysStatement
//   initial                                          -> kInitialStatement
//   function ... endfunction                         -> kFunctionDeclaration
//   task ... endtask                                 -> kTaskDeclaration
//   generate ... endgenerate                         -> kGenerateRegion
//   $info(...) / $error(...) as a module item        -> kSystemTFCall

// -----------------------------------------------------------------------------

static std::string_view ForbiddenConstructName(NodeEnum tag) {
  switch (tag) {
    case NodeEnum::kAlwaysStatement:     return "always";
    case NodeEnum::kInitialStatement:    return "initial";
    case NodeEnum::kFunctionDeclaration: return "function";
    case NodeEnum::kTaskDeclaration:     return "task";
    case NodeEnum::kGenerateRegion:      return "generate";
    case NodeEnum::kSystemTFCall:        return "system task call";
    default:                             return {};
  }
}

void ForbidSpecialBlocksRule::HandleSymbol(const verible::Symbol &symbol,
                                           const SyntaxTreeContext &context) {
  // Only interested in nodes.
  if (symbol.Kind() != verible::SymbolKind::kNode) return;
  const verible::SyntaxTreeNode &node = verible::SymbolCastToNode(symbol);

  // Match node tag type and get appopriate block name
  const std::string_view name =
      ForbiddenConstructName(static_cast<NodeEnum>(node.Tag().tag));
  // If node didn't match the disallowed constructs return
  if (name.empty()) return;
  violations_.insert(
      LintViolation(node, absl::StrCat("'", name, "' ", kMessage), context));
}

LintRuleStatus ForbidSpecialBlocksRule::Report() const {
  return LintRuleStatus(violations_, GetDescriptor());
}

}  // namespace analysis
}  // namespace verilog
