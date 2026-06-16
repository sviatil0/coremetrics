# Contributing

Welcome. This document is the short version of how to build, test, and ship a change in this repo.

## Quick start

```bash
git clone https://github.com/sviatil0/coremetrics.git
cd coremetrics

# Build only (no auto-run):
make bin/coremetrics
make bin/tests

# Build and run:
make test          # runs the full test suite
./bin/coremetrics  # launches the live UI
```

Dependencies are SDL3, SDL3_ttf, and SDL3_image. The `Makefile` is preconfigured for macOS Homebrew layouts (`-I/opt/homebrew/include` and `-L/opt/homebrew/lib`); swap the paths or use the commented-out `pkg-config`-based block for Linux.

CI checks the build on Linux, macOS, and Windows. All three legs must stay green before a PR is mergeable.

## Sanitizers

The test target can be rebuilt with AddressSanitizer or UndefinedBehaviorSanitizer:

```bash
make clean
make asan         # builds + runs tests with -fsanitize=address
# or
make clean
make ubsan        # builds + runs tests with -fsanitize=undefined
```

ASan flags use-after-free, double-free, heap-buffer-overflow, and stack-buffer-overflow. UBSan flags signed-integer overflow, divide-by-zero, null-pointer-deref, and a handful of other undefined behaviors. Both targets exit with a non-zero status on any flagged behavior, so they can be wired into pre-merge checks once the suite is clean.

## Branch flow

Branches are cut from `dev`, not from `main`. The flow is:

```
main  <--- dev  <--- feat/<short-name>
                <--- fix/<short-name>
                <--- chore/<short-name>
```

`main` is release-quality. `dev` is the integration branch. Every feature, fix, or chore lives on its own branch from `dev` and lands via a PR back into `dev`. The `dev -> main` promotion is a separate PR cut at release time.

## Pull requests

- One unit of work per PR. If it grew, split it.
- The PR title is a Conventional Commit subject (`feat(metrics): ...`, `fix(ui): ...`, `test(parsers): ...`, `chore(ci): ...`, `docs(readme): ...`).
- The PR body has a Summary (bulleted, focus on the why) and a Test plan (checklist).
- Target branch is `dev`. Never PR directly into `main`.
- No `Co-Authored-By` trailers. The author identity in the commit is the source of truth for the contribution graph.

## Coding style

C++23, no new third-party dependencies beyond SDL3, SDL3_ttf, and SDL3_image. The existing style is Allman braces, header guards with `#ifndef __X_HPP__`, `camelCase` identifiers, named constants instead of magic numbers, no lambdas in the application code (lambdas inside `ThreadPool::submit` are the exception), and no em-dashes anywhere in code, docs, or commit messages.

Optionally install pre-commit (`pip install pre-commit`) and run `pre-commit install` to get the project's style checks (no em-dashes, trailing whitespace, valid YAML) on every commit.

## Tests

New code adds tests. Pattern for a new suite:

1. `include/MyThingTest.hpp` declares `void myThingTestSuite();`
2. `tests/MyThingTest.cpp` defines suite functions that print `PASS`/`FAIL` lines
3. `tests/tests.cpp` includes the header and calls the suite from `main`

The full suite must stay green on macOS and Linux for the PR to land. ASan and UBSan should stay green for any code that touches metrics, parsers, or memory ownership.

## Releasing

A release is a PR from `dev` into `main`. It does not introduce new work, only collects what already landed on `dev`.

## Multi-agent workflow

A lot of this repo's velocity comes from running parallel Claude Code subagents on worktrees, each owning a disjoint slice of the codebase, opening a separate PR. The branching rules in [Release branch flow](#release-branch-flow) still hold: every agent branch is cut from `dev`, every PR targets `dev`, and only `dev -> main` is promoted on a tagged release. Conflicts between concurrent agent PRs are resolved by rebasing the later PR onto the merged trunk. The contribution badge workflow now re-fetches main before pushing so it tolerates the race.

You do not need to use the agent workflow to contribute. A single PR cut from `dev` is welcome and reviewed the same way.
