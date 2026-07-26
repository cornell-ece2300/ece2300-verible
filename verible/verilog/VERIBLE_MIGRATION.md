# ECE2300 Lint Tool — Verible Migration Tracking

**Goal:** replace the fragile custom Pyverilog fork with Verible for SystemVerilog
rule checking. Each rule is satisfied either by enabling an existing Verible
built-in, or by a custom C++ rule compiled into our Verible fork.

- Verible fork: `~/ece2300/ece2300-verible`
- Build script: `~/ece2300/verible-sandbox/build-verible.sh`
- Custom rules live in: `verible/verilog/analysis/checkers/<name>-rule.{h,cc}` (+ `_test.cc`)
- Course ruleset spec: `scripts/lint/lint_rules.py` (authoritative rule list)

> For a narrative explanation of how a rule works, how testing works, and how to run
> everything (professor-facing overview), see [`HOW_IT_WORKS.md`](./HOW_IT_WORKS.md).

_Last updated: 2026-07-20. Next up: **R404 PRIMONLY** (pairs naturally with R405 —
both inspect instantiations)._

**Status: 10 of 20 done.**

---

## Rules

Listed in `lint_rules.py` order. ✅ = working (built + tested) · ⬜ = not done.

### R1xx — Inferred Latch

| ID | Name | What it checks | Implementation | Status |
|---|---|---|---|---|
| R101 | LATCH | Every signal driven in an if-statement has a complete top-level default in `always_comb` | — | ⬜ |
| R102 | ASSIGNORDER | In `always_comb`, unconditional assignments come before any conditional | — | ⬜ |

### R2xx — X-Optimism / X-Propagation

The case side and if side mirror one-for-one: *fallback exists → fallback is all `'x` → every signal covered*.

| ID | Name | What it checks | Implementation | Status |
|---|---|---|---|---|
| R201 | CASEDEFAULT | Every `case` statement has a `default` case | built-in `case-missing-default` | ✅ |
| R202 | DEFAULTXONLY | The `default` case assigns only `'x` values | — | ⬜ |
| R203 | XASSIGN | Every signal assigned anywhere in the case is assigned `'x` in `default` | — | ⬜ |
| R204 | IFFINALELSE | Every if/else chain ends in a plain final `else` | `if-missing-final-else` (custom) | ✅ |
| R205 | ELSEXONLY | The final `else` clause assigns only `'x` values | — | ⬜ |
| R206 | IFXASSIGN | Every signal assigned anywhere in the if/else chain is assigned `'x` in the final `else` | — | ⬜ |

### R3xx — Always Block

| ID | Name | What it checks | Implementation | Status |
|---|---|---|---|---|
| R301 | ALWAYSFF | `always_ff` is disallowed (combinational / gate-level only) | `forbid-always-ff` (custom) | ✅ *(no unit test yet)* |
| R302 | ALWAYSSTAR | No generic `always @(...)`; use `always_comb`/`_ff`/`_latch` | `forbid-generic-always` (custom) | ✅ |
| R303 | BLKSEQ | No blocking `=` in `always_ff` | built-in `always-ff-non-blocking` | ✅ |
| R304 | NONBLKCOMBI | No non-blocking `<=` in `always_comb` | built-in `always-comb-blocking` | ✅ |
| R305 | ASYNCRESET | `always_ff` sensitivity list has only one edge term (no async reset) | `forbid-async-reset` (custom) | ✅ |
| R306 | NEGEDGE | No `negedge` in `always_ff`; only `posedge` | `forbid-negedge` (custom) | ✅ |

### R4xx — Gate Level / Structural

Two rulesets share these rules (see **Ruleset selection** in Notes):

- **`Struct`** = R401–R404, R406 — hierarchical modules; submodule instantiation **allowed**
- **`GL`** = `Struct` + **R405** — leaf modules; gate primitives only, **no** submodules

| ID | Name | What it checks | Implementation | Status |
|---|---|---|---|---|
| R401 | NOSPBLK | No `always`/`initial`/`function`/`task`/`generate`/system task call | `forbid-special-blocks` | ✅ |
| R402 | COMPLEXLHS | `assign` LHS is an identifier, `id[k]`, `id[msb:lsb]`, or a concatenation of those | — | ⬜ |
| R403 | COMPLEXRHS | `assign` RHS is the R402 set, or a simple literal | — | ✅ |
| R404 | PRIMONLY | Instantiated primitive is one of `and, or, not, xor, nand, nor, xnor` | — | ✅ |
| R405 | NOMODULE | No submodule instantiation — **`GL` ruleset only** | `forbid-module-instantiation`  | ✅ |
| R406 | LOGICINPORT | Port connections contain no operators (`~ & ^ …`); concatenations are fine | — | ✅ |

---

## Notes

**Retired rules.** `BADLHS` / `BADRHS` were removed on 2026-07-20: they fired when
the Pyverilog AST lacked a well-formed `Lvalue`/`Rvalue` — a malformed-parse guard.
Verible's parser rejects such an `assign` outright, so they could never fire.
R402–R406 were renumbered to close the gap.
The old `XPROP` / `WRONGXPROP` macro rules and the
`CASEINCOMPLETE` / `IFINCOMPLETE` split-out rules were removed on 2026-07-19.
Coverage checking now lives in R203/R206, which catch both failure modes (signal
missing from the fallback, and signal present but not assigned `'x`). Note the
R2xx block was renumbered at the same time — configs written before this date
may reference stale IDs.

**Open question — R203 / R206 (whole-signal vs bit-level).** Comparing whole
signal names is straightforward. Bit-level coverage (`q[3:2]` assigned but
`q[1:0]` not) requires signal widths plus constant-folding of index expressions
(`q[i]`, `q[WIDTH-1:0]`), and is undecidable for a variable index (`q[sel]`) —
that is elaboration, which Verible deliberately does not do. Recommended escape
hatch: additionally require that assignments in these blocks target whole signals
(no part-selects), which makes whole-signal comparison sound by construction. A
survey of the labs shows part-select LHS in student RTL is essentially
nonexistent, so this costs nothing in practice. **Needs a decision from the
professor.**

**Ruleset selection — NOT derivable from filenames.** Which ruleset a module gets
(`GL` / `Struct` / `RTL`) comes from an explicit per-module table in
`scripts/lint/rulesets.yaml`. It deliberately cuts across the `_GL`/`_RTL` naming:

- `MultiNotePlayer_RTL`, `MusicPlayer_RTL`, `MusicPlayerDpath_RTL`, `NotePlayer_RTL`,
  `Synchronizer_RTL` are **`Struct`** despite the `_RTL` suffix
- `AccumXcel`, `ALU_32b`, `ProcScycle`, `ProcSimple` are `Struct` with **no suffix**
- `DFF_GL`, `DFFR_GL`, `Counter_16b_GL`, `Mux2_8b_GL` are `Struct`, **not** `GL`

So the wrapper must read the yaml map; a filename heuristic would be wrong.
**Mechanism deferred** — decide after the R4xx rules themselves exist.

**`rulesets.yaml` needs a pass.** It references `CASEINFER` and
`SignalAssignInDefault`, which do not exist in `lint_rules.py`, plus `XPROP` /
`WRONGXPROP`, which were retired. Its `Struct` list also still names `BADLHS` /
`BADRHS`.

**Lab 1 module classification** (first migration target):
`GL` → `BinaryToBinCodedDec_GL`, `BinaryToSevenSegOpt_GL`, `BinaryToSevenSegUnopt_GL`;
`Struct` → `DisplayOpt_GL`, `DisplayUnopt_GL`.

**R301 has no unit test.** It predates the test pattern; add
`forbid-always-ff-rule_test.cc` to bring it in line with the other custom rules.

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
