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

#include "verible/verilog/analysis/checkers/forbid-generic-always-rule.h"

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

TEST(ForbidGenericAlwaysRuleTests, Various) {
  // The violation is anchored on the bare `always` keyword token.
  constexpr int kToken = TK_always;
  const std::initializer_list<LintTestCase> kTestCases = {
      // No violations
      {""},
      {"module m; endmodule"},

      {"module m; logic y, a, b; always_comb y = a ^ b; endmodule"},
      {"module m; logic q, a, clk; "
       "always_ff @(posedge clk) q <= a; endmodule"},
      {"module m; logic y, a, b; always_latch if (a) y = b; endmodule"},

      // Violations
      {"module m; logic y, a, b; ",
       {kToken, "always"},
       " @* y = a & b; endmodule"},

      {"module m; logic y, a, b; ",
       {kToken, "always"},
       " @(a or b) y = a | b; endmodule"},

      {"module m; logic q, a, clk; ",
       {kToken, "always"},
       " @(posedge clk) q <= a; endmodule"},

      {"module m; logic y1, y2, a, b; ",
       {kToken, "always"},
       " @* y1 = a & b; ",
       {kToken, "always"},
       " @* y2 = a | b; endmodule"},

      {"module m; logic y1, y2, a, b; always_comb y1 = a & b; ",
       {kToken, "always"},
       " @* y2 = a | b; endmodule"},
  };

  RunLintTestCases<VerilogAnalyzer, ForbidGenericAlwaysRule>(kTestCases);
}

}  // namespace
}  // namespace analysis
}  // namespace verilog
