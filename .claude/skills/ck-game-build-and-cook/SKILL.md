---
name: ck-game-build-and-cook
description: 'Use when building, cooking, staging, or packaging a CkFoundation game on Windows, including plugin script staging, fresh-workspace cooks, and stale outputs.'
---

# ck-game-build-and-cook — the build → cook → stage → package pipeline for a CkFoundation game

## Overview

Packaging a CkFoundation game is standard Unreal `BuildCookRun` **plus three framework-specific
obligations** that stock UE documentation will never tell you about:

1. **AngelScript is not cooked content.** The AS runtime compiles `.as` source at boot, from loose
   files. The engine fork stages the *project's* `Script/` automatically; every **plugin** that
   ships AS (CkFoundation itself, at minimum) must be staged explicitly or your packaged build
   fails AS compilation at boot — often refusing to boot at all (§2).
2. **Test/gym content must be excluded from the cook**, and CkTests from Shipping/Test targets —
   otherwise placed test actors reference classes that don't exist in the packaged runtime (§3).
3. **The first cook in a fresh workspace may legitimately fail.** Generated spawn-params AS files
   are gitignored and self-healed; the heal is inherently two-pass and both passes must run in the
   *same* workspace (§4).

Everything else — DLL locks, stale outputs, PIE-vs-packaged divergence — is shared with normal UE
development but has sharper teeth here; this skill owns the pipeline-level view and routes the
diagnosis to the owning skill.

Jargon used below, defined once:
- **Cook** — the editor commandlet that converts editor assets (`.uasset`/`.umap`) into
  platform-optimized cooked content under `Saved/Cooked/<Platform>/`.
- **Stage** — copying cooked content + binaries + config into a runnable layout
  (`Saved/StagedBuilds/`); **pak** bundles staged content into `.pak`/IoStore containers.
- **NonUFS** — files staged loose on disk rather than inside the pak ("non-Unreal-File-System").
  AS source must be NonUFS: the script compiler reads real files, not pak entries.
- **EntitySpawnParams (ESP)** — generated AS accessors for entity spawn parameters
  (`Script/Generated/*_EntitySpawnParams.as`), regenerated every editor boot, deliberately
  gitignored, and rebuilt by the framework's self-heal dispatcher when absent.

## When NOT to use this skill

| You are actually trying to… | Load instead |
|---|---|
| Set up the engine/submodules, fix "modules missing or built with a different engine version", register an engine GUID, add a C++ module | `ck-build-and-env` |
| Diagnose a packaged crash (0xC0000005 GC class), an AS error wall, UBT/linker failures beyond the environment | `ck-debugging-playbook` (framework) / `ck-game-debugging-playbook` (game symptoms) |
| Wire a brand-new project from an empty repo (uproject, Config, first entity) | `ck-game-project-bootstrap` |
| Author or run tests against the editor (no packaging involved) | `ck-game-testing-discipline` |
| Ship via a store SDK, CI service, or distribution channel | Your project's local skill — see §7 boundary |

---


## Reference files — load only what the task needs

Section numbers cited elsewhere in this skill point into these files.

| Topic | Read |
|---|---|
| The pipeline, AS staging, test exclusion, two-pass cooks | `references/pipeline-and-staging.md` |

## 5. Stale-output traps — what to nuke, when

| Symptom | Stale artifact | Fix | Safe? |
|---|---|---|---|
| Packaged build shows old assets/behavior despite fresh edits; a re-stage without recook ships yesterday's content | `Saved/Cooked/<Platform>/` reused by `-skipcook` or an iterative cook | Delete `Saved/Cooked/` (or run the cook without iterate flags), recook | Always safe — cost is one full recook |
| Editor boot after a branch/submodule switch: "modules … built with a different engine version" naming Ck modules | Plugin `Binaries/` + `Intermediate/` from the previous branch | Owned by `ck-build-and-env` §6 **T1** — delete per-plugin `Binaries/`+`Intermediate/`, regenerate, rebuild. Do NOT delete `Saved/` or `DerivedDataCache/` for this class | Safe; DDC/Saved deletion is the classic wrong move |
| A "green" build/test run predating your last edit is being cited as evidence | The binary itself | **Stale-green rule** — a build that predates your edit proves nothing; static-init registrations baked into an old binary actively lie. Canonical telling: `ck-debugging-playbook` → Common mistakes → Stale-green; verification-gate discipline: `ck-change-control` §"What done requires" | — |
| Cook fails only in CI fresh clones, passes locally | Absent gitignored ESP (your local workspace healed long ago) | §4 two-pass rule | — |

Rule of thumb: `Saved/Cooked` is cheap and always regenerable; `Intermediate/` deletion follows
the escalation ladder in `ck-debugging-playbook` §1.3; `DerivedDataCache/` is expensive and almost
never the culprit.

---

## 6. DLL locks and multi-session machines

- **LNK1104 "cannot open file …dll"** during any build = something has the DLL mapped. Prime
  suspects: a running editor for this project, or a second session/working copy building against
  the same binaries. Find the holder: `ck-debugging-playbook` §1.2 (the log-write-lock probe —
  process-name scans lie under renamed fork binaries). Guard scripts and multi-session rules
  (`-skipcompile` on headless boots, check for MSBuild/UBT before building): `ck-build-and-env`
  §6 T2–T4.
- **Never build while the editor holds DLLs** — beyond the link failure, a killed UBT deletes
  stale outputs before relinking, leaving modules deleted-but-not-relinked (T3), and a mid-edit
  hot-reload half-state compounds the stale-green problem (§5).
- Cook/stage/package runs are builds too from a lock perspective: `-build` inside `BuildCookRun`
  invokes UBT, and the cook commandlet *is* an editor process — a second interactive editor open
  on the same project while a cook runs invites both lock contention and asset-save races.

---

## 7. Packaged-vs-PIE verification duty

**A feature is not done until it has been exercised in a packaged or Test-config build, at
minimum at boot level.** PIE structurally cannot represent the packaged runtime:

- **Ensure visibility differs.** In Test/Shipping the ensure predicate still evaluates and the
  recovery block still runs — with zero logging/dialog. "No ensure fired" in a packaged Test run
  means nothing without the log-grep discipline. Matrix and consequences:
  `ck-debugging-playbook` §6.5; define matrix: `ck-macros-and-codegen` §2.4.
- **GC verifier behavior differs**, and the packaged-only 0xC0000005 GC crash class does not
  reproduce in PIE at all. Symptoms and the discriminating experiment
  (`Ck.Diag.VerifyGCAssumptions`): `ck-debugging-playbook` §5; game-side symptom routing:
  `ck-game-debugging-playbook`.
- Cook-time stripping, discovery/scan differences, cooked-ini layering — the full 6-axis
  checklist is `ck-debugging-playbook` §6.

Minimum packaged boot gate (agent-runnable):

```powershell
# From the archived build folder: launch with logging, then read the log — the exit alone is not a verdict.
& ".\<Game>.exe" -log
# Then grep <archive>\<Game>\Saved\Logs\<Game>.log for:
#   "Angelscript: Error"  → AS staging/compile failure (§2)
#   fatal/callstack lines → route to ck-debugging-playbook §5/§6
```

(Corpus example (BusterBlock): `CkAuto/CkRun_LogOnly.bat` is exactly this — launch the packaged
exe with `-log`.)

`[EDITOR-VERIFY]` — what an agent cannot do: interactive gameplay in the packaged build (input
feel, UI flows, store furniture interactions). Hand the user: 1) which archived build folder to
run, 2) the exact flow to drive, 3) which log lines/breadcrumbs prove the path executed. A
packaged boot that reaches the main menu exercises module load, AS staging+compile, cooked-asset
load, and static-init registration — that is the floor, not the ceiling.

---

## 8. Where the generic flow ends

Everything above is engine + framework: any CkFoundation game on any infrastructure needs it.
Everything *beyond* the archive directory is project territory: store SDKs (Steam/EGS
integration, uploads), CI services (Buildkite, GitHub Actions), crash-symbol upload (Sentry
etc.), launchers, and distribution. Per the campaign's PROJECT_TEMPLATE local-skill policy
(`Plugins/CkFoundation/.claude/PROJECT_TEMPLATE/`), those live in a **project-local skill** in the
consuming game's repo — the corpus artifacts you may encounter in BusterBlock (`.runreal/`
pipelines, `deploy-egs.ts`/`deploy-steam.ts`, Sentry wiring, PhantomLauncher) are exactly that
residue and are not part of this skill's contract.

---

## Common mistakes

- Packaging without the plugin `Script/` staging lines, then triaging the resulting AS error wall
  one error at a time. Check `DirectoriesToAlwaysStageAsNonUFS` first (§2).
- Treating a fresh-clone cook exit 3 as a real failure and "fixing" it by committing the generated
  ESP files — that creates the wedge that blocks self-heal permanently (§4).
- Letting CI retry the cook with a per-attempt clean checkout — every attempt is pass 1 forever (§4).
- First-ever Shipping/Test build attempted without the CkTests `DisablePlugins` block — fails on
  non-redistributable module rejection, or worse, ships test infrastructure (§3).
- Re-staging with `-skipcook` after content edits and wondering why the build is old (§5).
- Building (or cooking) while an editor for the same project is open — LNK1104 now, corrupted
  hot-reload state later (§6).
- Calling a feature done on PIE evidence alone. PIE ≠ packaged: four defines, ensure silence, GC
  verifiers, cook stripping (§7).
- Citing a green run whose binaries predate your last edit — stale-green (§5).

## Provenance and maintenance

Authored 2026-07-03 against BusterBlock at detached HEAD `52a75e13d` (dev tip) as the corpus, with
CkFoundation framework skills dated 2026-07-02. Verified directly by the author (not taken from
secondary reports): `Config/DefaultGame.ini:113-146` (MapsToCook, AlwaysCook incl. ESP dirs,
NeverCook GYMs/AutoTests + comment, AlwaysStageAsNonUFS + comment), `Source/BusterBlock.Target.cs`
(CkTests DisablePlugins block), `Source/BusterBlockEditor.Target.cs:15` (Unique build env),
`Source/BusterBlock/BusterBlock.Build.cs:98-102` (game module no longer depends on CkTests),
`.gitignore:384-401` (ESP + stub-recovery ignores + wedge comment),
`.runreal/scripts/cook-with-retry.ps1` (full read), `.runreal/buildgraph/Build.xml` (full read,
BuildCookRun shapes), and commits `e0de34899`, `b8da4ad3b`, `a6c9a7980`, `b9b8a2214`, `1fba680be`
via `git show --stat`.

Re-verify volatile claims:

```powershell
# cwd = consuming game repo (BusterBlock shown)
Select-String -Path Config\DefaultGame.ini -Pattern "DirectoriesToAlwaysStageAsNonUFS|DirectoriesToNeverCook"
Select-String -Path Source\*.Target.cs -Pattern "DisablePlugins"
git show --stat e0de34899 b8da4ad3b a6c9a7980 b9b8a2214 1fba680be
Get-Content .runreal\scripts\cook-with-retry.ps1 -TotalCount 25   # BB-only glue; rationale comment
```

Note for agents: the repo-root `.ignore` hides `Script/`, plugin `Script/`, `docs/`, and content
dirs from ripgrep-based tools — zero matches there are not absence; re-check with
`rg --no-ignore` or `Get-ChildItem`.
