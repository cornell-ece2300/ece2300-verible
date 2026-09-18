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

#include "verible/verilog/analysis/checkers/default-case-x-only-rule.h"

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

TEST(DefaultCaseXOnlyRuleTests, Various) {
  constexpr int kToken = SymbolIdentifier;
  const std::initializer_list<LintTestCase> kTestCases = {
      // No violations
      {""},
      {"module m; endmodule"},
      {"module m; logic y, a; always_comb y = a; endmodule"},

      {"module m; logic [1:0] sel; logic y, a; always_comb "
       "case (sel) 2'd0: y = a; default: y = 'x; endcase endmodule"},

      {"module m; logic [1:0] sel; logic y, z, a; always_comb "
       "case (sel) 2'd0: begin y = a; z = a; end "
       "default: begin y = 'x; z = 'x; end endcase endmodule"},

      {"module m; logic [1:0] sel; logic [3:0] w; always_comb "
       "case (sel) 2'd0: w = 4'd3; default: w = 4'bxxxx; endcase endmodule"},

      // Violations, anchored on the LHS of the offending assignment
      {"module m; logic [1:0] sel; logic y, a; always_comb "
       "case (sel) 2'd0: y = a; default: ",
       {kToken, "y"},
       " = 1'b0; endcase endmodule"},

      // default assigns another signal
      {"module m; logic [1:0] sel; logic y, a, b; always_comb "
       "case (sel) 2'd0: y = a; default: ",
       {kToken, "y"},
       " = b; endcase endmodule"},

      // mixed block
      {"module m; logic [1:0] sel; logic y, z, a; always_comb "
       "case (sel) 2'd0: begin y = a; z = a; end "
       "default: begin y = 'x; ",
       {kToken, "z"},
       " = 1'b0; end endcase endmodule"},

      // nonblocking form
      {"module m; logic [1:0] sel; logic q, a, clk; always_ff @(posedge clk) "
       "case (sel) 2'd0: q <= a; default: ",
       {kToken, "q"},
       " <= 1'b0; endcase endmodule"},
  };

  RunLintTestCases<VerilogAnalyzer, DefaultCaseXOnlyRule>(kTestCases);
}

}  // namespace
}  // namespace analysis
}  // namespace verilog
