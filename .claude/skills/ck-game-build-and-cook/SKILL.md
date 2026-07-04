---
name: ck-game-build-and-cook
description: Use when building, cooking, staging, or packaging a CkFoundation game on Windows —
  RunUAT BuildCookRun shapes, Development/Test/Shipping client builds; when a packaged build
  won't boot behind a wall of AngelScript errors ("unknown super type", hundreds of AS compile
  errors at boot), when the cook commandlet exits 3 with "Cannot run when angelscript has failed
  to compile" on a fresh clone, when a packaged build serves stale assets or old behavior, when
  test/gym content leaks into a Shipping build, or when a build dies with LNK1104. For BUILDING
  games ON CkFoundation. Not for engine/env setup or build-environment breakage (ck-build-and-env),
  packaged crash diagnosis (ck-debugging-playbook / ck-game-debugging-playbook), or day-1 project
  wiring (ck-game-project-bootstrap); for modifying the framework itself, see ck-change-control.
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

## 1. The generic pipeline

Five stages, in dependency order. Each is one `BuildCookRun` invocation (or one flag set on a
combined invocation). All commands are **PowerShell**, run from anywhere; `$Engine` is the engine
root — resolve it with `$Engine = & <ProjectRoot>/CkAuto/Get-ProjectEnginePath.ps1` (registry GUID
lookup per `ck-build-and-env` §1; never hardcode), and
`$Project = "D:\Repos\<Game>\<Game>.uproject"`.

### 1.1 Editor build first — the cook runs *inside* the editor binary

The cook commandlet is the editor. No editor binaries → no cook. Build shape and headless-boot
shapes are owned by `ck-build-and-env` §4; the one pipeline-relevant reminder:

```powershell
& "$Engine\Engine\Build\BatchFiles\Build.bat" <Game>Editor Win64 Development `
    -Project="$Project" -WaitMutex -FromMsBuild
```

**Never run this while the editor is open for the same project.** A running editor holds the
module DLLs mapped; the linker fails with LNK1104, and a partially-relinked module set corrupts
hot-reload state (`ck-build-and-env` §6 T2/T3; triage in `ck-debugging-playbook` §1.2). With
`TargetBuildEnvironment.Unique` (standard for Ck games — verified `BusterBlockEditor.Target.cs:15`),
the editor binary lands project-named under `<Project>/Binaries/Win64/<Game>Editor.exe` /
`<Game>Editor-Cmd.exe`, not in `Engine/Binaries/`.

### 1.2 Game/client target build — configuration matters more than in stock UE

```powershell
& "$Engine\Engine\Build\BatchFiles\Build.bat" <Game> Win64 Development -Project="$Project"
# Configurations: Development | Test | Shipping (Debug rarely packaged)
```

Per-configuration `CK_*` defines change framework behavior — **PIE (Development-Editor) and a
packaged Development game differ by exactly four defines**, and Test/Shipping silence ensure
output while still evaluating predicates and running recovery blocks. The full 17-define ×
5-config matrix is owned by `ck-macros-and-codegen` §2.4; the packaged-divergence consequences by
`ck-debugging-playbook` §6.4–6.5. Practical rule: a config you never built is a config you never
tested (§6 below).

Your game target should carry the CkTests exclusion (§3) — verify it before your first
Shipping/Test build, not after it fails.

### 1.3 Cook → stage/pak → package → archive

One-shot form (local developer packaging — fine for most day-to-day use):

```powershell
& "$Engine\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun `
    -project="$Project" -platform=Win64 -configuration=Development -target=<Game> `
    -build -cook -stage -pak -package -archive -archivedirectory="D:\Builds\<Game>"
```

Split form — each stage its own invocation, so a failure is attributable and retryable per-stage.
This is the shape a CI graph wants, and the shape the corpus uses (Corpus example (BusterBlock):
`.runreal/buildgraph/Build.xml:160,198,203,207`, verified 2026-07-03):

```powershell
# Cook (uses the Development editor binaries regardless of the target config you'll stage)
... BuildCookRun -project="$Project" -platform=Win64 -configuration=Development `
    -cook -skipbuild -skipstage -skippak -skippackage

# Stage + pak (this is where -configuration selects which target binaries get staged)
... BuildCookRun -project="$Project" -target=<Game> -platform=Win64 -configuration=<Config> `
    -skipbuild -skipcook -stage -pak

# Package, then archive
... BuildCookRun ... -skipbuild -skipcook -skipstage -skippak -package
... BuildCookRun ... -skipbuild -skipcook -skipstage -skippak -skippackage `
    -archive -archivedirectory="D:\Builds\<Game>\<BuildId>"
```

Dedicated server: same shapes with `-server -noclient` (corpus: `Build.xml:232,243`).

Cook-time plugin exclusions: if an editor-only or third-party plugin emits cook-time errors that
have nothing to do with cooked content, disable it *in the cook commandlet* rather than reaching
for `-IgnoreCookErrors`:
`-AdditionalCookerOptions=-DisablePlugins=<PluginA>,<PluginB>`
(Corpus example (BusterBlock): `Build.xml:35-39` disables `AdvancedCommenting,ProInstanceToolsPlugin`
for exactly this reason — the pattern is generic, those two plugin names are BB residue.)

What to cook: standard UE — list shipping maps in `+MapsToCook` and content roots in
`+DirectoriesToAlwaysCook` (`Config/DefaultGame.ini`, `[/Script/UnrealEd.ProjectPackagingSettings]`).
Ck-specific: also always-cook the generated **EntitySpawnParams content** dirs — Corpus example
(BusterBlock): `DefaultGame.ini:118,122` cooks `/CkFoundation/EntitySpawnParams` and
`/Game/EntitySpawnParams`. (Entries confirmed 2026-07-03; the rationale — spawn-params assets are
referenced dynamically, invisible to the cooker's dependency walker — is inferred, not confirmed.)

---

## 2. AngelScript staging — the framework step everyone misses

**The mechanism (confirmed):** at boot the AS runtime compiles the project's `Script/` root plus
every *enabled* plugin's `Script/` root (`FAngelscriptManager::MakeAllScriptRoots`). The engine
fork's `Engine/Config/BaseGame.ini` stages the project's `../Script` as loose NonUFS automatically
— **but not plugin `Script/` roots.** A plugin script root is only added if it exists on disk in
the staged layout, and default staging never copies it. (Authoritative comment block:
BusterBlock `Config/DefaultGame.ini:132-142`, verified 2026-07-03.)

**The rule:** every AS-bearing plugin your packaged build enables needs a staging line in
`Config/DefaultGame.ini` under `[/Script/UnrealEd.ProjectPackagingSettings]`. Paths are relative
to the project `Content/` dir; they land at `<Staged>/<Game>/Plugins/<Plugin>/Script`, which is
exactly `Plugin->GetBaseDir()/Script`:

```ini
+DirectoriesToAlwaysStageAsNonUFS=(Path="../Plugins/CkFoundation/Script")
; Add a line per additional plugin that ships a Script/ folder your packaged build enables.
```

Corpus example (BusterBlock): `DefaultGame.ini:143-144` stages `CkFoundation/Script` and
`CkTests/Script` (CkTests stays enabled in packaged *Development* builds for gym/autotest support;
it is disabled entirely in Shipping/Test — §3 — where its staged dir is inert). Editor-only test
plugins (`<Game>Tests`, §3) never need a line: they're absent from packaged targets altogether.

**What happens when you forget** (both incidents verified via `git show`):
- Any project script inheriting a plugin-defined AS base fails to compile in the packaged build —
  the base type simply isn't there. Fixed by introducing the staging lines: commit `b8da4ad3b`
  "fix(packaging): stage plugin Script/ folders so plugin-defined AS bases resolve".
- At scale it's fatal, not degraded: **431 "unknown super type" errors aborted AS preprocessing
  entirely and the Shipping client never booted** (commit `e0de34899`, 2026-06-25). The symptom is
  an error wall in the packaged log, then no game. Do not triage the individual AS errors — check
  staging first.

**`Script/Generated/` in packaged builds:** the project `Script/` staging brings `Script/Generated/`
along with it (it's inside the root). The gitignored ESP files (see Overview) are regenerated by
any editor boot, so a workspace that has ever booted the editor stages complete Generated content.
A *fresh-clone CI workspace* has not — which is why the cook itself needs two passes (§4).

---

## 3. Test-content exclusion — keep the test skeleton out of shipped builds

Three coordinated exclusions, all mandatory. The WHY is one incident: commit `e0de34899` — test
and gym AS was compiled at boot by a packaged Shipping client whose CkTests C++ bases were
(correctly) disabled, producing the 431-error no-boot wall above. The content half of the same
commit: placed actors in gym/autotest maps reference test AS classes and CkTests C++, both absent
in the cook commandlet's target runtime — cooking those maps dangles the references.

1. **Never-cook test maps/content** (`Config/DefaultGame.ini`):

   ```ini
   +DirectoriesToNeverCook=(Path="/Game/<Game>/Map/GYMs")
   +DirectoriesToNeverCook=(Path="/Game/<Game>/Map/AutoTests")
   ```

   Corpus example (BusterBlock): `DefaultGame.ini:130-131` with the rationale comment at :125-129.
   Editor autotests/gyms load the uassets directly and are unaffected — only packaging is denied.

2. **Disable CkTests in Shipping/Test targets** (`Source/<Game>.Target.cs`). CkTests depends on
   non-redistributable Developer modules (FunctionalTesting, Gauntlet → AutomationController, …);
   non-editor Shipping/Test builds *reject* non-redistributable dependencies. Development packages
   keep it, and the editor target is unaffected so the cook commandlet still has CkTests available.
   (Verified `BusterBlock.Target.cs:17-27`; introduced by commit `1fba680be`.)

   ```csharp
   if (Target.Configuration == UnrealTargetConfiguration.Shipping ||
       Target.Configuration == UnrealTargetConfiguration.Test)
   {
       DisablePlugins.Add("CkTests");
   }
   ```

3. **House the game's own test AS/C++ in an editor-only `<Game>Tests` plugin** —
   `EnabledByDefault: false`, enabled in the `.uproject` with `"TargetAllowList": ["Editor"]`,
   declaring a plugin dependency on CkTests. Everything CkTests-dependent then compiles for the
   editor (including editor-binary `-game` Gauntlet runs) and is structurally absent from every
   packaged build — no per-config staging or cook rules needed for it. Corpus example
   (BusterBlock): `Plugins/BusterBlockTests/`, migration commits `68616a8a5` / `0637ed1ac`.
   The intermediate `Script/Dev/` convention you may see referenced in older comments was the
   first-generation fix from `e0de34899`; the editor-only plugin superseded it — use the plugin
   form for new projects.

---

## 4. Fresh-workspace cooks are two-pass — plan for it, don't fight it

**Mechanism (framework behavior, generic):** ESP files are gitignored (§2). On a fresh checkout
they're absent, so the cook's AS compile fails and the commandlet exits **3** ("Cannot run when
angelscript has failed to compile"). Before exiting, the CkAngelscriptGenerator's headless
self-heal dispatcher synthesizes recovery stubs (`Script/Generated/_StubRecovery_*.as`)
synchronously. The **next** cook compiles green against those stubs and regenerates the canonical
files. The heal is inherently two-pass and the stubs must survive between passes. (Internals:
`ck-angelscript-interop`; first-boot-vs-real-error discriminator: `ck-debugging-playbook` §4.1.)

**The operational rule:** retry the cook **in the same workspace, without re-cleaning**. A CI
retry that re-runs the whole job (e.g. a checkout-clean per attempt) deletes the gitignored stubs
first — every retry is another pass 1, and the cook never converges. AS surfaces errors in waves,
so budget ~3 passes; stop at first success so a clean cook runs exactly once.

Corpus example (BusterBlock) — this loop is BB pipeline glue, but the rationale it encodes is the
generic rule above: `.runreal/scripts/cook-with-retry.ps1` (MaxPasses=3, same-workspace loop,
comment block explains why Buildkite `retry: automatic` cannot supply pass 2), wired by commit
`a6c9a7980`. A minimal generic equivalent:

```powershell
# PowerShell — same-workspace cook retry (generic shape)
for ($Pass = 1; $Pass -le 3; $Pass++)
{
    & "$Engine\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun -project="$Project" `
        -platform=Win64 -configuration=Development -cook -skipbuild -skipstage -skippak -skippackage
    if ($LASTEXITCODE -eq 0) { exit 0 }
    # Self-heal stubs persist in Script/Generated/ for the next pass — do NOT clean here.
}
exit 1
```

**The committed-ESP wedge:** never commit a plugin's `*_EntitySpawnParams.as`. A committed plugin
ESP leaves its subclass accessor present, which *blocks* the self-heal of the base project accessor
it subclasses — the cook then fails on every pass. Gitignore them for all non-submodule plugins
(commit `b9b8a2214`, the 2026-06-29 StoreDriver cook wedge; BusterBlock `.gitignore:395-401`).

---

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
