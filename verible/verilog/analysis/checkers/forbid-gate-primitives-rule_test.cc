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

#include "verible/verilog/analysis/checkers/forbid-gate-primitives-rule.h"

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

TEST(ForbidGatePrimitivesRuleTests, Various) {
  const std::initializer_list<LintTestCase> kTestCases = {
      // No violations
      {""},
      {"module m; endmodule"},
      {"module m; wire a, b, y; assign y = a & b; endmodule"},
      {"module m; wire [15:0] a, b, y; assign y = a + b; endmodule"},
      {"module m; wire [15:0] a, b, y; assign y = a * b; endmodule"},
      {"module m; logic a, b, y; always_comb y = a ^ b; endmodule"},
      {"module m; logic clk, d, q; "
       "always_ff @(posedge clk) q <= d; endmodule"},
      {"module m; logic a, b, y; always @(a or b) y = a | b; endmodule"},
      {"module m; wire a, y; Foo_GL u (.in(a), .out(y)); endmodule"},
      {"module m; wire a, y; Foo_RTL u (y, a); endmodule"},
      {"module m; // and(y, a, b);\n"
       "initial $display(\"not(y, a);\"); endmodule"},

      // Violations for gate primitives
      {"module m; wire y, a, b, en, enb; ",
       {TK_and, "and"},
       "(y, a, b); endmodule"},
      {"module m; wire y, a, b, en, enb; ",
       {TK_or, "or"},
       "(y, a, b); endmodule"},
      {"module m; wire y, a, b, en, enb; ",
       {TK_xor, "xor"},
       "(y, a, b); endmodule"},
      {"module m; wire y, a, b, en, enb; ",
       {TK_nand, "nand"},
       "(y, a, b); endmodule"},
      {"module m; wire y, a, b, en, enb; ",
       {TK_nor, "nor"},
       "(y, a, b); endmodule"},
      {"module m; wire y, a, b, en, enb; ",
       {TK_xnor, "xnor"},
       "(y, a, b); endmodule"},
      {"module m; wire y, a, b, en, enb; ",
       {TK_not, "not"},
       "(y, a); endmodule"},
      {"module m; wire y, a, b, en, enb; ",
       {TK_buf, "buf"},
       "(y, a); endmodule"},
      {"module m; wire y, a, b, en, enb; ",
       {TK_bufif0, "bufif0"},
       "(y, a, en); endmodule"},
      {"module m; wire y, a, b, en, enb; ",
       {TK_bufif1, "bufif1"},
       "(y, a, en); endmodule"},
      {"module m; wire y, a, b, en, enb; ",
       {TK_notif0, "notif0"},
       "(y, a, en); endmodule"},
      {"module m; wire y, a, b, en, enb; ",
       {TK_notif1, "notif1"},
       "(y, a, en); endmodule"},
      {"module m; wire y, a, b, en, enb; ",
       {TK_nmos, "nmos"},
       "(y, a, en); endmodule"},
      {"module m; wire y, a, b, en, enb; ",
       {TK_pmos, "pmos"},
       "(y, a, en); endmodule"},
      {"module m; wire y, a, b, en, enb; ",
       {TK_rnmos, "rnmos"},
       "(y, a, en); endmodule"},
      {"module m; wire y, a, b, en, enb; ",
       {TK_rpmos, "rpmos"},
       "(y, a, en); endmodule"},
      {"module m; wire y, a, b, en, enb; ",
       {TK_cmos, "cmos"},
       "(y, a, en, enb); endmodule"},
      {"module m; wire y, a, b, en, enb; ",
       {TK_rcmos, "rcmos"},
       "(y, a, en, enb); endmodule"},
      {"module m; wire y, a, b, en, enb; ",
       {TK_tran, "tran"},
       "(y, a); endmodule"},
      {"module m; wire y, a, b, en, enb; ",
       {TK_rtran, "rtran"},
       "(y, a); endmodule"},
      {"module m; wire y, a, b, en, enb; ",
       {TK_tranif0, "tranif0"},
       "(y, a, en); endmodule"},
      {"module m; wire y, a, b, en, enb; ",
       {TK_tranif1, "tranif1"},
       "(y, a, en); endmodule"},
      {"module m; wire y, a, b, en, enb; ",
       {TK_rtranif0, "rtranif0"},
       "(y, a, en); endmodule"},
      {"module m; wire y, a, b, en, enb; ",
       {TK_rtranif1, "rtranif1"},
       "(y, a, en); endmodule"},
      {"module m; wire y, a, b, en, enb; ",
       {TK_pullup, "pullup"},
       "(y); endmodule"},
      {"module m; wire y, a, b, en, enb; ",
       {TK_pulldown, "pulldown"},
       "(y); endmodule"},

      // Different instantiation forms
      {"module m; wire y, a, b; ", {TK_and, "and"}, " g(y, a, b); endmodule"},
      {"module m; wire [3:0] y, a, b; ",
       {TK_xor, "xor"},
       " g[3:0](y, a, b); endmodule"},
      {"module m; wire y, z, a, b; ",
       {TK_and, "and"},
       " g1(y, a, b), g2(z, a, b); endmodule"},
      {"module m; wire y, z, a, b; ",
       {TK_and, "and"},
       " (y, a, b); ",
       {TK_not, "not"},
       " (z, y); endmodule"},
      {"module m; wire y, a, b; generate if (1) begin : g ",
       {TK_and, "and"},
       " (y, a, b); end endgenerate endmodule"},

  };
  RunLintTestCases<VerilogAnalyzer, ForbidGatePrimitivesRule>(kTestCases);
}

}  // namespace
}  // namespace analysis
}  // namespace verilog
