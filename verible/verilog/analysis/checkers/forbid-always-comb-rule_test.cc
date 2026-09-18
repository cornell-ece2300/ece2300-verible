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

#include "verible/verilog/analysis/checkers/forbid-always-comb-rule.h"

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

TEST(ForbidAlwaysCombRuleTests, Various) {
  const std::initializer_list<LintTestCase> kTestCases = {
      // No violations
      {""},
      {"module m; endmodule"},
      {"module m; wire y, a, b; assign y = a & b; endmodule"},
      {"module m; logic y, a, b; always @* y = a & b; endmodule"},
      {"module m; logic q, d, clk; "
       "always @(posedge clk) q <= d; endmodule"},
      {"module m; logic q, d, clk; "
       "always_ff @(posedge clk) q <= d; endmodule"},
      {"module m; logic y, a, b; always_latch if (a) y = b; endmodule"},
      {"module m; logic y; initial y = 0; endmodule"},
      {"module m; // always_comb in a comment\n"
       "initial $display(\"always_comb\"); endmodule"},
      {"module m; logic y, a, b; always_ff y <= a & b; endmodule"},


      // Violations
      {"module m; logic y, a, b; ",
       {TK_always_comb, "always_comb"},
       " y = a & b; endmodule"},
      {"module m; logic y, a, b; ",
       {TK_always_comb, "always_comb"},
       " begin if (a) y = b; else y = 'x; end endmodule"},

      {"module m; logic y, z, a, b; ",
       {TK_always_comb, "always_comb"},
       " begin y = a; z = b; end ",
       {TK_always_comb, "always_comb"},
       " y = b; endmodule"},

  };

  RunLintTestCases<VerilogAnalyzer, ForbidAlwaysCombRule>(kTestCases);
}

}  // namespace
}  // namespace analysis
}  // namespace verilog
