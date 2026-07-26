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

#include "verible/verilog/analysis/checkers/restrict-gate-primitives-rule.h"

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

VERILOG_REGISTER_LINT_RULE(RestrictGatePrimitivesRule);

static constexpr std::string_view kMessage =
    "gate primitive is not allowed. Use only: and, or, not, xor, nand, nor, "
    "xnor.";

const LintRuleDescriptor &RestrictGatePrimitivesRule::GetDescriptor() {
  static const LintRuleDescriptor d{
      .name = "restrict-gate-primitives",
      .topic = "gate-level-modeling",
      .desc = "Allows only the and, or, not, xor, nand, nor, and xnor gate "
              "primitives to be instantiated.",
  };
  return d;
}

// Returns true if name is one of the following seven gate primitives 
static bool IsAllowedGate(std::string_view name) {
  return name == "and"  || name == "or"  || name == "not" || name == "xor" ||
         name == "nand" || name == "nor" || name == "xnor";
}

void RestrictGatePrimitivesRule::HandleSymbol(const verible::Symbol &symbol,
                                              const SyntaxTreeContext &context) {
  // Only interested in nodes.
  if (symbol.Kind() != verible::SymbolKind::kNode) return;
  const verible::SyntaxTreeNode &node = verible::SymbolCastToNode(symbol);
  // kGateInstantiation is the primitive gate tag
  if (!node.MatchesTag(NodeEnum::kGateInstantiation)) return;        

  // kGateInstantiation
  //   Leaf @0  (#"and" @45-48: "and")     ← gate type
  //   kPrimitiveGateInstanceList          ← (y, a, b) part
  //   Leaf @2  (#';')

  const verible::SyntaxTreeLeaf *gate_type =
      verible::GetSubtreeAsLeaf(node, NodeEnum::kGateInstantiation, 0);

  // Raise violation for nullptr gate type pointer, all allowed gate types return a type
  if (gate_type == nullptr) {
    violations_.insert(
        LintViolation(node, kMessage, context));
    return;
  }

  // Return if gate type is in allowed gate list: and, or, not, xor, nand, nor, xnor
  const std::string_view name = gate_type->get().text();
  if (IsAllowedGate(name)) return;

  // If not in allowed list, give error message with the name of the disallowed primitive
  // gate type
  violations_.insert(LintViolation(
    gate_type->get(), absl::StrCat("'", name, "' ", kMessage), context));
}

LintRuleStatus RestrictGatePrimitivesRule::Report() const {
  return LintRuleStatus(violations_, GetDescriptor());
}

}  // namespace analysis
}  // namespace verilog
