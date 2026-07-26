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

#include "verible/verilog/analysis/checkers/restrict-port-connections-rule.h"

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

TEST(RestrictPortConnectionsRuleTests, Various) {
  const std::initializer_list<LintTestCase> kTestCases = {
      // No violation 
      {""},
      {"module m; wire a, b, y; and( y, a, b ); endmodule"},
      {"module m; wire [3:0] a; wire y; and( y, a[0], a[1] ); endmodule"},
      {"module m; wire a, y; Foo_GL u ( .in(a), .out(y) ); endmodule"},
      {"module m; wire [3:0] a; wire y; "
       "Foo_GL u ( .in(a[3:2]), .out(y) ); endmodule"},
      {"module m; wire a, b, y; Foo_GL u ( .in({a, b}), .out(y) ); endmodule"},
      {"module m; wire a, b, y; assign y = a & b; endmodule"},

      // Violations
      {"module m; wire a, b, y; and( y, ",
       {'~', "~"},
       "a, b ); endmodule"},

      {"module m; wire a, b, y; Foo_GL u ( .in(",
       {SymbolIdentifier, "a"},
       " & b), .out(y) ); endmodule"},

      {"module m; wire a, b, sel, y; Foo_GL u ( .in(",
       {SymbolIdentifier, "sel"},
       " ? a : b), .out(y) ); endmodule"},

      {"module m; wire a, b, y; and( y, ",
       {'~', "~"},
       "a, ",
       {'~', "~"},
       "b ); endmodule"},
  };

  RunLintTestCases<VerilogAnalyzer, RestrictPortConnectionsRule>(kTestCases);
}

}  // namespace
}  // namespace analysis
}  // namespace verilog
