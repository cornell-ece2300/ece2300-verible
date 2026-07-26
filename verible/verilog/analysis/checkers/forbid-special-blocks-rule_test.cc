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

#include "verible/verilog/analysis/checkers/forbid-special-blocks-rule.h"

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

TEST(ForbidSpecialBlocksRuleTests, Various) {
  // Each violation anchors on its construct's leftmost leaf, the keyword.
  //
  //     {TK_always_comb, "always_comb"}   {TK_initial,  "initial"}
  //     {TK_function,    "function"}      {TK_task,     "task"}
  //     {TK_generate,    "generate"}
  const std::initializer_list<LintTestCase> kTestCases = {
      // No violation
      {""},
      {"module m; endmodule"},
      {"module m; logic a, b, c; and(c, a, b); endmodule"},
      {"module m; logic a, b, c, d ; and(c, a, b); and(d, a, c); endmodule"},
      {"module m; logic a, b, c, d ; assign c = a & b; assign d = a & c; endmodule"},
      {"module m; wire a, b, y, t; and( y, a, b ); not( t, a ); endmodule"},
      {"module m; logic clk, rst, en, d, q; DFFR_RTL dffr (.clk (clk),.rst (rst),.en  (en),.d   (d),.q   (q)); endmodule"},
      {"module m; wire a, y1, y2; Foo_GL u1 ( .in(a), .out(y1) ); Foo_GL u2 ( .in(a), .out(y2) ); endmodule"},

      // Violations -- there is no single shared kToken because each 
      // disallowed construct fails with its own token
      {"module m; logic q, d, clk, rst; ",
       {TK_always, "always"},
       " @(posedge clk or negedge rst) q <= d; endmodule"},

      {"module m; logic q, d, clk, rst; ",
       {TK_always_ff, "always_ff"},
       " @(posedge clk or negedge rst) q <= d; endmodule"},

      {"module m; logic a, b, c, d; ",
       {TK_always_comb, "always_comb"},
       " begin c = a & b; d = a & c; end endmodule"},

      {"module m; logic q; ",
       {TK_initial, "initial"},
       " q = 1'b0; endmodule"},

      {"module m; ",
       {TK_function, "function"},
       " automatic logic f(input logic x); return x; endfunction endmodule"},

      {"module m; ",
       {TK_task, "task"},
       " automatic t(); endtask endmodule"},

      {"module m; wire z; ",
       {TK_generate, "generate"},
       " if (1) begin : g wire w; end endgenerate endmodule"},

      {"module m; wire a; ",
       {SystemTFIdentifier, "$info"},
       "(\"hi\"); endmodule"},
  };

  RunLintTestCases<VerilogAnalyzer, ForbidSpecialBlocksRule>(kTestCases);
}

}  // namespace
}  // namespace analysis
}  // namespace verilog
