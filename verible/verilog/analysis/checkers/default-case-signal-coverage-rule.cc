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

#include "verible/verilog/analysis/checkers/default-case-signal-coverage-rule.h"

#include <map>
#include <set>
#include <string_view>
#include <vector>

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

VERILOG_REGISTER_LINT_RULE(DefaultCaseSignalCoverageRule);

static constexpr std::string_view kMessage =
    "is assigned in a case item but not in the 'default' case. Assign it 'x "
    "in the default so an x selector propagates.";

const LintRuleDescriptor &DefaultCaseSignalCoverageRule::GetDescriptor() {
  static const LintRuleDescriptor d{
      .name = "default-case-signal-coverage",
      .topic = "x-propagation",
      .desc = "Checks that every signal assigned in a case item is also "
              "assigned in the `default` case.",
  };
  return d;
}

// Collect every assignment node under `symbol`
static void FindAssignments(
    const verible::Symbol &symbol,
    std::vector<const verible::SyntaxTreeNode *> *found) {
  if (symbol.Kind() != verible::SymbolKind::kNode) return;
  const verible::SyntaxTreeNode &node = verible::SymbolCastToNode(symbol);

  if (node.MatchesTag(NodeEnum::kNetVariableAssignment) ||
      node.MatchesTag(NodeEnum::kNonblockingAssignmentStatement)) {
    found->push_back(&node);
    return;
  }

  for (const auto &child : node.children()) {
    if (child != nullptr) FindAssignments(*child, found);
  }
}

// Return the name of the signal an assignment targets, or empty.
//
// The LHS is the assignment's kLPValue child, and the signal name is its
// leftmost leaf. Note that this makes the signal exploration signal level
// and not bit level:
//     y        -> "y"
//     v[3]     -> "v"
//     v[3:0]   -> "v"
static std::string_view AssignedSignalName(
    const verible::SyntaxTreeNode &assignment) {
  for (const auto &child : assignment.children()) {
    if (child == nullptr || child->Kind() != verible::SymbolKind::kNode) {
      continue;
    }
    const verible::SyntaxTreeNode &child_node =
        verible::SymbolCastToNode(*child);
    if (!child_node.MatchesTag(NodeEnum::kLPValue)) continue;

    const verible::SyntaxTreeLeaf *name = verible::GetLeftmostLeaf(child_node);
    if (name == nullptr) return {};
    return name->get().text();
  }
  return {};
}

//     kCaseStatement      we are matching the whole case statement block
//       kCaseItemList
//         kCaseItem       2'd0: begin y = a; z = a; end
//         kCaseItem       2'd1: begin y = a; w = a; end
//         kDefaultItem    default: begin y = 'x; z = 'x; end
//

void DefaultCaseSignalCoverageRule::HandleSymbol(
    const verible::Symbol &symbol, const SyntaxTreeContext &context) {
  // Only interested in nodes.
  if (symbol.Kind() != verible::SymbolKind::kNode) return;
  const verible::SyntaxTreeNode &node = verible::SymbolCastToNode(symbol);

  if (!node.MatchesTag(NodeEnum::kCaseStatement)) return;

  // Find the list holding the case items
  const verible::SyntaxTreeNode *item_list = nullptr;
  for (const auto &child : node.children()) {
    if (child == nullptr || child->Kind() != verible::SymbolKind::kNode) {
      continue;
    }
    const verible::SyntaxTreeNode &child_node =
        verible::SymbolCastToNode(*child);
    if (child_node.MatchesTag(NodeEnum::kCaseItemList)) {
      item_list = &child_node;
      break;
    }
  }
  if (item_list == nullptr) return;

  // Normal case items contribute signals to assigned; the default item
  // contributes the ones that are covered
  std::map<std::string_view, const verible::SyntaxTreeNode *> assigned;
  std::set<std::string_view> covered;

  for (const auto &child : item_list->children()) {
    if (child == nullptr || child->Kind() != verible::SymbolKind::kNode) {
      continue;
    }
    const verible::SyntaxTreeNode &item = verible::SymbolCastToNode(*child);

    const bool is_default = item.MatchesTag(NodeEnum::kDefaultItem);
    if (!is_default && !item.MatchesTag(NodeEnum::kCaseItem)) continue;

    std::vector<const verible::SyntaxTreeNode *> assignments;
    FindAssignments(item, &assignments);

    for (const verible::SyntaxTreeNode *assignment : assignments) {
      const std::string_view name = AssignedSignalName(*assignment);
      if (name.empty()) continue;

      if (is_default) {
        covered.insert(name);
      } else {
        // insert, instead of operator[], because it keeps the first assignment of 
        // a signal, so the violation points at the earliest place it appears.
        assigned.insert({name, assignment});
      }
    }
  }

  // Anything assigned in a case item but never in the default
  for (const auto &entry : assigned) {
    if (covered.find(entry.first) != covered.end()) continue;
    violations_.insert(LintViolation(
        *entry.second, absl::StrCat("Signal '", entry.first, "' ", kMessage),
        context));
  }
}

LintRuleStatus DefaultCaseSignalCoverageRule::Report() const {
  return LintRuleStatus(violations_, GetDescriptor());
}

}  // namespace analysis
}  // namespace verilog
