// Copyright 2017-2023 The Verible Authors.
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

#include "verible/verilog/analysis/checkers/if-x-assignment-rule.h"

#include <map>
#include <set>
#include <string_view>
#include <vector>

#include "verible/common/analysis/lint-rule-status.h"
#include "verible/common/text/concrete-syntax-leaf.h"
#include "verible/common/text/concrete-syntax-tree.h"
#include "verible/common/text/symbol.h"
#include "verible/common/text/syntax-tree-context.h"
#include "verible/common/text/tree-utils.h"
#include "verible/verilog/CST/statement.h"
#include "verible/verilog/CST/verilog-nonterminals.h"
#include "verible/verilog/analysis/descriptions.h"
#include "verible/verilog/analysis/lint-rule-registry.h"

namespace verilog {
namespace analysis {

using verible::LintRuleStatus;
using verible::SyntaxTreeContext;

VERILOG_REGISTER_LINT_RULE(IfXAssignmentRule);

static constexpr std::string_view kMessage =
    "Signal assigned in the if/else chain is missing from the final 'else' "
    "clause. Assign this signal to 'x in the final else.";

const LintRuleDescriptor &IfXAssignmentRule::GetDescriptor() {
  static const LintRuleDescriptor d{
      .name = "if-x-assignment",
      .topic = "x-propagation",
      .desc = "Checks that every signal assigned in an if/else chain is also "
              "assigned in its final `else` clause.",
  };
  return d;
}

// Recursively collect every assignment node under 
static void FindAssignments(
    const verible::Symbol &symbol,
    std::vector<const verible::SyntaxTreeNode *> *found) {
  // Leaves can't be assignments and have no children -- dead end.
  if (symbol.Kind() != verible::SymbolKind::kNode) return;
  const verible::SyntaxTreeNode &node = verible::SymbolCastToNode(symbol);

  // Check if node is an assignment
  if (node.MatchesTag(NodeEnum::kNetVariableAssignment) ||
      node.MatchesTag(NodeEnum::kNonblockingAssignmentStatement)) {
    found->push_back(&node);
    return;  
  }

  // Recurse to children if not an assignment
  for (const auto &child : node.children()) {
    if (child != nullptr) FindAssignments(*child, found);
  }
}

// Returns assigned signal's name in an assignment
static std::string_view GetAssignedSignalName(
    const verible::SyntaxTreeNode &assignment) {
  for (const auto &child : assignment.children()) {
    if (child == nullptr ||
        child->Kind() != verible::SymbolKind::kNode) {
      continue;
    }

    const auto &child_node = verible::SymbolCastToNode(*child);
    if (!child_node.MatchesTag(NodeEnum::kLPValue)) continue;

    // Signal name lies in the leftmost leaf
    const verible::SyntaxTreeLeaf *signal =
        verible::GetLeftmostLeaf(child_node);

    if (signal != nullptr) return signal->get().text();
  }

  return {};
}

void IfXAssignmentRule::HandleSymbol(const verible::Symbol &symbol,
                                     const SyntaxTreeContext &context) {
  
  if (symbol.Kind() != verible::SymbolKind::kNode) return;
  const auto &node = verible::SymbolCastToNode(symbol);
  if (!node.MatchesTag(NodeEnum::kConditionalStatement)) return;
  // Do not process if (kConditionalStatement) under an else if of the root kConditionalStatement
  if (context.DirectParentIs(NodeEnum::kElseBody)) return;

  // Follow the else-if chain to its final else body
  const verible::SyntaxTreeNode *current = &node;
  const verible::SyntaxTreeNode *final_else_body = nullptr;

  while (true) {
    const verible::SyntaxTreeNode *else_clause =
        GetConditionalStatementElseClause(*current);
    if (else_clause == nullptr) return;  // R204 handles it

    const verible::SyntaxTreeNode *body =
        GetElseClauseStatementBody(*else_clause);
    if (body == nullptr) return;

    if (!body->MatchesTag(NodeEnum::kConditionalStatement)) {
      final_else_body = body;
      break;
    }

    current = body;
  }

  std::vector<const verible::SyntaxTreeNode *> chain_assignments;
  std::vector<const verible::SyntaxTreeNode *> final_else_assignments;

  // Node from the root gets all assignments including in the final else
  FindAssignments(node, &chain_assignments);
  // Only gets assignments in the final else
  FindAssignments(*final_else_body, &final_else_assignments);

  std::map<std::string_view, const verible::SyntaxTreeNode *> chain_signals;
  std::set<std::string_view> final_else_signals;

  // Compare the assigned signal names
  for (const verible::SyntaxTreeNode *assignment : chain_assignments) {
    const std::string_view name = GetAssignedSignalName(*assignment);
    if (!name.empty()) {
      chain_signals.emplace(name, assignment);
    }
  }

  for (const verible::SyntaxTreeNode *assignment : final_else_assignments) {
    const std::string_view name = GetAssignedSignalName(*assignment);
    if (!name.empty()) {
      final_else_signals.insert(name);
    }
  }

  for (const auto &[name, assignment] : chain_signals) {
    if (final_else_signals.find(name) == final_else_signals.end()) {
      violations_.insert(
          verible::LintViolation(*assignment, kMessage, context));
    }
  }
  
}

LintRuleStatus IfXAssignmentRule::Report() const {
  return LintRuleStatus(violations_, GetDescriptor());
}

}  // namespace analysis
}  // namespace verilog
