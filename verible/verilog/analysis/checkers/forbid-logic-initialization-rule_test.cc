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

#include "verible/verilog/analysis/checkers/forbid-logic-initialization-rule.h"

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

TEST(ForbidLogicInitializationRuleTests, Various) {
  const std::initializer_list<LintTestCase> kTestCases = {
      // No violations
      {""},
      {"module m; logic x; endmodule"},
      {"module m; logic [7:0] a, b; endmodule"},
      {"module m; logic x; assign x = a & b; endmodule"},
      {"module m; logic x; always_comb x = a & b; endmodule"},
      {"module m; logic x; always_ff @(posedge clk) "
       "if (rst) x <= '0; else x <= d; endmodule"},

      {"module m; wire x = a & b; endmodule"},
      {"module m; reg r = 0; bit b = 0; int n = 0; endmodule"},
      {"module m; parameter logic P = 0; localparam logic Q = 1; endmodule"},
      {"module m; typedef logic bit_t; bit_t x = 0; endmodule"},
      {"module m; // logic x = 0;\nendmodule"},
      {"module m; initial $display(\"logic x = 0;\"); endmodule"},

      {"module m; logic x ", {'=', "="}, " 1'b0; endmodule"},
      {"module m; logic [7:0] count ", {'=', "="}, " '0; endmodule"},
      {"module m; logic x ", {'=', "="}, " a & b; endmodule"},
      {"module m; logic x ", {'=', "="}, " sel ? a : b; endmodule"},

      // Violations
      {"module m; var logic signed [3:0] x ", {'=', "="}, " '0; endmodule"},
      {"module m; logic a, b ", {'=', "="}, " 0, c; endmodule"},
      {"module m; logic a ",
       {'=', "="},
       " 0, b, c ",
       {'=', "="},
       " 1; endmodule"},
      {"module m; logic [7:0] a [2] ",
       {'=', "="},
       " '{default: '0}; endmodule"},

      {"module m; initial begin static logic x ",
       {'=', "="},
       " 0; x = 1; end endmodule"},
      {"module m; function automatic logic f(); logic x ",
       {'=', "="},
       " 0; return x; endfunction endmodule"},
  };
  RunLintTestCases<VerilogAnalyzer, ForbidLogicInitializationRule>(kTestCases);
}

}  // namespace
}  // namespace analysis
}  // namespace verilog
