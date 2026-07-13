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

#include "verible/verilog/analysis/checkers/forbid-async-reset-rule.h"

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

TEST(ForbidAsyncResetRuleTests, Various) {
  // The violation is anchored on the always_ff keyword.
  constexpr int kToken = TK_always_ff;
  const std::initializer_list<LintTestCase> kTestCases = {
      // --- No violations ---
      {""},
      {"module m; endmodule"},
      // Single clock edge -> synchronous, allowed.
      {"module m; logic q, d, clk; "
       "always_ff @(posedge clk) q <= d; endmodule"},
      // A single negedge is one term, so fine for this rule).
      {"module m; logic q, d, clk; "
       "always_ff @(negedge clk) q <= d; endmodule"},
      // Not an always_ff at all.
      {"module m; logic y, a, b; always_comb y = a & b; endmodule"},
      // Two edge terms but in a *generic* always, should be flagged for generic always.
      {"module m; logic q, d, clk, rst; "
       "always @(posedge clk or negedge rst) q <= d; endmodule"},

      // --- Violations: always_ff with more than one edge term ---
      {"module m; logic q, d, clk, rst; ",
       {kToken, "always_ff"},
       " @(posedge clk or negedge rst) q <= d; endmodule"},

      {"module m; logic q, d, clk, rst; ",
       {kToken, "always_ff"},
       " @(posedge clk or posedge rst) q <= d; endmodule"},
  };

  RunLintTestCases<VerilogAnalyzer, ForbidAsyncResetRule>(kTestCases);
}

}  // namespace
}  // namespace analysis
}  // namespace verilog
