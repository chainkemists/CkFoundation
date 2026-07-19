---
name: ck-ship-pr
description: "Publish CK superproject and submodule work like ck-ship-dev, but submodules go out as feature-branch PRs for the maintainer to merge instead of direct dev pushes. Use when the user asks to ship via PRs, or when the session's maintainer has a standing PR-first preference."
---

# Ship via PRs — the review-first variant of ck-ship-dev

Follow [ck-ship-dev](../ck-ship-dev/SKILL.md) steps 0–4 and 7 EXACTLY (pre-flight,
commits, fetch + divergence table, rebase + backup branch, regate, final report).
This skill replaces ONLY the two publish steps — everything else stays single-sourced
in ck-ship-dev so the procedures never drift apart.

## 5-PR. Publish the submodules as PRs

Only after the gate is green and each repo is `N ahead / 0 behind`:

```bash
git -C <submodule> push origin <feature-branch>       # never dev directly
gh pr create --base dev --head <feature-branch> ...   # run inside the submodule
```

- Long-lived legacy branches (e.g. CkFoundation `feature/5.5-temporary`) are their
  own canonical target — push those direct, exactly as ck-ship-dev does.
- ck-ship-dev's withhold rule applies unchanged: don't publish a submodule a sibling
  session is actively mid-work in.

## 6-PR. Gitlink bumps wait for the merge

STOP until the maintainer merges each submodule PR: a superproject gitlink must
point at a SHA reachable from the submodule's `origin/dev`, and a squash-merge
REWRITES your SHAs. After the merge: fetch, re-point local dev at `origin/dev`,
re-verify containment (`git -C <sub> merge-base --is-ancestor <pointed-SHA>
origin/dev`), then bump exactly as ck-ship-dev step 6 prescribes.

The superproject publish follows the same default: feature-branch PR unless the user
explicitly asks for a direct dev push (or the superproject's flow is already
PR-gated, e.g. BusterBlock `feature/` branches → CI).
