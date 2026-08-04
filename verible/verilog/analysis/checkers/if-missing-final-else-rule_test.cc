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

#include "verible/verilog/analysis/checkers/if-missing-final-else-rule.h"

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

TEST(IfMissingFinalElseRuleTests, Various) {
  // The violation gets flagged on the leftmost leaf of the flagged conditional,
  // which is its "if" keyword, so markers are {kToken, "if"}
  constexpr int kToken = TK_if;
  const std::initializer_list<LintTestCase> kTestCases = {
      // no if blocks
      {""},
      {"module m; endmodule"},
      {"module m; logic q, d, clk; "
       "always_ff @(posedge clk) q <= d; endmodule"},
      {"module m; logic y, a, b; always_comb y = a & b; endmodule"},

      // if with no else --> flagged on its "if"
      {"module m; logic y, a, b; always_comb begin y = 1'b0; ",
       {kToken, "if"},
       " (a) y = b; end endmodule"},

      // if with an else, no flag
      {"module m; logic y, a, b; always_comb begin y = 1'b0; if (a) y = b; "
       "else y = a; end endmodule"},

      // dffr with else
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

      // dffr with no else --> flagged on the LAST if of the chain
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
       "    else ",
       {kToken, "if"},
       " ( !rst && !en )\n"
       "      q <= q;\n"
       "  end\n"
       "endmodule\n"},
  };

  RunLintTestCases<VerilogAnalyzer, IfMissingFinalElseRule>(kTestCases);
}

}  // namespace
}  // namespace analysis
}  // namespace verilog
