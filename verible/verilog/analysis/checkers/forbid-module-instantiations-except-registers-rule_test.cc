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

#include "verible/verilog/analysis/checkers/forbid-module-instantiations-except-registers-rule.h"

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

TEST(ForbidModuleInstantiationsExceptRegistersRuleTests, Various) {
  constexpr int kToken = SymbolIdentifier;
  const std::initializer_list<LintTestCase> kTestCases = {
      // No violations
      {""},
      {"module m; logic q; wire d; assign q = d; endmodule"},
      {"module m; SomeType variable; endmodule"},
      {"module m; always_ff @(posedge clk) q <= d; endmodule"},
      {"module m; and g(y, a, b); not (z, y); endmodule"},

      // Every allowed module
      {"module m; DFF_GL r(); endmodule"},
      {"module m; DFF_RTL r(); endmodule"},
      {"module m; DFFR_GL r(); endmodule"},
      {"module m; DFFR_RTL r(); endmodule"},
      {"module m; DFFRE_GL r(); endmodule"},
      {"module m; DFFRE_RTL r(); endmodule"},
      {"module m; Register_16b_GL r(); endmodule"},
      {"module m; Register_16b_RTL r(); endmodule"},
      {"module m; RegfileZ2r1w_32x32b_RTL r(); endmodule"},
      {"module m; Register_32b_RTL r(); endmodule"},
      {"module m; ShiftRegister_44b_RTL r(); endmodule"},

      {"module m; DFF_RTL r(.clk(clk), .d(d), .q(q)); endmodule"},
      {"module m; DFF_RTL r(clk, d, q); endmodule"},
      {"module m; DFF_RTL #(.WIDTH(1)) r(); endmodule"},
      {"module m; DFF_RTL r[3:0](); endmodule"},
      {"module m; DFF_RTL r0(), r1(); endmodule"},

      // Violations
      {"module m; Adder_16b_RTL ", {kToken, "adder"}, "(); endmodule"},
      {"module m; Other ", {kToken, "DFF_RTL"}, "(); endmodule"},
      {"module m; DFF_RTL_extra ", {kToken, "r"}, "(); endmodule"},
      {"module m; DFF_RTL::Other ", {kToken, "r"}, "(); endmodule"},
      {"module m; dff_rtl ", {kToken, "r"}, "(); endmodule"},
      {"module m; Other #(.WIDTH(1)) ", {kToken, "r"}, "[3:0](); endmodule"},
      {"module m; Other ",
       {kToken, "r0"},
       "(), ",
       {kToken, "r1"},
       "(); endmodule"},
      {"module m; DFF_RTL r(); Other ", {kToken, "u"}, "(); endmodule"},

  };
  RunLintTestCases<VerilogAnalyzer,
                   ForbidModuleInstantiationsExceptRegistersRule>(kTestCases);
}

}  // namespace
}  // namespace analysis
}  // namespace verilog
