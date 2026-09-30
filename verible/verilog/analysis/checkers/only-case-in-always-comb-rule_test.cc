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

#include "verible/verilog/analysis/checkers/only-case-in-always-comb-rule.h"

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

TEST(OnlyCaseInAlwaysCombRuleTests, Various) {
  const std::initializer_list<LintTestCase> kTestCases = {
      // No violations
      {""},
      {"module m; endmodule"},
      {"module m; assign y = a; endmodule"},
      {"module m; always @* begin y = a; z = b; end endmodule"},
      {"module m; always_ff @(posedge clk) q <= d; endmodule"},
      {"module m; always_latch if (en) q = d; endmodule"},
      {"module m; initial begin y = a; z = b; end endmodule"},
      {"module m; // always_comb\ninitial $display(\"always_comb\"); "
       "endmodule"},
      {"module m; always_comb begin end endmodule"},
      {"module m; always_comb case (s) default: y = a; endcase endmodule"},
      {"module m; always_comb begin "
       "case (s) 0: y = a; default: y = b; endcase end endmodule"},
      {"module m; always_comb casez (s) 1'b?: y = a; endcase endmodule"},
      {"module m; always_comb begin "
       "casez (s) 1'b?: y = a; endcase end endmodule"},
      {"module m; always_comb casex (s) 1'bx: y = a; endcase endmodule"},
      {"module m; always_comb begin "
       "casex (s) 1'bx: y = a; endcase end endmodule"},
      {"module m; always_comb unique case (s) default: y = a; endcase "
       "endmodule"},
      {"module m; always_comb begin "
       "priority casez (s) default: y = a; endcase end endmodule"},
      {"module m; always_comb begin : outputs "
       "case (s) default: y = a; endcase end : outputs endmodule"},
      // Contents of the case branches are not restricted by this rule
      {"module m; always_comb begin case (s) "
       "0: begin y = a; z = b; end "
       "1: if (en) y = a; else y = b; "
       "default: case (t) default: y = c; endcase "
       "endcase end endmodule"},
      {"module m; always_comb begin /* comment */ "
       "case (s) default: y = a; endcase // comment\nend endmodule"},
      {"module m; always_comb case (s) default: y = a; endcase "
       "always_comb begin case (t) default: z = b; endcase end endmodule"},

      // Violations
      {"module m; ", {TK_always_comb, "always_comb"}, " y = a; endmodule"},
      {"module m; ",
       {TK_always_comb, "always_comb"},
       " begin y = a; end endmodule"},
      {"module m; ",
       {TK_always_comb, "always_comb"},
       " if (en) y = a; else y = b; endmodule"},
      {"module m; ",
       {TK_always_comb, "always_comb"},
       " begin if (en) y = a; end endmodule"},
      {"module m; ",
       {TK_always_comb, "always_comb"},
       " begin y = a; case (s) default: y = b; endcase end endmodule"},
      {"module m; ",
       {TK_always_comb, "always_comb"},
       " begin case (s) default: y = b; endcase y = a; end endmodule"},
      {"module m; ",
       {TK_always_comb, "always_comb"},
       " begin case (s) default: y = a; endcase "
       "case (t) default: z = b; endcase end endmodule"},
      {"module m; ",
       {TK_always_comb, "always_comb"},
       " begin logic tmp; case (s) default: y = a; endcase end endmodule"},
      {"module m; ",
       {TK_always_comb, "always_comb"},
       " begin ; case (s) default: y = a; endcase end endmodule"},
      {"module m; ",
       {TK_always_comb, "always_comb"},
       " begin case (s) default: y = a; endcase ; end endmodule"},
      // A case buried inside another block, conditional, or loop is not enough.
      {"module m; ",
       {TK_always_comb, "always_comb"},
       " begin begin case (s) default: y = a; endcase end end endmodule"},
      {"module m; ",
       {TK_always_comb, "always_comb"},
       " if (en) case (s) default: y = a; endcase endmodule"},
      {"module m; ",
       {TK_always_comb, "always_comb"},
       " begin for (int i = 0; i < 2; i++) "
       "case (s) default: y = a; endcase end endmodule"},
      {"module m; ",
       {TK_always_comb, "always_comb"},
       " randcase 1: y = a; endcase endmodule"},
      {"module m; always_comb case (s) default: y = a; endcase ",
       {TK_always_comb, "always_comb"},
       " y = b; always_comb begin end endmodule"},
  };
  RunLintTestCases<VerilogAnalyzer, OnlyCaseInAlwaysCombRule>(kTestCases);
}

}  // namespace
}  // namespace analysis
}  // namespace verilog
