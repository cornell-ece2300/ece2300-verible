# ECE2300 Verible Lint Rules — How It Works

A walkthrough of how our custom SystemVerilog lint rules are written, tested, and
run on top of Verible. Written to be read start-to-finish; no prior Verible
knowledge assumed. For the per-rule status/roadmap, see
[`VERIBLE_MIGRATION.md`](./VERIBLE_MIGRATION.md).

---

## 1. Background: why Verible

The course lint tool enforces ECE2300 coding conventions (no generic `always`,
no blocking assignment in `always_ff`, gate-level modules use only primitives,
etc.). It previously relied on a **custom fork of Pyverilog** — a SystemVerilog
parser we had to keep patching because it did not fully support the language.
Maintaining a parser is not the point of the course, and every new lab feature
risked breaking it.

**Verible** is Google's open-source SystemVerilog parsing-and-linting toolkit,
actively maintained and used in industry. It already parses the full language
correctly (it reads 614 of our 615 lab files with zero errors — the one failure
is a genuine typo in an old file). Instead of maintaining our own parser, we
write our lint *rules* on top of Verible's parser.

Each ECE2300 rule is satisfied one of three ways:

1. **Enable a Verible built-in** — Verible ships ~60 rules; a few already match
   our conventions exactly, so we just turn them on in config. No code.
2. **Write a custom rule** — a small C++ file compiled into our Verible build.
3. **Drop** — rules we've decided to retire.

The status of all 20 rules is tracked in `VERIBLE_MIGRATION.md`. This document
explains **how a custom rule actually works**, using our simplest real rule as
the running example.

---

## 2. How Verible sees the code: the syntax tree

When Verible parses a source file, it builds a **Concrete Syntax Tree (CST)** — a
tree that mirrors the grammar of the code. Every construct is a **node**
(e.g. an `always` statement, a case statement, a port list), and every actual
piece of text — a keyword, an identifier, `<=`, `;` — is a **leaf** holding one
token.

You can see this tree for any file (the `verible-verilog-syntax` tool ships
prebuilt in the sandbox — see §5 for putting it on your PATH):

```bash
~/ece2300/verible-sandbox/verible-bin/bin/verible-verilog-syntax --printtree FILE.v
```

For example, `always @* y = a & b;` produces (abridged):

```
Node @0 (tag: kAlwaysStatement)
  Leaf @0 (#always "always")        <-- the keyword token
  Node @1 (tag: kEventControl) ...
  Node @2 (tag: kBlockingAssignment) ...
```

The key facts a rule relies on:

- **A node has a *tag*** identifying what it is (`kAlwaysStatement`,
  `kCaseStatement`, …).
- **A leaf has a *token type*** identifying the exact keyword/symbol
  (`TK_always`, `TK_always_ff`, `TK_negedge`, `<=` is `TK_LE`, …).
- Every leaf knows its **byte range** in the source file, which is how a
  diagnostic reports a precise `file:line:column`.

A lint rule is just code that walks this tree looking for a specific shape.

---

## 3. How a custom rule works

### 3.1 The dispatch model

Verible runs all active rules in a **single pass** over the tree. It visits every
node and every leaf once, and for each one it calls a method on every rule:

```cpp
void HandleSymbol(const Symbol& symbol, const SyntaxTreeContext& context);
```

There is **no "give me only the always-statements" filtering** — every rule is
offered *every* tree element, and each rule is responsible for checking "is this
the thing I care about?" and ignoring everything else. This keeps rules
independent and the traversal simple.

(`Symbol` is the common base type for "a node or a leaf." A rule first asks which
one it got, then inspects accordingly.)

### 3.2 A complete rule, line by line

Our rule **R302 — forbid generic `always`** flags any bare `always @(...)` block
and tells the student to use `always_comb` / `always_ff` / `always_latch`
instead. Here is its entire logic (from
`analysis/checkers/forbid-generic-always-rule.cc`):

```cpp
void ForbidGenericAlwaysRule::HandleSymbol(const verible::Symbol &symbol,
                                           const SyntaxTreeContext &context) {
  // (1) Ignore leaves; we only care about nodes here.
  if (symbol.Kind() != verible::SymbolKind::kNode) return;

  // (2) Is this node an `always` statement? If not, ignore it.
  const verible::SyntaxTreeNode &node = verible::SymbolCastToNode(symbol);
  if (!node.MatchesTag(NodeEnum::kAlwaysStatement)) return;

  // (3) The first leaf of an always-statement is the keyword itself:
  //     always / always_ff / always_comb / always_latch.
  const verible::SyntaxTreeLeaf *keyword = verible::GetLeftmostLeaf(symbol);
  if (keyword == nullptr) return;

  // (4) Only the *bare* `always` keyword is a violation. The typed forms
  //     (always_ff etc.) are different token types and are left alone.
  if (keyword->get().token_enum() == TK_always) {
    // (5) Record a violation pointing at the keyword token, so the message
    //     appears at exactly that file:line:column.
    violations_.insert(LintViolation(keyword->get(), kMessage, context));
  }
}
```

That's the whole method. The pattern generalizes to most of our remaining rules:

1. **Filter by kind** (node vs leaf).
2. **Filter by tag/token** (is this the construct I check?).
3. **Drill to the relevant token(s)**.
4. **Decide** whether it violates the rule.
5. **Record a violation** anchored at a source location.

The violations a rule collects are reported back at the end via a `Report()`
method. A one-line registration macro, `VERILOG_REGISTER_LINT_RULE(...)`, makes
the rule available to the linter under the name `forbid-generic-always`.

### 3.3 What the rule produces

```
$ verible-verilog-lint --ruleset=none --rules=forbid-generic-always test.v
test.v:18:3-8: Generic 'always @(...)' block found. Use 'always_comb', ... [forbid-generic-always]
test.v:22:3-8: Generic 'always @(...)' block found. Use 'always_comb', ... [forbid-generic-always]
test.v:26:3-8: Generic 'always @(...)' block found. Use 'always_comb', ... [forbid-generic-always]
```

`18:3-8` is the exact span of the offending `always` keyword. The tag in brackets
is the rule name. The linter exits non-zero when it finds violations, which is
how a grading/CI script detects failure.

---

## 4. How testing works

Every custom rule ships with an automated unit test. The test **runs the real
rule** on small SystemVerilog snippets and checks that the violations it produces
are *exactly* the ones we expect — no missing ones, no extra ones.

### 4.1 The idea: an inline answer key

A test case is a snippet of Verilog written as a list of text fragments. A plain
string is ordinary source text. A fragment wrapped as `{TK_always, "always"}` is
**both source text and a marker saying "a violation must occur right here."**

```cpp
// Should be flagged: the marker says "expect a violation on this `always`".
{"module m; logic y, a, b; ",
 {TK_always, "always"},
 " @* y = a & b; endmodule"},

// Should NOT be flagged: no marker anywhere means "expect zero violations".
{"module m; logic y, a, b; always_comb y = a ^ b; endmodule"},
```

The harness glues the fragments into one source string, parses it, runs the rule,
and then compares:

- **What the rule reported** (each violation has a byte range + token), against
- **What we marked** (each marker has a byte range + token).

They must match *exactly* — same location, same token. A mismatch fails the test
and prints the difference. Two things this catches automatically:

- **Over-flagging** — if the rule wrongly flagged the `always_comb` case, that
  violation has no matching marker → test fails.
- **Under-flagging** — if the rule missed a marked `always`, that marker has no
  matching violation → test fails.

So the marker-free cases (like `always_comb`, `always_ff`, `always_latch`) are
real tests: they assert the rule stays silent where it should.

### 4.2 Our test cases

`forbid-generic-always-rule_test.cc` covers, in one test:

| Snippet | Expected |
|---|---|
| empty file / module with nothing | no violations |
| `always_comb`, `always_ff`, `always_latch` | no violations (typed forms are allowed) |
| `always @*`, `always @(a or b)`, `always @(posedge clk)` | one violation each |
| two generic `always` in one module | two violations |
| mix of `always_comb` + generic `always` | only the generic one flagged |

A green run means the rule behaved correctly on every one of these.

---

## 5. How to run everything

All paths are on the ecelinux server.

### One-time PATH setup (for the prebuilt helper tools)

The stock Verible tools (`verible-verilog-syntax`, etc.) ship prebuilt in the
sandbox but are not on PATH by default. Add them once per shell:

```bash
export PATH="$HOME/ece2300/verible-sandbox/verible-bin/bin:$PATH"
```

Note: these prebuilt binaries are the stock release and do **not** contain our
custom rules — use them for tree inspection (`--printtree`). To *run* our rules,
use the freshly built `$BIN` below.

### Build the fork (incremental — fast after the first time)

```bash
~/ece2300/verible-sandbox/build-verible.sh
BIN=~/ece2300/ece2300-verible/bazel-bin/verible/verilog/tools/lint/verible-verilog-lint
```

### Lint a file

```bash
# Run only one custom rule (all other rules off) — good for demos:
$BIN --ruleset=none --rules=forbid-generic-always FILE.v

# Run several rules at once:
$BIN --ruleset=none --rules=forbid-generic-always,forbid-always-ff FILE.v
```

### Run a rule's unit test

```bash
cd ~/ece2300/ece2300-verible
~/ece2300/verible-sandbox/bazel-7.6.0 --output_base=/tmp/$USER-verible-bazel \
  test --linkopt=-lstdc++fs --test_output=all \
  //verible/verilog/analysis/checkers:forbid-generic-always-rule_test
```

A passing run prints `[ PASSED ] 1 test.`. (Bazel caches results, so an unchanged
re-run prints `(cached)` and does not re-execute — add `--nocache_test_results`
to force it.)

### Inspect the parse tree (how rules are designed)

```bash
verible-verilog-syntax --printtree FILE.v
```

This is the starting point for writing any new rule: look at the tree for the
construct you want to catch, find its node tag and the token you need, then write
the five-step `HandleSymbol` from §3.2.

---

## 6. Current status

Two custom rules are complete (R301, R302), three map to Verible built-ins, two
are dropped, and ~13 remain — of which only three are genuinely hard (they need
cross-statement data-flow analysis). The full per-rule breakdown, difficulty
ratings, and next steps are in [`VERIBLE_MIGRATION.md`](./VERIBLE_MIGRATION.md).
