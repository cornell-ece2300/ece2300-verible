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

#include "verible/verilog/analysis/checkers/forbid-nested-ternary-rule.h"

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

TEST(ForbidNestedTernaryRuleTests, Various) {
  const std::initializer_list<LintTestCase> kTestCases = {
      // No violations
      {""},
      {"module m; endmodule"},
      {"module m; assign y = sel ? a : b; endmodule"},
      {"module m; assign y = ((sel ? a : b)); endmodule"},
      {"module m; assign y = (s ? a : b) + (t ? c : d); endmodule"},
      {"module m; assign y = s ? a : b, z = t ? c : d; endmodule"},
      {"module m; always_comb if (en) y = sel ? a : b; endmodule"},
      {"module m; always_ff @(posedge clk) q <= sel ? a : b; endmodule"},

      {"module m; // assign y = s ? a : t ? b : c;\nendmodule"},
      {"module m; initial $display(\"s ? a : t ? b : c\"); endmodule"},
      {"module m; always_comb casez (sel) "
       "2'b?1: y = a; default: y = 'x; endcase endmodule"},

      // Violations
      {"module m; assign y = sel ? a : (c ", {'?', "?"}, " b : z); endmodule"},
      {"module m; assign y = sel ? a : c ", {'?', "?"}, " b : z; endmodule"},
      {"module m; assign y = sel ? (c ", {'?', "?"}, " b : z) : a; endmodule"},
      {"module m; assign y = (c ", {'?', "?"}, " b : z) ? a : d; endmodule"},

      {"module m; assign y = sel ? a : (((c ",
       {'?', "?"},
       " b : z))); endmodule"},
      {"module m; assign y = sel ? a : 1 + (c ",
       {'?', "?"},
       " b : z); endmodule"},
      {"module m; assign y = sel ? f(c ", {'?', "?"}, " b : z) : a; endmodule"},

      {"module m; assign y = s ? a : t ",
       {'?', "?"},
       " b : u ",
       {'?', "?"},
       " c : d; endmodule"},
      {"module m; assign y = s ? (t ",
       {'?', "?"},
       " a : b) : (u ",
       {'?', "?"},
       " c : d); endmodule"},

      {"module m; localparam N = s ? a : t ", {'?', "?"}, " b : c; endmodule"},
      {"module m; Foo f (.in(s ? a : t ", {'?', "?"}, " b : c)); endmodule"},
      {"module m; always_comb y = s ? a : t ", {'?', "?"}, " b : c; endmodule"},
      {"module m; always_ff @(posedge clk) q <= s ? a : t ",
       {'?', "?"},
       " b : c; endmodule"},
  };
  RunLintTestCases<VerilogAnalyzer, ForbidNestedTernaryRule>(kTestCases);
}

}  // namespace
}  // namespace analysis
}  // namespace verilog
