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

#include "verible/verilog/analysis/checkers/forbid-nested-if-statements-rule.h"

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

TEST(ForbidNestedIfStatementsRuleTests, Various) {
  const std::initializer_list<LintTestCase> kTestCases = {
      // No violations
      {""},
      {"module m; endmodule"},
      {"module m; assign y = sel ? a : b; endmodule"},
      {"module m; always_comb if (en) y = a; endmodule"},
      {"module m; always_comb if (en) y = a; else y = b; endmodule"},
      {"module m; always_comb if (a) y = a; "
       "else if (b) y = b; else if (c) y = c; else y = 'x; endmodule"},
      {"module m; always_comb begin "
       "if (a) begin y = a; end else if (b) begin y = b; end "
       "else begin y = 'x; end end endmodule"},
      {"module m; always_ff @(posedge clk) begin "
       "if (rst) q <= 0; else if (en) q <= d; "
       "else if (!en) q <= q; else q <= 'x; end endmodule"},

      {"module m; always_comb begin "
       "if (a) y = b; else y = c; "
       "if (d) z = e; else z = f; end endmodule"},
      {"module m; always_comb begin begin "
       "if (a) y = b; else y = c; end end endmodule"},
      {"module m; always_comb case (s) "
       "0: if (a) y = b; else y = c; default: y = 'x; endcase endmodule"},

      {"module m; // if (a) if (b) y = c;\nendmodule"},
      {"module m; initial $display(\"if (a) if (b) y = c;\"); endmodule"},
      {"`ifdef FEATURE\nmodule m; endmodule\n`endif\n"},

      // Violations with wested if statements
      {"module m; always_comb if (a) ", {TK_if, "if"}, " (b) y = c; endmodule"},
      {"module m; always_comb if (a) begin ",
       {TK_if, "if"},
       " (b) y = c; else y = d; end else y = 'x; endmodule"},
      {"module m; always_comb if (a) y = b; else begin ",
       {TK_if, "if"},
       " (c) y = d; else y = e; end endmodule"},
      {"module m; always_comb if (a) y = b; else if (c) begin ",
       {TK_if, "if"},
       " (d) y = e; end else y = 'x; endmodule"},
      {"module m; always_comb if (a) y = b; else if (c) y = d; "
       "else begin ",
       {TK_if, "if"},
       " (e) y = f; end endmodule"},

      {"module m; always_comb if (a) begin ",
       {TK_if, "if"},
       " (b) y = c; else if (d) y = e; else y = 'x; end endmodule"},

      {"module m; always_comb if (a) begin ",
       {TK_if, "if"},
       " (b) begin ",
       {TK_if, "if"},
       " (c) y = d; end end endmodule"},
      {"module m; always_comb if (a) case (s) 0: ",
       {TK_if, "if"},
       " (b) y = c; default: y = 'x; endcase endmodule"},

      {"module m; always_ff @(posedge clk) if (en) begin ",
       {TK_if, "if"},
       " (sel) q <= a; else q <= b; end endmodule"},
      {"module m; task automatic set_output; if (a) ",
       {TK_if, "if"},
       " (b) y = c; endtask endmodule"},
  };
  RunLintTestCases<VerilogAnalyzer, ForbidNestedIfStatementsRule>(kTestCases);
}

}  // namespace
}  // namespace analysis
}  // namespace verilog
