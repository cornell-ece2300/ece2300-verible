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

#include "verible/verilog/analysis/checkers/forbid-if-statements-rule.h"

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

TEST(ForbidIfStatementsRuleTests, Various) {
  const std::initializer_list<LintTestCase> kTestCases = {
      // No violations
      {""},
      {"module m; endmodule"},
      {"module m; assign y = a & b; endmodule"},
      {"module m; assign y = sel ? a : b; endmodule"},
      {"module m; always_comb case (sel) "
       "0: y = a; default: y = 'x; endcase endmodule"},
      {"module m; always_comb casez (sel) "
       "2'b?1: y = a; default: y = 'x; endcase endmodule"},

      // If in comments
      {"module m; // if (sel) y = a;\nendmodule"},
      {"module m; initial $display(\"if (sel) y = a;\"); endmodule"},
      {"`ifdef FEATURE\nmodule m; endmodule\n`else\n"
       "module n; endmodule\n`endif\n"},

      {"module m; always_comb ", {TK_if, "if"}, " (sel) y = a; endmodule"},
      {"module m; always_comb ",
       {TK_if, "if"},
       " (sel) begin y = a; end else begin y = b; end endmodule"},
      {"module m; always_ff @(posedge clk) ",
       {TK_if, "if"},
       " (en) q <= d; endmodule"},
      {"module m; initial ", {TK_if, "if"}, " (sel) y = a; endmodule"},
      {"module m; task automatic set_output; ",
       {TK_if, "if"},
       " (sel) y = a; endtask endmodule"},

      // Report every if in an else-if chain
      {"module m; always_comb ",
       {TK_if, "if"},
       " (a) y = a; else ",
       {TK_if, "if"},
       " (b) y = b; else y = 'x; endmodule"},
      {"module m; always_comb ",
       {TK_if, "if"},
       " (a) begin ",
       {TK_if, "if"},
       " (b) y = b; end endmodule"},

      {"module m; always_comb unique ",
       {TK_if, "if"},
       " (sel) y = a; else y = b; endmodule"},
      {"module m; always_comb priority ",
       {TK_if, "if"},
       " (sel) y = a; else y = b; endmodule"},

      {"module m; generate ",
       {TK_if, "if"},
       " (1) begin : g assign y = a; end endgenerate endmodule"},
      {"module m; ",
       {TK_if, "if"},
       " (1) begin : g assign y = a; end endmodule"},
  };
  RunLintTestCases<VerilogAnalyzer, ForbidIfStatementsRule>(kTestCases);
}

}  // namespace
}  // namespace analysis
}  // namespace verilog
