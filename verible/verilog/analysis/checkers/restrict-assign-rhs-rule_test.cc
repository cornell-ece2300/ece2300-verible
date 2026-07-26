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

#include "verible/verilog/analysis/checkers/restrict-assign-rhs-rule.h"

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

TEST(RestrictAssignRhsRuleTests, Various) {
  const std::initializer_list<LintTestCase> kTestCases = {
      // No violation
      {""},
      {"module m; wire [3:0] a, y; assign y = a; endmodule"},
      {"module m; wire [3:0] a; wire y; assign y = a[3]; endmodule"},
      {"module m; wire [3:0] a, y; assign y = a[3:0]; endmodule"},
      {"module m; wire y; assign y = 1'b0; endmodule"},
      {"module m; wire [3:0] a; wire [7:0] w; assign w = {4'b0, a}; endmodule"},
      {"module m; wire a, b, y; and( y, a, b ); endmodule"},
      {"module m; logic y, a, b; always_comb y = a & b; endmodule"},

      // Violations
      {"module m; wire a, b, y; assign y = ",
       {SymbolIdentifier, "a"},
       " & b; endmodule"},

      {"module m; wire a, y; assign y = ",
       {'~', "~"},
       "a; endmodule"},

      {"module m; wire a, b, sel, y; assign y = ",
       {SymbolIdentifier, "sel"},
       " ? a : b; endmodule"},

      {"module m; wire a, y; function automatic logic f(input logic x); "
       "return x; endfunction assign y = ",
       {SymbolIdentifier, "f"},
       "(a); endmodule"},

      {"module m; wire a, b; wire [3:0] c; wire [7:0] w; assign w = {",
       {SymbolIdentifier, "a"},
       " & b, c}; endmodule"},

      {"module m; wire a, b, c, y; assign y = ",
       {SymbolIdentifier, "a"},
       " & b | c; endmodule"},


  };

  RunLintTestCases<VerilogAnalyzer, RestrictAssignRhsRule>(kTestCases);
}

}  // namespace
}  // namespace analysis
}  // namespace verilog
