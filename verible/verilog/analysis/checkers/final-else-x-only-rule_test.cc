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

#include "verible/verilog/analysis/checkers/final-else-x-only-rule.h"

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

TEST(FinalElseXOnlyRuleTests, Various) {
  constexpr int kToken = SymbolIdentifier;
  const std::initializer_list<LintTestCase> kTestCases = {
      // --- No violations ---
      {""},
      {"module m; endmodule"},
      {"module m; logic q, d, clk; "
       "always_ff @(posedge clk) q <= d; endmodule"},
      {"module m; logic q, d, clk; "
       "always_ff @(negedge clk) q <= d; endmodule"},
      // Not an always_ff at all.
      {"module m; logic y, a, b; always_comb y = a & b; endmodule"},
      {"module m; logic q, d, clk, rst; "
       "always @(posedge clk or negedge rst) q <= d; endmodule"},

      // try 'x
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

      // try 4'bx
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
       "      q <= 4'bx;\n"
       "  end\n"
       "endmodule\n"},

      // try `WIDTH'bx and the previous together
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
       "    else begin\n"
       "      q <= `WIDTH'bx;\n"
       "      q <= 4'bx;\n"
       "      q <= 'x;\n"
       "    end\n"
       "  end\n"
       "endmodule\n"},

      // Violations after here
      // try 1'b0
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
       "      ",
       {kToken, "q"},
       " <= 1'b0;\n"
       "  end\n"
       "endmodule\n"},

      // try 10'b0
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
       "      ",
       {kToken, "q"},
       " <= 10'b0;\n"
       "  end\n"
       "endmodule\n"},

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
       "      ",
       {kToken, "q"},
       " <= 2'b0x;\n"
       "  end\n"
       "endmodule\n"},

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
       "      ",
       {kToken, "q"},
       " <= 4'b010x;\n"
       "  end\n"
       "endmodule\n"},
  };

  RunLintTestCases<VerilogAnalyzer, FinalElseXOnlyRule>(kTestCases);
}

}  // namespace
}  // namespace analysis
}  // namespace verilog
