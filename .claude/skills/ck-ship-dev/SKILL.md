---
name: ck-ship-dev
description: "Use when publishing CK superproject and plugin work through commit, fetch, rebase, post-rebase gates, submodule pushes, pointer bumps, and dev delivery."
---

# Ship dev — commit / fetch / rebase / regate / push / bring-dev-up / push-dev

Publishes the session's work across a CK superproject (CkPlugins, BusterBlock, …) and
its `Plugins/*` + `CkAuto/` submodules. Every step below exists because skipping it has
bitten before — do them in order and do not skip the regate.

**Scope rule (applies to every step):** stage only files this session authored.
Enumerate any other dirty path as "left untouched for its owning session" in the final
report. Never blanket `git add <dir>` — and never touch another session's untracked
files (e.g. a `CONTINUATION_PROMPT_*.md` parked in a submodule).

## 0. Pre-flight

- Probe the editor lock — file-mutating git ops and builds are guarded while the
  editor runs (`CkAuto/Check-UnrealNotRunning.ps1` hook):

  ```powershell
  try { [IO.File]::Open('<root>/Saved/Logs/<ProjectName>.log','Open','Write','None').Close(); 'free' } catch { 'locked' }
  ```

  If locked, wait for the editor to close (poll ~60 s); never kill it.
- CK repos run background processes (GC, LFS, fsmonitor) that hold `.git/index.lock`.
  Before each git write, wait for the lock to clear (retry loop, ~3 s intervals, give
  up loudly after ~20 tries).
- Superproject `.claude/*` is gitignored by design (machine-local; only
  `settings.json` is tracked). Anything meant to be versioned — skills, docs — belongs
  in a submodule repo, not the superproject `.claude/`.

## 1. Commit outstanding work (submodules first)

For each submodule with session work: `git status --short`, verify every dirty file is
yours, stage them **by name**, commit in the house style (see `git log --oneline -10`
for the active convention). Superproject config files the session's tooling wrote
(e.g. `Config/Default*.ini` changed by editor settings you clicked) get their own
small commits — flag them in the report so the user can drop unintended ones.

## 2. Fetch + divergence table

For the superproject and every submodule with local commits:

```bash
git -C <repo> fetch origin --quiet
git -C <repo> rev-list --left-right --count dev...origin/dev   # "ahead  behind"
```

Report the table before acting on it.

## 3. Rebase (only repos that are behind)

- **Backup branch first, always:** `git branch -f backup/dev-pre-rebase-<YYYY-MM-DD> dev`.
- `git rebase origin/dev dev`. On conflict: stop and resolve by hand — never
  `--skip` a commit, never force-continue.
- Rebase **rewrites your SHAs** — every downstream pointer bump must use the new tips,
  which is why superproject bumps happen at step 6, not before.

## 4. Regate on the rebased base (NOT optional)

A green run from before the rebase is stale-green — upstream commits now sit under
your code. Rebuild and re-run the affected test pattern via the toolbox
(never raw Build.bat / UnrealEditor-Cmd):

```powershell
./CkAuto/UnrealToolbox.exe --build --generate --config=Auto --target=Editor `
    --test --test-pattern <Pattern> --output=Saved/Logs/BuildTest-PostRebase.log `
    --project="<root>"
```

- Pass `--generate` if ANY `.Build.cs` / `.uplugin` changed — yours **or incoming**
  (check `git log --stat` of the incoming range if unsure; a new upstream module is
  the common case).
- Verdict = the log's trailing `=== Test summary ===` block (`Failed: 0`), not the
  exit code alone. Run it in the background and print the `Get-Content -Wait -Tail 50`
  one-liner for the user.
- Red gate ⇒ stop the ship, fix forward (or restore from the backup branch), regate.

## 5. Push the submodules

Only after the gate is green and each repo is `N ahead / 0 behind`:

```bash
git -C <submodule> push origin dev
```

Withhold a submodule's push (and say why) if a sibling session is actively mid-work
in that repo — a parked continuation file alone is fine (untracked files don't
publish); unpushed commits you don't recognize are not.

## 6. Bring dev up (superproject pointer bumps)

- **Cross-repo publish guard:** for every gitlink you are about to commit, confirm the
  pointed-at SHA is on the remote — `git -C <sub> merge-base --is-ancestor HEAD
  origin/dev` must succeed. Publishing a pointer to an unpushed SHA breaks
  `submodule update` for everyone.
- Stage ONLY the gitlink paths (`git add Plugins/<Name>` — the path itself, nothing
  recursive) plus nothing else, and commit:
  `chore(submodule): bump <names> — <one-line why>`.

## 7. Push dev + report

`git -C <root> push origin dev`, then report:

- per-repo pushed tip SHAs (old → new when rebased),
- the gate verdict line (`Passed: N / Failed: 0`),
- backup branch names,
- every dirty path deliberately left untouched and why.
