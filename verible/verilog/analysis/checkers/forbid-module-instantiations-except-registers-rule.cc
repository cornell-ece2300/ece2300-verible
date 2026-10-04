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

#include "verible/verilog/analysis/checkers/forbid-module-instantiations-except-registers-rule.h"

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

namespace verilog {
namespace analysis {

using verible::LintRuleStatus;
using verible::LintViolation;
using verible::SyntaxTreeContext;

VERILOG_REGISTER_LINT_RULE(ForbidModuleInstantiationsExceptRegistersRule);

static constexpr std::string_view kMessage =
    "Module instantiation is not allowed here unless it is an approved "
    "register module.";

const LintRuleDescriptor &
ForbidModuleInstantiationsExceptRegistersRule::GetDescriptor() {
  static const LintRuleDescriptor d{
      .name = "forbid-module-instantiations-except-registers",
      .topic = "module-instantiation",
      .desc =
          "Allows only flip-flop and register module instantiations.",
  };
  return d;
}

static bool IsAllowedRegister(std::string_view name) {
  return name == "DFF_GL" || name == "DFF_RTL" || name == "DFFR_GL" ||
         name == "DFFR_RTL" || name == "DFFRE_GL" || name == "DFFRE_RTL" ||
         name == "Register_16b_GL" || name == "Register_16b_RTL" ||
         name == "RegfileZ2r1w_32x32b_RTL" || name == "Register_32b_RTL" ||
         name == "ShiftRegister_44b_RTL";
}

void ForbidModuleInstantiationsExceptRegistersRule::HandleSymbol(
    const verible::Symbol &symbol, const SyntaxTreeContext &context) {
  if (symbol.Kind() != verible::SymbolKind::kNode) return;
  const auto &node = verible::SymbolCastToNode(symbol);
  if (!node.MatchesTag(NodeEnum::kGateInstance)) return;

  // The instance name is in this node. The module type is in its enclosing
  // declaration: kDataDeclaration -> kInstantiationBase -> kInstantiationType.
  const auto *declaration = context.NearestParentWithTag(NodeEnum::kDataDeclaration);
  if (declaration != nullptr) {
    const auto *type = GetTypeIdentifierFromDataDeclaration(*declaration);
    if (type != nullptr &&
        type->Tag() == verible::NodeTag(NodeEnum::kUnqualifiedId)) {
      const auto *name = verible::GetLeftmostLeaf(*type);
      if (name != nullptr && IsAllowedRegister(name->get().text())) return;
    }
  }

  violations_.insert(LintViolation(node, kMessage, context));
}

LintRuleStatus ForbidModuleInstantiationsExceptRegistersRule::Report() const {
  return LintRuleStatus(violations_, GetDescriptor());
}

}  // namespace analysis
}  // namespace verilog
