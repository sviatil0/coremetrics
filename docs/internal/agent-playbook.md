# Agent Playbook

Internal reference for running multi-agent parallel PR waves against this repo.
Aimed at future maintainers (human or otherwise) who want to ship many small,
atomic changes concurrently without losing the branching invariants.

## Why agent waves

A wave is a batch of independent agents, each producing one PR. When the work
is shaped right (disjoint files, clear scope, no cross-agent state), throughput
goes up sharply because review, CI, and rebase costs amortize across the batch
instead of stacking serially.

The cost is coordination. This playbook is the coordination contract.

## Hard scope rule

Every agent gets a single, explicit "ONLY touch" file list in its prompt.
Agents must refuse to edit anything outside that list, even when a nearby file
"obviously" needs a one-line tweak. Cross-cutting changes are scoped to their
own dedicated agent.

Two agents touching the same file is the failure mode this rule exists to
prevent. If you cannot guarantee disjoint file sets, do not launch the wave.

## Branching

- Cut every feature branch from `dev`, never from `main`.
- Branch name follows the conventional commit type: `feat/...`, `fix/...`,
  `docs/...`, `chore/...`, `ci/...`, `refactor/...`, `test/...`,
  `tools/...`, `bench/...`, `packaging/...`.
- Open the PR against `dev`. Only `dev` promotes to `main`, and only at
  tagged releases.

## Worktree isolation

Each agent runs inside its own git worktree under
`.claude/worktrees/agent-<id>/`. Worktrees are locked so they survive across
sessions. Benefits:

- Independent working trees, so concurrent edits never collide on disk.
- Each agent gets its own branch checkout without disturbing the main
  checkout.
- Failed runs can be inspected post-mortem without blocking the next wave.

Do not delete a locked worktree until the PR is merged or explicitly
abandoned.

## Conventional commits

Required format: `type(scope): subject`. No em-dashes anywhere in commit
messages, PR titles, or PR bodies. Use `--`, commas, colons, or parentheses
instead. The repo lint catches stray U+2014 characters.

Author identity is locked to:

```
Sviatoslav Oleksiienko <soleksiienko1@gmail.com>
```

Agents must not append `Co-Authored-By:` trailers. The contribution badge
workflow attributes commits by author email; foreign trailers pollute the
breakdown.

## Author identity injection per PR

Before any commit, agents verify `git config user.name` and
`git config user.email` match the locked identity. The hook layer rejects
commits with mismatched identity so a misconfigured worktree fails loudly
rather than producing ghost authors.

## Conflict resolution

When two agents land work that touches the same file (the rule should
prevent this, but reality intervenes):

1. The first PR merges normally.
2. The second PR rebases onto the new `dev` tip: `git fetch origin dev &&
   git rebase origin/dev`.
3. The agent resolves conflicts inside its worktree and force-pushes the
   rebased branch.
4. CI re-runs. Do not merge until green.

Never merge `dev` into the feature branch. Always rebase. This keeps the
history linear and the contribution audit clean.

## Contribution badge race fix

The contribution-badge workflow rewrites `README.md` on every push to `dev`.
Parallel merges used to race and clobber each other. The current workflow
retries on push rejection by fetching `dev`, rebasing the badge commit, and
re-pushing. If you see badge drift, check the workflow run logs for the
rebase loop, not the badge generator.

## PostToolUse security review

The PostToolUse hook runs a lightweight security scan after each commit.
When it surfaces a finding:

- If actionable, fix it in the same branch before opening the PR.
- If it is a false positive or accepted risk, acknowledge it explicitly in
  the PR body under a `## Security review` heading, citing the finding and
  the reason for accepting it.

Silently ignoring findings is grounds for rejection at review.

## PR review flow

- Keep PRs small and atomic. One concern per PR.
- Bundle related small PRs into a review session, not into a single mega PR.
- If a PR sits behind newer merges, rebase it onto current `dev` before
  asking for review.
- Squash-merge into `dev` so the trunk history stays one commit per PR.
- `dev` promotes to `main` only at release tags, never per patch.

## Quick checklist before launching a wave

- Disjoint file sets per agent, written into each prompt.
- Each branch cut from current `origin/dev`.
- Author identity verified in each worktree.
- Conventional commit type matches the branch prefix.
- No em-dashes anywhere.
- PR opens against `dev`.
