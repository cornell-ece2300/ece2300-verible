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

#include "verible/verilog/analysis/checkers/forbid-module-instantiation-rule.h"

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

TEST(ForbidModuleInstantiationRuleTests, Various) {

  constexpr int kToken = SymbolIdentifier;
  const std::initializer_list<LintTestCase> kTestCases = {
      // No violations
      {""},
      {"module m; endmodule"},

      {"module m; logic q, d, clk; "
       "always_ff @(posedge clk) q <= d; endmodule"},
      {"module m; logic q, d, clk; "
       "always_ff @(negedge clk) q <= d; endmodule"},
      {"module m; logic y, a, b; always_comb y = a & b; endmodule"},
      {"module m; logic q, d, clk, rst; "
       "always @(posedge clk or negedge rst) q <= d; endmodule"},
      {"module m; logic q, d, clk, rst; always_ff @(posedge clk or negedge rst) q <= d; endmodule"},
      {"module m; logic q, d, clk, rst; always_ff @(posedge clk or posedge rst) q <= d; endmodule"},
      {"module m; wire a, b, y, t; and( y, a, b ); not( t, a ); endmodule"},

      // Violations
      {"module m; logic clk, rst, en, d, q; DFFR_RTL ",
       {kToken, "dffr"},
       " (.clk (clk),.rst (rst),.en  (en),.d   (d),.q   (q)); endmodule"},
      
      // Two instances of the same submodule --> two violations
      {"module m; wire a, y1, y2; Foo_GL ",
       {kToken, "u1"},
       " ( .in(a), .out(y1) ); Foo_GL ",
       {kToken, "u2"},
       " ( .in(a), .out(y2) ); endmodule"},
  };

  RunLintTestCases<VerilogAnalyzer, ForbidModuleInstantiationRule>(kTestCases);
}

}  // namespace
}  // namespace analysis
}  // namespace verilog
