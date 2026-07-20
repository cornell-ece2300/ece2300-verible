// Copyright 2017-2023 The Verible Authors.
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

#include "verible/verilog/analysis/checkers/if-x-assignment-rule.h"

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

TEST(IfXAssignmentRuleTests, Various) {
  constexpr int kToken = SymbolIdentifier;
  const std::initializer_list<LintTestCase> kTestCases = {
      // --- No violations ---
      {""},
      {"module m; endmodule"},
      {"module m; logic q, d, clk; "
       "always_ff @(posedge clk) q <= d; endmodule"},

      // One signal is assigned through the complete chain
      {"module m; logic q, a, b; always_comb begin "
       "if (a) q = b; else if (b) q = a; else q = 'x; "
       "end endmodule"},

      // DFFR 
      {"module DFFR_RTL\n"
       "(\n"
       "  input  logic clk,\n"
       "  input  logic rst,\n"
       "  input  logic en,\n"
       "  input  logic d,\n"
       "  output logic q\n"
       ");\n"
       "  always_ff @( posedge clk ) begin\n"
       "    if ( rst )\n"
       "      q <= 1'b0;\n"
       "    else if ( !rst && en )\n"
       "      q <= d;\n"
       "    else if ( !rst && !en )\n"
       "      q <= q;\n"
       "    else\n"
       "      q <= 'x;\n"
       "  end\n"
       "endmodule\n"},

      // Multiple assigned signals are all repeated in the final else
      {"module m; logic q, y, a, b; always_comb begin\n"
       "  if (a) begin\n"
       "    q = a;\n"
       "    y = b;\n"
       "  end else if (b) begin\n"
       "    q = b;\n"
       "    y = a;\n"
       "  end else begin\n"
       "    q = 'x;\n"
       "    y = 'x;\n"
       "  end\n"
       "end endmodule"},

      // No final else: handled by R204
      {"module m; logic q, a, b; always_comb begin "
       "if (a) q = b; "
       "end endmodule"},

      // --- Violations ---

      // The chain assigns q, but the final else assigns a different signal
      {"module m; logic q, y, a, b; always_comb begin if (a) ",
       {kToken, "q"},
       " = b; else y = 'x; end endmodule"},

      // y is missing even though q is covered and extra is added in the else.
      {"module m; logic q, y, extra, a, b; always_comb begin\n"
       "  if (a) begin\n"
       "    q = a;\n"
       "    ",
       {kToken, "y"},
       " = b;\n"
       "  end else begin\n"
       "    q = 'x;\n"
       "    extra = 'x;\n"
       "  end\n"
       "end endmodule"},
  };

  RunLintTestCases<VerilogAnalyzer, IfXAssignmentRule>(kTestCases);
}

}  // namespace
}  // namespace analysis
}  // namespace verilog
