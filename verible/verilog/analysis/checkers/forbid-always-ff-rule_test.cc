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

#include "verible/verilog/analysis/checkers/forbid-always-ff-rule.h"

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

TEST(ForbidAlwaysFfRuleTests, Various) {
  const std::initializer_list<LintTestCase> kTestCases = {
      // No violations
      {""},
      {"module m; endmodule"},
      {"module m; wire a, y; assign y = a; endmodule"},
      {"module m; logic a, y; always_comb y = a; endmodule"},
      {"module m; logic a, y; always @* y = a; endmodule"},
      {"module m; logic clk, d, q; "
       "always @(posedge clk) q <= d; endmodule"},
      {"module m; logic en, d, q; always_latch if (en) q = d; endmodule"},
      {"module m; // always_ff in a comment\n"
       "initial $display(\"always_ff\"); endmodule"},

      // Violations
      {"module m; logic clk, d, q; ",
       {TK_always_ff, "always_ff"},
       " @(posedge clk) q <= d; endmodule"},
      {"module m; logic clk, d, q; ",
       {TK_always_ff, "always_ff"},
       " @(negedge clk) q <= d; endmodule"},
      {"module m; logic clk, rst, d, q; ",
       {TK_always_ff, "always_ff"},
       " @(posedge clk) begin if (rst) q <= 0; else q <= d; end endmodule"},

      {"module m; logic clk, d, q, r; ",
       {TK_always_ff, "always_ff"},
       " @(posedge clk) begin q <= d; r <= d; end ",
       {TK_always_ff, "always_ff"},
       " @(posedge clk) q <= r; endmodule"},
  };

  RunLintTestCases<VerilogAnalyzer, ForbidAlwaysFfRule>(kTestCases);
}

}  // namespace
}  // namespace analysis
}  // namespace verilog
