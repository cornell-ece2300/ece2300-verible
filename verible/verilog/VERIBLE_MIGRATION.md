# ECE2300 Lint Tool — Verible Migration Tracking

**Goal:** replace the fragile custom Pyverilog fork with Verible for SystemVerilog
rule checking. Rules are either (a) an existing Verible built-in we just enable,
(b) a custom C++ rule compiled into our Verible fork, or (c) dropped.

- Verible fork: `~/ece2300/ece2300-verible`
- Build script: `~/ece2300/verible-sandbox/build-verible.sh`
- Custom rules live in: `verible/verilog/analysis/checkers/<name>-rule.{h,cc}` (+ `_test.cc`)
- Course ruleset spec: `scripts/lint/lint_rules.py`

Status legend: ✅ done · 🟡 in progress · ⬜ not started · ➖ dropped

> For a narrative explanation of how a rule works, how testing works, and how to run
> everything (professor-facing overview), see [`HOW_IT_WORKS.md`](./HOW_IT_WORKS.md).

_Last updated: 2026-07-12. Next up: gate-level cluster (**R401 / R407**) or **R102 ASSIGNORDER**._

---

## Summary

| Bucket | Count | Rules |
|---|---|---|
| ✅ Done (custom) | 4 | R301, R302, R305, R306 |
| Enable built-in (no code) | 3 | R201, R303, R304 |
| ➖ Dropped | 2 | R204, R205 |
| ⬜ To implement (custom) | 11 | R101, R102, R202, R203, R401–R408 |

---

## 1. Done — custom rules written, built, tested

| ID | Name | Rule file | Verible rule name | Test |
|---|---|---|---|---|
| R301 | ALWAYSFF | `forbid-always-ff-rule.{h,cc}` | `forbid-always-ff` | ⬜ (add `_test.cc`) |
| R302 | ALWAYSSTAR | `forbid-generic-always-rule.{h,cc}` | `forbid-generic-always` | `_test.cc` ✅ |
| R306 | NEGEDGE | `forbid-negedge-rule.{h,cc}` | `forbid-negedge` | `_test.cc` ✅ |
| R305 | ASYNCRESET | `forbid-async-reset-rule.{h,cc}` | `forbid-async-reset` | `_test.cc` ✅ |

## 2. Covered by Verible built-ins — enable in config, no code

| ID | Name | Built-in rule | Notes |
|---|---|---|---|
| R201 | CASEDEFAULT | `case-missing-default` | Exempts `unique case` (correct). |
| R303 | BLKSEQ | `always-ff-non-blocking` | Allows blocking to *local* vars; tighten with `catch_modifying_assignments` if R303 must be strict. |
| R304 | NONBLKCOMBI | `always-comb-blocking` | Exact match. |

## 3. Dropped

| ID | Name | Reason |
|---|---|---|
| R204 | XPROP | Macro-based X-optimism; being removed from ruleset. |
| R205 | WRONGXPROP | Same. |

## 4. To implement — custom rules needed

### Medium (shape/pattern check on one construct)
| ID | Name | Check | Status |
|---|---|---|---|
| R408 | LOGICINPORT | Port-connection expr contains no operators (`~ & ^ …`) | ⬜ |
| R407 | NOMODULE | No module instantiation in a gate-level module | ⬜ |
| R406 | PRIMONLY | Only `and/or/xor/not` (+variants) as gate primitives | ⬜ |
| R401 | NOSPBLK | No `always/initial/function/task/generate/$system` in gate-level module | ⬜ |
| R404 | COMPLEXLHS | `assign` LHS is a single signal or part-select | ⬜ |
| R402 | BADLHS | `assign` LHS operand is a wire/primitive (lvalue) | ⬜ |
| R403 | BADRHS | `assign` RHS operand is a wire/primitive (rvalue) | ⬜ |
| R405 | COMPLEXRHS | `assign` RHS is identifier, `id[msb:lsb]`, or simple literal | ⬜ |
| R102 | ASSIGNORDER | In `always_comb`, unconditional assigns precede conditionals | ⬜ |

### Hard (cross-statement data-flow / completeness analysis)
| ID | Name | Check | Status |
|---|---|---|---|
| R101 | LATCH | Every bit of an if-driven signal has a top-level default in `always_comb` | ⬜ |
| R203 | CASEINCOMPLETE | Every signal assigned in any case-item is also assigned in `default` | ⬜ |
| R202 | XASSIGN | Every case-driven signal is assigned `'x` in `default` | ⬜ |

**Notes**
- R401–R408 are **gate-level-only**: they should fire only on `_GL.v` files. Since labs are
  one-module-per-file with `_GL`/`_RTL` naming, this is a filename gate on the wrapper plus a
  shared "is this a gate-level module?" guard — build them as a cluster.
- The Hard three (R101/R202/R203) need a helper that walks a block and accumulates per-signal
  assignment coverage. Prototype this before committing the whole ruleset to C++.

---

## Build / test / run quick reference

```bash
# Build the fork (incremental)
~/ece2300/verible-sandbox/build-verible.sh
BIN=~/ece2300/ece2300-verible/bazel-bin/verible/verilog/tools/lint/verible-verilog-lint

# Run ONE custom rule on a file (all default rules off)
$BIN --ruleset=none --rules=forbid-generic-always FILE.v

# Inspect the parse tree when designing a rule
~/ece2300/verible-sandbox/verible-bin/bin/verible-verilog-syntax --printtree FILE.v

# Run a rule's unit test
cd ~/ece2300/ece2300-verible
~/ece2300/verible-sandbox/bazel-7.6.0 --output_base=/tmp/$USER-verible-bazel \
  test --linkopt=-lstdc++fs \
  //verible/verilog/analysis/checkers:forbid-generic-always-rule_test
```

## Anatomy of a custom rule (checklist for each new rule)
1. `verible/verilog/analysis/checkers/<name>-rule.h` — class extends `SyntaxTreeLintRule`.
2. `<name>-rule.cc` — `HandleSymbol()` does the check; `VERILOG_REGISTER_LINT_RULE(...)`.
3. `<name>-rule_test.cc` — `RunLintTestCases<VerilogAnalyzer, Class>({...})`.
4. `BUILD`: add `cc_library` (**`alwayslink = 1`**) + `cc_test`, and add the lib to the
   `verilog-lint-rules` aggregator deps.
5. Build, then `--rules=<name>` on a sample file to confirm it fires.
