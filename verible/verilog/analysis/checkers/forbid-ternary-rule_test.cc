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

#include "verible/verilog/analysis/checkers/forbid-ternary-rule.h"

#include <initializer_list>

#include "gtest/gtest.h"
#include "verible/common/analysis/linter-test-utils.h"
#include "verible/common/analysis/syntax-tree-linter-test-utils.h"
#include "verible/verilog/analysis/verilog-analyzer.h"

namespace verilog {
namespace analysis {
namespace {

using verible::LintTestCase;
using verible::RunLintTestCases;

TEST(ForbidTernaryRuleTests, Various) {
  const std::initializer_list<LintTestCase> kTestCases = {
      // No violations: other forms of combinational logic.
      {""},
      {"module m; endmodule"},
      {"module m; assign y = a & b; endmodule"},
      {"module m; always_comb if (sel) y = a; else y = b; endmodule"},
      {"module m; always_comb case (sel) "
       "0: y = a; default: y = 'x; endcase endmodule"},

      // ? in comments
      {"module m; always_comb casez (sel) "
       "2'b?1: y = a; default: y = 'x; endcase endmodule"},
      {"module m; // assign y = sel ? a : b;\nendmodule"},
      {"module m; initial $display(\"sel ? a : b\"); endmodule"},

      // Violations
      {"module m; assign y = sel ", {'?', "?"}, " a : b; endmodule"},
      {"module m; always_comb y = sel ", {'?', "?"}, " a : b; endmodule"},
      {"module m; always_ff @(posedge clk) q <= sel ",
       {'?', "?"},
       " a : b; endmodule"},

      // Ternaries in declarations, port connections, and task arguments.
      {"module m; localparam N = 1 ", {'?', "?"}, " 2 : 3; endmodule"},
      {"module m; Foo f (.in(sel ", {'?', "?"}, " a : b)); endmodule"},
      {"module m; initial set_output(sel ", {'?', "?"}, " a : b); endmodule"},

      // Report each nested ternary, including within larger expressions.
      {"module m; assign y = sel ",
       {'?', "?"},
       " (a ",
       {'?', "?"},
       " b : c) : d; endmodule"},
      {"module m; assign y = sel ",
       {'?', "?"},
       " a : b ",
       {'?', "?"},
       " c : d; endmodule"},
      {"module m; assign y = 1 + (sel ", {'?', "?"}, " a : b); endmodule"},
  };
  RunLintTestCases<VerilogAnalyzer, ForbidTernaryRule>(kTestCases);
}

}  // namespace
}  // namespace analysis
}  // namespace verilog
