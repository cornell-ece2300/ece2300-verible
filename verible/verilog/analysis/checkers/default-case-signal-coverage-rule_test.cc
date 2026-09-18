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

#include "verible/verilog/analysis/checkers/default-case-signal-coverage-rule.h"

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

TEST(DefaultCaseSignalCoverageRuleTests, Various) {
  constexpr int kToken = SymbolIdentifier;
  const std::initializer_list<LintTestCase> kTestCases = {
      // No violations
      {""},
      {"module m; endmodule"},
      {"module m; logic y, a; always_comb y = a; endmodule"},
      {"module m; logic [1:0] sel; logic y, z, a; always_comb "
       "case (sel) 2'd0: begin y = a; z = a; end "
       "2'd1: begin y = a; z = a; end "
       "default: begin y = 'x; z = 'x; end endcase endmodule"},
      {"module m; logic [1:0] sel; logic y, z, a; always_comb "
       "case (sel) 2'd0: y = a; "
       "default: begin y = 'x; z = 'x; end endcase endmodule"},
      {"module m; logic [1:0] sel; logic [3:0] v; logic a; always_comb "
       "case (sel) 2'd0: v[3] = a; default: v = 'x; endcase endmodule"},

      // Violations, anchored on the LHS of the case-item assignment
      // w is assigned in a case item but missing from the default
      {"module m; logic [1:0] sel; logic y, w, a; always_comb "
       "case (sel) 2'd0: y = a; 2'd1: ",
       {kToken, "w"},
       " = a; default: y = 'x; endcase endmodule"},

      // two uncovered signals, two violations
      {"module m; logic [1:0] sel; logic y, v, w, a; always_comb "
       "case (sel) 2'd0: ",
       {kToken, "w"},
       " = a; 2'd1: ",
       {kToken, "v"},
       " = a; default: y = 'x; endcase endmodule"},

      // same signal in several case items, one violation
      {"module m; logic [1:0] sel; logic y, w, a; always_comb "
       "case (sel) 2'd0: ",
       {kToken, "w"},
       " = a; 2'd1: w = a; default: y = 'x; endcase endmodule"},

      // no default present
      {"module m; logic [1:0] sel; logic y, z, a; always_comb "
       "case (sel) 2'd0: begin ",
       {kToken, "y"},
       " = a; ",
       {kToken, "z"},
       " = a; end endcase endmodule"},
  };

  RunLintTestCases<VerilogAnalyzer, DefaultCaseSignalCoverageRule>(kTestCases);
}

}  // namespace
}  // namespace analysis
}  // namespace verilog
