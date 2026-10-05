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

#include "verible/verilog/analysis/checkers/forbid-one-bit-literal-ternary-rule.h"

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

TEST(ForbidOneBitLiteralTernaryRuleTests, Various) {
  const std::initializer_list<LintTestCase> kTestCases = {
      // No violations
      {""},
      {"module m; assign y = s ? a : b; endmodule"},
      {"module m; assign y = s ? 1'b0 : a; endmodule"},
      {"module m; assign y = s ? a : 1'b1; endmodule"},
      {"module m; assign y = s ? 1'b0 : 2'b01; endmodule"},
      {"module m; assign y = s ? 2'b00 : 1'b1; endmodule"},
      {"module m; assign y = s ? 2'b00 : 2'b01; endmodule"},
      {"module m; assign y = s ? 1'bx : 1'b1; endmodule"},
      {"module m; assign y = s ? 1'b0 : 1'bz; endmodule"},
      {"module m; assign y = s ? 1'b? : 1'b1; endmodule"},

      {"module m; assign y = s ? 0 : 1; endmodule"},
      {"module m; assign y = s ? '0 : '1; endmodule"},
      {"module m; assign y = s ? 'b0 : 'b1; endmodule"},
      {"module m; assign y = s ? 1'b0 : 1; endmodule"},

      // Violations
      {"module m; assign y = s ", {'?', "?"}, " 1'b0 : 1'b1; endmodule"},
      {"module m; assign y = s ", {'?', "?"}, " 1'b1 : 1'b0; endmodule"},
      {"module m; assign y = s ", {'?', "?"}, " 1'b0 : 1'b0; endmodule"},
      {"module m; assign y = s ", {'?', "?"}, " 1'b1 : 1'b1; endmodule"},

      {"module m; assign y = s ", {'?', "?"}, " 1'd0 : 1'h1; endmodule"},
      {"module m; assign y = s ", {'?', "?"}, " 1'o1 : 1'B0; endmodule"},
      {"module m; assign y = s ", {'?', "?"}, " 1'sb1 : 1'sd0; endmodule"},
      {"module m; assign y = s ", {'?', "?"}, " 01'b0_0 : 0_1'h0_1; endmodule"},
      {"module m; assign y = s ", {'?', "?"}, " (1'b0) : ((1'b1)); endmodule"},
      {"module m; assign y = s ",
       {'?', "?"},
       " 1 /*width*/ 'b0 : 1'b1; endmodule"},
       
      {"module m; always_comb y = s ", {'?', "?"}, " 1'b0 : 1'b1; endmodule"},
      {"module m; assign y = s ? a : (t ",
       {'?', "?"},
       " 1'b0 : 1'b1); endmodule"},
      {"module m; assign y = (s ",
       {'?', "?"},
       " 1'b0 : 1'b1) ? a : b; endmodule"},
  };
  RunLintTestCases<VerilogAnalyzer, ForbidOneBitLiteralTernaryRule>(kTestCases);
}

}  // namespace
}  // namespace analysis
}  // namespace verilog
