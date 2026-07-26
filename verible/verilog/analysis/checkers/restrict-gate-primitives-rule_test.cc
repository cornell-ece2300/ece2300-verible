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

#include "verible/verilog/analysis/checkers/restrict-gate-primitives-rule.h"

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

TEST(RestrictGatePrimitivesRuleTests, Various) {
  // Violations anchor on the gate keyword. Each disallowed gate has its own
  // token, so there is no single shared kToken:
  //     {TK_buf, "buf"}  {TK_bufif0, "bufif0"}  {TK_notif1, "notif1"}
  //     {TK_pullup, "pullup"} ...
  const std::initializer_list<LintTestCase> kTestCases = {
      // No violations
      {""},
      {"module m; wire a, b, y; and( y, a, b ); endmodule"},
      {"module m; wire a, b, y; or( y, a, b ); endmodule"},
      {"module m; wire a, y; not( y, a ); endmodule"},
      {"module m; wire a, b, y; xor( y, a, b ); endmodule"},
      {"module m; wire a, b, y; nand( y, a, b ); endmodule"},
      {"module m; wire a, b, y; nor( y, a, b ); endmodule"},
      {"module m; wire a, b, y; xnor( y, a, b ); endmodule"},
      {"module m; wire a, b, y; assign y = a & b; endmodule"},
      {"module m; wire a, y; Foo_GL u ( .in(a), .out(y) ); endmodule"},

      // Violations
      {"module m; wire a, y; ",
       {TK_buf, "buf"},
       "( y, a ); endmodule"},

      {"module m; wire y; ",
       {TK_pullup, "pullup"},
       "( y ); endmodule"},

      {"module m; wire a, y, en; ",
       {TK_bufif0, "bufif0"},
       "( y, a, en ); endmodule"},

      {"module m; wire a, y, en; ",
       {TK_notif1, "notif1"},
       "( y, a, en ); endmodule"},

       {"module m; wire a, b, y, t; and( y, a, b ); ",
       {TK_buf, "buf"},
       "( t, a ); endmodule"},


  };

  RunLintTestCases<VerilogAnalyzer, RestrictGatePrimitivesRule>(kTestCases);
}

}  // namespace
}  // namespace analysis
}  // namespace verilog
