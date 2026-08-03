# The pipeline, AS staging, test exclusion, two-pass cooks

Reference for `ck-game-build-and-cook`: the generic build/cook/stage pipeline (§1), the AngelScript staging step everyone misses (§2), keeping the test skeleton out of shipped builds (§3), and why a fresh-workspace cook is two-pass (§4).

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

