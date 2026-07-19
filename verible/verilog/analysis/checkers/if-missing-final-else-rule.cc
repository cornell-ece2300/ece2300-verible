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

#include "verible/verilog/analysis/checkers/if-missing-final-else-rule.h"

#include <set>
#include <string_view>

#include "verible/common/analysis/lint-rule-status.h"
#include "verible/common/text/concrete-syntax-leaf.h"
#include "verible/common/text/concrete-syntax-tree.h"
#include "verible/common/text/symbol.h"
#include "verible/common/text/syntax-tree-context.h"
#include "verible/common/text/tree-utils.h"
#include "verible/verilog/CST/verilog-nonterminals.h"
#include "verible/verilog/analysis/descriptions.h"
#include "verible/verilog/analysis/lint-rule-registry.h"
#include "verible/verilog/parser/verilog-token-enum.h"

namespace verilog {
namespace analysis {

using verible::LintRuleStatus;
using verible::LintViolation;
using verible::SyntaxTreeContext;
VERILOG_REGISTER_LINT_RULE(IfMissingFinalElseRule);

static constexpr std::string_view kMessage =
    "if/else chain is missing a final 'else' clause. Add a final else so an "
    "'x' condition falls through to a defined x-assignment.";

const LintRuleDescriptor &IfMissingFinalElseRule::GetDescriptor() {
  static const LintRuleDescriptor d{
      .name = "if-missing-final-else",
      .topic = "x-propagation",
      .desc = "Checks that every if/else chain terminates in a plain `else` "
              "clause.",
  };
  return d;
}

// -----------------------------------------------------------------------------
//
// Shape of the tree (from `verible-verilog-syntax --printtree`):
//
//   kConditionalStatement          <- one link in the chain
//     kIfClause
//       kIfHeader                  <- the `if (cond)` part
//       kIfBody
//     kElseClause                  <- ABSENT entirely when there is no else
//       kElseBody
//         kConditionalStatement    <- present when this is an `else if`
//                                     (the chain nests to the right)
//
// So an if/else chain is a right-leaning nest. To decide whether it terminates
// in a plain `else`, walk down: at each kConditionalStatement, look for a
// kElseClause child.
//   - no kElseClause          -> chain ends without an else  => VIOLATION
//   - kElseClause whose body is another kConditionalStatement -> `else if`,
//                                keep walking down that one
//   - kElseClause whose body is NOT a kConditionalStatement   -> plain final
//                                else => OK
//
// Two things to watch:
//   1. Only evaluate the OUTERMOST kConditionalStatement of a chain, otherwise
//      each nested `else if` gets reported separately for the same chain.
//      `context.IsInside(NodeEnum::kElseBody)` is one way to skip inner links.
//   2. Anchor the violation somewhere useful for the student -- the leftmost
//      leaf of the outermost statement is the `if` keyword.

/*
                    Leaf @1 (#"if" @152-154: "if")
                    Node @2 (tag: kParenGroup) {
                      Leaf @0 (#'(' @155-156: "(")
                      Node @1 (tag: kExpression) {
                        Node @0 (tag: kFunctionCall) {
                          Node @0 (tag: kReference) {
                            Node @0 (tag: kLocalRoot) {
                              Node @0 (tag: kUnqualifiedId) {
                                Leaf @0 (#SymbolIdentifier @157-160: "rst")
                              }
                            }
                          }
                        }
                      }
                      Leaf @2 (#')' @161-162: ")")
                    }
                  }
                  Node @1 (tag: kIfBody) {
                    Node @0 (tag: kNonblockingAssignmentStatement) {
                      Node @0 (tag: kLPValue) {
                        Node @0 (tag: kReference) {
                          Node @0 (tag: kLocalRoot) {
                            Node @0 (tag: kUnqualifiedId) {
                              Leaf @0 (#SymbolIdentifier @169-170: "q")
                            }
                          }
                        }
                      }
                      Leaf @1 (#"<=" @171-173: "<=")
                      Node @3 (tag: kExpression) {
                        Node @0 (tag: kNumber) {
                          Leaf @0 (#TK_DecNumber @174-175: "1")
                          Node @1 (tag: kBaseDigits) {
                            Leaf @0 (#TK_BinBase @175-177: "\'b")
                            Leaf @1 (#TK_BinDigits @177-178: "0")
                          }
                        }
                      }
                      Leaf @4 (#';' @178-179: ";")
                    }
                  }
                }
                Node @1 (tag: kElseClause) {
                  Leaf @0 (#"else" @184-188: "else")
                  Node @1 (tag: kElseBody) {
                    Node @0 (tag: kConditionalStatement) {
                      Node @0 (tag: kIfClause) {
                        Node @0 (tag: kIfHeader) {
                          Leaf @1 (#"if" @189-191: "if")
                          Node @2 (tag: kParenGroup) {
                            Leaf @0 (#'(' @192-193: "(")
                            Node @1 (tag: kExpression) {
                              Node @0 (tag: kBinaryExpression) {
                                Node @0 (tag: kUnaryPrefixExpression) {
                                  Leaf @0 (#'!' @194-195: "!")
                                  Node @1 (tag: kFunctionCall) {
                                    Node @0 (tag: kReference) {
                                      Node @0 (tag: kLocalRoot) {
                                        Node @0 (tag: kUnqualifiedId) {
                                          Leaf @0 (#SymbolIdentifier @195-198: "rst")
                                        }
                                      }
                                    }
                                  }
                                }
                                Leaf @1 (#"&&" @199-201: "&&")
                                Node @2 (tag: kFunctionCall) {
                                  Node @0 (tag: kReference) {
                                    Node @0 (tag: kLocalRoot) {
                                      Node @0 (tag: kUnqualifiedId) {
                                        Leaf @0 (#SymbolIdentifier @202-204: "en")
                                      }
                                    }
                                  }
                                }
                              }
                            }
                            Leaf @2 (#')' @205-206: ")")
                          }
                        }
                        Node @1 (tag: kIfBody) {
                          Node @0 (tag: kNonblockingAssignmentStatement) {
                            Node @0 (tag: kLPValue) {
                              Node @0 (tag: kReference) {
                                Node @0 (tag: kLocalRoot) {
                                  Node @0 (tag: kUnqualifiedId) {
                                    Leaf @0 (#SymbolIdentifier @213-214: "q")
                                  }
                                }
                              }
                            }
                            Leaf @1 (#"<=" @215-217: "<=")
                            Node @3 (tag: kExpression) {
                              Node @0 (tag: kFunctionCall) {
                                Node @0 (tag: kReference) {
                                  Node @0 (tag: kLocalRoot) {
                                    Node @0 (tag: kUnqualifiedId) {
                                      Leaf @0 (#SymbolIdentifier @218-219: "d")
                                    }
                                  }
                                }
                              }
                            }
                            Leaf @4 (#';' @219-220: ";")
                          }
                        }
                      }
                      Node @1 (tag: kElseClause) {
                        Leaf @0 (#"else" @225-229: "else")
                        Node @1 (tag: kElseBody) {
                          Node @0 (tag: kConditionalStatement) {
                            Node @0 (tag: kIfClause) {
                              Node @0 (tag: kIfHeader) {
                                Leaf @1 (#"if" @230-232: "if")
                                Node @2 (tag: kParenGroup) {
                                  Leaf @0 (#'(' @233-234: "(")
                                  Node @1 (tag: kExpression) {
                                    Node @0 (tag: kBinaryExpression) {
                                      Node @0 (tag: kUnaryPrefixExpression) {
                                        Leaf @0 (#'!' @235-236: "!")
                                        Node @1 (tag: kFunctionCall) {
                                          Node @0 (tag: kReference) {
                                            Node @0 (tag: kLocalRoot) {
                                              Node @0 (tag: kUnqualifiedId) {
                                                Leaf @0 (#SymbolIdentifier @236-239: "rst")
                                              }
                                            }
                                          }
                                        }
                                      }
                                      Leaf @1 (#"&&" @240-242: "&&")
                                      Node @2 (tag: kUnaryPrefixExpression) {
                                        Leaf @0 (#'!' @243-244: "!")
                                        Node @1 (tag: kFunctionCall) {
                                          Node @0 (tag: kReference) {
                                            Node @0 (tag: kLocalRoot) {
                                              Node @0 (tag: kUnqualifiedId) {
                                                Leaf @0 (#SymbolIdentifier @244-246: "en")
                                              }
                                            }
                                          }
                                        }
                                      }
                                    }
                                  }
                                  Leaf @2 (#')' @247-248: ")")
                                }
                              }
                              Node @1 (tag: kIfBody) {
                                Node @0 (tag: kNonblockingAssignmentStatement) {
                                  Node @0 (tag: kLPValue) {
                                    Node @0 (tag: kReference) {
                                      Node @0 (tag: kLocalRoot) {
                                        Node @0 (tag: kUnqualifiedId) {
                                          Leaf @0 (#SymbolIdentifier @255-256: "q")
                                        }
                                      }
                                    }
                                  }
                                  Leaf @1 (#"<=" @257-259: "<=")
                                  Node @3 (tag: kExpression) {
                                    Node @0 (tag: kFunctionCall) {
                                      Node @0 (tag: kReference) {
                                        Node @0 (tag: kLocalRoot) {
                                          Node @0 (tag: kUnqualifiedId) {
                                            Leaf @0 (#SymbolIdentifier @260-261: "q")
                                          }
                                        }
                                      }
                                    }
                                  }
                                  Leaf @4 (#';' @261-262: ";")
                                }
                              }
                            }
                            Node @1 (tag: kElseClause) {
                              Leaf @0 (#"else" @267-271: "else")
                              Node @1 (tag: kElseBody) {
                                Node @0 (tag: kNonblockingAssignmentStatement) {
                                  Node @0 (tag: kLPValue) {
                                    Node @0 (tag: kReference) {
                                      Node @0 (tag: kLocalRoot) {
                                        Node @0 (tag: kUnqualifiedId) {
                                          Leaf @0 (#SymbolIdentifier @278-279: "q")
                                        }
                                      }
                                    }
                                  }
                                  Leaf @1 (#"<=" @280-282: "<=")
                                  Node @3 (tag: kExpression) {
                                    Node @0 (tag: kNumber) {
                                      Leaf @0 (#TK_UnBasedNumber @283-285: "\'x")
                                    }
                                  }
                                  Leaf @4 (#';' @285-286: ";")
                                }
                              }
                            }
                          }
                        }
                      }
                    }

*/
// -----------------------------------------------------------------------------
void IfMissingFinalElseRule::HandleSymbol(const verible::Symbol &symbol,
                                          const SyntaxTreeContext &context) {
  // Only interested in kConditionalStatement nodes.
  if (symbol.Kind() != verible::SymbolKind::kNode) return;
  const verible::SyntaxTreeNode &node = verible::SymbolCastToNode(symbol);
  if (!node.MatchesTag(NodeEnum::kConditionalStatement)) return;

  // TODO: walk the chain and report when it does not end in a plain else.
}

LintRuleStatus IfMissingFinalElseRule::Report() const {
  return LintRuleStatus(violations_, GetDescriptor());
}

}  // namespace analysis
}  // namespace verilog
