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

#include "verible/verilog/analysis/checkers/forbid-negedge-rule.h"

#include <initializer_list>

#include "gtest/gtest.h"
#include "verible/common/analysis/linter-test-utils.h"
#include "verible/common/analysis/syntax-tree-linter-test-utils.h"
#include "verible/verilog/analysis/verilog-analyzer.h"
#include "verible/verilog/parser/verilog-token-enum.h"

namespace verilog {
namespace analysis {
namespace {

using verible::LintTestCase;
using verible::RunLintTestCases;

TEST(ForbidNegedgeRuleTests, Various) {
  // The violation is anchored on the `negedge` keyword token.
  constexpr int kToken = TK_negedge;
  const std::initializer_list<LintTestCase> kTestCases = {
      // --- No violations ---
      {""},
      {"module m; endmodule"},
      // posedge is the allowed edge.
      {"module m; logic q, d, clk; "
       "always_ff @(posedge clk) q <= d; endmodule"},
      // No event control at all.
      {"module m; logic y, a, b; always_comb y = a & b; endmodule"},
      // negedge in a *generic* always is out of scope for this rule
      // (that should by only caught by forbid-generic-always instead).
      {"module m; logic q, d, clk; "
       "always @(negedge clk) q <= d; endmodule"},

      // --- Violations: negedge inside always_ff ---
      {"module m; logic q, d, clk; always_ff @(",
       {kToken, "negedge"},
       " clk) q <= d; endmodule"},

      // Mixed sensitivity: only the negedge term is flagged.
      {"module m; logic q, d, clk, rst; always_ff @(posedge clk or ",
       {kToken, "negedge"},
       " rst) q <= d; endmodule"},

      // Two negedge terms -> two violations.
      {"module m; logic q, d, clk, rst; always_ff @(",
       {kToken, "negedge"},
       " clk or ",
       {kToken, "negedge"},
       " rst) q <= d; endmodule"},
  };

  RunLintTestCases<VerilogAnalyzer, ForbidNegedgeRule>(kTestCases);
}

}  // namespace
}  // namespace analysis
}  // namespace verilog
