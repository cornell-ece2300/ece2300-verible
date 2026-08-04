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

#include "verible/verilog/analysis/checkers/forbid-module-instantiation-rule.h"

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

VERILOG_REGISTER_LINT_RULE(ForbidModuleInstantiationRule);

static constexpr std::string_view kMessage =
    "Module instantiation is not allowed. Build the module from" 
    "gate primitives (and, or, not, xor, nand, nor, xnor).";

const LintRuleDescriptor &ForbidModuleInstantiationRule::GetDescriptor() {
  static const LintRuleDescriptor d{
      .name = "forbid-module-instantiation",
      .topic = "gate-level-modeling",
      .desc = "Disallows instantiating a submodule; leaf gate-level modules "
              "must use gate primitives only.",
  };
  return d;
}

// Node @1 (tag: kModuleItemList) { --> the kModuleItemList can generate false flags
//       Node @0 (tag: kDataDeclaration) {
//         Node @1 (tag: kInstantiationBase) {
//           Node @0 (tag: kInstantiationType) {
//             Node @0 (tag: kDataType) {
//               Node @1 (tag: kLocalRoot) {
//                 Node @0 (tag: kUnqualifiedId) {
//                   Leaf @0 (#SymbolIdentifier @132-140: "DFFR_RTL")
//                 }
//               }
//               Node @3 (tag: kPackedDimensions) {
//               }
//             }
//           }
//           Node @1 (tag: kGateInstanceRegisterVariableList) {
//             Node @0 (tag: kGateInstance) { --> gate instance here refers to module, so this node will be matched
//               Leaf @0 (#SymbolIdentifier @141-145: "dffr")
//               Node @1 (tag: kUnpackedDimensions) {
//               }

void ForbidModuleInstantiationRule::HandleSymbol(
    const verible::Symbol &symbol, const SyntaxTreeContext &context) {
  // Only interested in nodes.
  if (symbol.Kind() != verible::SymbolKind::kNode) return;
  const verible::SyntaxTreeNode &node = verible::SymbolCastToNode(symbol);
  if (!node.MatchesTag(NodeEnum::kGateInstance)) return;

  // If it is a kGateInstance insert violation
  violations_.insert(LintViolation(node, kMessage, context));

}

LintRuleStatus ForbidModuleInstantiationRule::Report() const {
  return LintRuleStatus(violations_, GetDescriptor());
}

}  // namespace analysis
}  // namespace verilog
