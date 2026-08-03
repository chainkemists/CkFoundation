# Failure classes §1-§7

Reference for `ck-debugging-playbook`: UBT failures, UHT/GENERATED_BODY errors, cross-module linker errors, AngelScript compile/binding failures, packaged-only GC crashes, PIE-vs-packaged divergence, environment and multi-session traps. Enter from the triage table in SKILL.md.

## 1. UBT failures

### 1.1 Missing / wrong-configuration modules

Module DLLs are per-target and per-configuration, encoded in the filename. Verified on-disk layout in
the BusterBlock superproject (plugin modules build INTO the project tree — `Plugins/CkFoundation/` has
no `Binaries/` or `Intermediate/` of its own):

```
Binaries/Win64/BusterBlockEditor-CkCore.dll                    ← Development editor (no suffix)
Binaries/Win64/BusterBlockEditor-CkCore-Win64-DebugGame.dll    ← DebugGame editor
```

So a DebugGame-built workspace launched as Development (or vice versa) boots into symptom row 1 or 2 —
the DLLs exist, just not under the names that configuration loads. This exact mismatch produced bogus
"plugin module not found" reports when a tool's build configuration drifted from its launch
configuration (operator experience, 2026-06). Discriminator first, always:

```powershell
# PowerShell, cwd = superproject root
Get-ChildItem Binaries/Win64 -Filter '*CkCore*' | Select-Object Name, LastWriteTime
```

If the DLL for your configuration exists but is OLDER than your last source edit, the build you trusted
did not include your change (stale-green trap) — rebuild before debugging anything else.

### 1.2 DLL locks — find the holder

`LNK1104: cannot open file '...-<Module>.dll'` means the linker cannot overwrite a loaded/locked DLL.
Holders, ranked: a running editor (including one launched early while the build was still in flight —
operator experience, 2026-06), a crashed-but-zombie editor process, a second concurrent build.

```powershell
# PowerShell — candidate processes:
Get-Process UnrealEditor*, msbuild, dotnet, UnrealBuildTool -ErrorAction SilentlyContinue

# Definitive per-project probe: UE holds an exclusive write lock on its active log while running.
# Process-name scans lie (custom forks rename the editor binary); this doesn't.
# (Technique mirrors the BusterBlock guard script CkAuto/Check-UnrealNotRunning.ps1:83-101.)
$locked = $false
Get-ChildItem Saved/Logs -Filter *.log -File | ForEach-Object {
    try { ([IO.File]::Open($_.FullName, 'Open', 'Write', 'None')).Close() }
    catch { $locked = $true }
}
if ($locked) { 'editor RUNNING for this project' } else { 'no editor holds the log' }
```

Close the editor (or kill the zombie), then rebuild. If you killed a UBT/linker mid-write, expect
deleted DLLs → row 1 on next boot; just rebuild.

### 1.3 Stale Intermediate — what to delete, when

Verified layout (BusterBlock superproject, 2026-07-02) — all build state is PROJECT-level:

| State | Path |
|---|---|
| UHT-generated headers | `Intermediate/Build/Win64/BusterBlockEditor/Inc/<Module>/UHT/*.generated.h` |
| Object files / unity blobs | `Intermediate/Build/Win64/x64/BusterBlockEditor/<Config>/<Module>/` |
| Module DLLs | `Binaries/Win64/<Target>-<Module>[-Win64-<Config>].dll` |

Escalation ladder (steps b-d are standard-UE practice; the paths above are the verified specifics):

- **a. Don't delete for ordinary staleness.** UBT re-evaluates `.Build.cs`/`.uplugin` changes itself; a
  plain rebuild handles source-newer-than-binary. Regenerating project files only fixes IDE/solution
  staleness, not compiled artifacts.
- **b. Ghost errors naming deleted code** (row 4): delete that module's two dirs (Inc + obj) and rebuild.
- **c. Mass ghost errors after a branch switch / rebase:** delete `Intermediate/Build` (forces full UHT +
  compile — slow) and rebuild.
- **d. Never delete `Saved/` or `DerivedDataCache/` for C++ build problems** — they hold logs/config and
  cooked derived data, not build state; deleting them destroys evidence and costs hours of DDC rebuild.
- Plugin-standalone note: a plugin built outside a project (marketplace-style) gets its own
  `<Plugin>/Binaries` + `<Plugin>/Intermediate` — not this repo family's layout.

## 2. UHT / GENERATED_BODY errors

Order inside every reflected Ck struct/class — each line depends on the ones above it:

1. `USTRUCT(...)` / `UCLASS(...)`
2. `GENERATED_BODY()` — first thing in the body, or UHT stops with "Expected a GENERATED_BODY() at the
   start of the class" (engine `EpicGames.UHT/Parsers/UhtClassParser.cs`).
3. `CK_GENERATED_BODY(FMyType);` — plants `using ThisType = FMyType;`
   (`Source/CkCore/Public/CkCore/Macros/CkMacros.h:67-69`). Consumers below it:
   `CK_PROPERTY_UPDATE` (and therefore `CK_PROPERTY`, which includes it) returns `ThisType&` in every
   build (CkMacros.h:111-116); additionally, in AngelScript-enabled builds `CK_PROPERTY` /
   `CK_PROPERTY_GET` / `CK_PROPERTY_SET` each emit a registration function containing
   `using ClassType = ThisType;` (`CkMacros_AngelScript.h:297,368,424`). Consequence worth knowing: a
   struct using only `CK_PROPERTY_GET`/`CK_PROPERTY_SET` without `CK_GENERATED_BODY` compiles on a
   machine WITHOUT the AS engine plugin (`WITH_ANGELSCRIPT_CK=0` strips the registrations) and fails
   with `'ThisType': undeclared identifier` on a machine with it.
4. Member declarations — before any `CK_PROPERTY*` / `CK_DEFINE_CONSTRUCTORS`, which take the type via
   `decltype(_Member)`.
5. Accessor + constructor macros last.
6. `#include "<File>.generated.h"` must be the header's LAST include, or UHT errors "#include found
   after .generated.h file" (engine `EpicGames.UHT/Parsers/UhtHeaderFileParser.cs`).

Also: UFUNCTION declarations cannot use trailing-return syntax (UHT rejects them — root CLAUDE.md
"Code style"). Full macro expansions, constraints, and add-a-new-X checklists: load
`ck-macros-and-codegen`. Canonical ordered exemplar: the fragment-data shape in root CLAUDE.md
("Encapsulation") and `Source/CkTimer/Public/CkTimer/CkTimer_Fragment_Data.h`.

## 3. Cross-module linker errors

**LNK2019/LNK2001 unresolved external** for a symbol defined in another Ck module — two causes, check
both:

1. **Missing export.** The defining class/function needs its module's API macro
   (`CKCORE_API`, `CKTIMER_API`, ...). A header-visible symbol without it links fine inside its own
   module and fails from every other one.
2. **Missing Build.cs dependency.** Add the module to your `PublicDependencyModuleNames` — but check the
   **module tier table** in `Source/CLAUDE.md` first: deps must never point to a higher tier, and runtime
   modules must never depend on T5 (editor) modules. If the dependency you "need" violates the tier, the
   design is wrong, not the Build.cs.

**LNK2005 / C2084 duplicate symbol — unity-build collisions.** Every Ck module is a unity build
(§ Overview). Two .cpp files each defining a file-local `static Helper()` or an anonymous-namespace
helper get concatenated into one TU and collide. This is why the root doctrine bans anonymous namespaces
and file-local statics outright — use a filename-derived named namespace
(`namespace ck_timer_processor`). Registration macros already defend themselves: `CK_REGISTER_SNAPSHOTABLE`
token-pastes the fragment TYPE name instead of `__LINE__` precisely because unity TUs collide on line
numbers (comment at `Source/CkEcs/Public/CkEcs/Snapshot/CkSnapshot_FragmentRegistry.h:132-136`).

**Unity group shift — the sneakier variant.** Editing one .cpp re-chunks the unity blobs; a DIFFERENT
file that was silently borrowing includes from its old blob-neighbors suddenly fails with missing types.
Real precedent, in the commit record: `56b344310` had to add an explicit `CkTag_EditorOnly.h` include to
`CkEntityLifetime_Utils.cpp` because "the sibling Processor.cpp edit shifts the unity-build group and
re-exposes a latent transitive-include dependency" (commit body, verbatim). Fix = add the explicit
include to the failing file; the edit that "caused" it is innocent.

**Adaptive unity — "it compiled until I touched it."** UBT excludes files that are dirty in git from
unity blobs and compiles them standalone (`bUseAdaptiveUnityBuild`, default true, working set via
`git status` — engine `UnrealBuildTool/Configuration/TargetRules.cs:1734-1742`). So a file with missing
includes builds fine for everyone else and breaks only on the machine where it's modified. Same fix:
add the includes it actually needs.

## 4. AngelScript compile / binding failures

Full 13-item silent-breakage catalog and the codegen pipeline: load `ck-angelscript-interop`. The three
triage-critical ones:

**4.1 First-boot transient vs real error.** After changes touching dynamic handles or generated scripts,
the FIRST editor boot may print a wall of `"'FCk_Handle_X' is not a data type"` plus
`"Hot reload failed ... Keeping all old script code"` — then the self-heal generator regenerates
(`DynamicHandleTypes.json`, recovery stubs) and reloads clean IN THE SAME BOOT. That transient is
EXPECTED; gate on the post-regen clean reload, not first-pass silence
(`Script/CLAUDE.md` §22.3, `Source/CkAngelscriptGenerator/Claude.md`). **Discriminator: boot a second
time. Second boot still red = a real error in your script; fix it, don't blame the generator.**
Opt-outs when you need the dispatcher out of the way: `-NoCkAsRegen` (per launch) or
`UCk_AngelscriptGenerator_ProjectSettings_UE::_EnableAsBootstrapSelfHeal` (generator Claude.md:149-150).
The clean gate, precisely: clean reload after the deferred regen AND the handle's entry present in
`DynamicHandleTypes.json` — boot shape + the exact grep: `ck-angelscript-interop` §4 step 2
(boot command also in `ck-build-and-env` §4).

**4.2 ScriptMixin arg0 rule — member-only vs static-only.** When a Utils class carries
`Meta = (ScriptMixin = "FCk_Handle_Timer")` (exemplar `Source/CkTimer/Public/CkTimer/CkTimer_Utils.h:34`)
and a static UFUNCTION's FIRST parameter type equals that mixin target, the engine binds it as a MEMBER
method of the handle type — and the static-class call form stops resolving (engine
`AngelscriptCode/Private/Binds/Helper_FunctionSignature.h:291-292`). Arg0 of any other type stays a plain
static. Symptom either way is a bare "No matching signatures".

```angelscript
UCk_Utils_Timer_UE::BindTo_OnUpdate(Timer, Delegate);  // ❌ does not resolve (arg0 == mixin type)
Timer.BindTo_OnUpdate(Delegate);                       // ✅ member form
utils_timer::BindTo_OnUpdate(Timer, Delegate);         // ✅ generated wrapper — works for BOTH kinds; prefer it
```

Corollary: new C++ must BUILD before the AS that calls it can compile — a red AS callsite after adding a
C++ util usually means the editor is running old binaries.

**4.3 Runtime-only f-string throw.** A raw `FCk_Handle` in an f-string compiles clean and throws
`"Invalid type to append to string."` at execution time (engine
`AngelscriptCode/Private/Binds/Bind_FString.cpp:597`) — invisible to `-skipcompile` compile-check boots
and to any run that never executes the line. Use `{Handle.ToString()}`.

## 5. Packaged-only crashes — the GC class

### 5.1 The disregard-pool incident, as git records it

Symptom: 0xC0000005 in a packaged Development client; PIE always clean. The commit chain
(all verified via `git -C Plugins/CkFoundation show`):

| SHA | Role |
|---|---|
| `d77810096` (2026-06-02) | Diagnostics: adds `Ck.Diag.DumpAngelscriptAssets` + `Ck.Diag.VerifyGCAssumptions` (`Source/CkCore/Public/CkCore/Object/CkObject_Utils.cpp:603,621`) |
| `feb08ee94` (2026-06-02) | Root cause + fix: pre-GC rooting pass (`CkDeferredAssetInit_AngelScript.cpp`) |
| `a8a93baac` (2026-06-24) | Hardening: `CK_ENSURE_IF_NOT(IsInGameThread())` tripwire at the rooting entry — DETECTION of a future regression (off-thread move), not part of the fix |

Root cause (commit body + in-code design note `CkDeferredAssetInit_AngelScript.cpp:448-469`): AngelScript
`asset ... of ...` owners and AS CDOs are created during AS InitialCompile, BEFORE the engine closes the
disregard-for-GC set — so they live in the permanent pool GC never traverses. The post-boundary asset
refresh then attaches normal-pool objects (minted sub-objects, `assets::load`'d meshes) under those
untraversed owners; the first real GC reclaims the children out from under them → dangling pointer →
crash. Fix: before each GC (non-editor only), walk the AS disregard objects with the GC reference
collector and `AddToRoot` every referenced target the engine verifier would flag.

### 5.2 Why PIE structurally cannot reproduce it

Not "hard to hit in PIE" — impossible by construction:

- The failing edge requires objects on BOTH sides of the `CloseDisregardForGC()` boot boundary (engine
  LaunchEngineLoop.cpp:3863): an owner born before it (permanent, untraversed) referencing a child born
  after it (collectable). Which pool an object lands in is decided by WHEN it was created relative to
  that one-shot boundary — the identical object created later has different GC semantics.
- In-editor, the bug doesn't manifest at all: "asset registry/Content Browser keep things alive,
  verifiers are off there" — the in-code rationale for gating the entire fix `#if !WITH_EDITOR`
  (`CkDeferredAssetInit_AngelScript.cpp:467-471`).

So: any GC-lifetime claim about AS-born objects is only testable in a cooked client.

### 5.3 Discriminating experiment — "is my packaged crash this class?"

On a cooked (packaged Development) client, open the console and run — all three names verified in code:

| Command | What it does |
|---|---|
| `Ck.Diag.VerifyGCAssumptions` | Enables engine `gc.VerifyAssumptions` + `gc.VerifyAssumptionsOnFullPurge`, forces a full-purge GC. On an affected build it logs every "Disregard for GC object X referencing Y..." violation as a Warning, THEN Fatals (engine `GarbageCollectionVerification.cpp`) — the complete violation list is in the log before the crash. Clean build: no Fatal. |
| `Ck.Diag.DumpAngelscriptAssets` | Structured dump of AS asset/CDO GC state, forced full GC, dump again — diff the before/after. |
| `gc.CollectGarbageEveryFrame 1` | Engine soak cvar (engine `UnrealEngine.cpp:1671`) — makes reclaim-after-GC bugs fire in seconds instead of minutes. The fix commit's own verification gate. |

Fatal from the verifier → this class; go read `feb08ee94` and the design note before touching anything.
No violations but still crashing → different bug; check §5.4 and §6.

`[EDITOR-VERIFY]` (agents cannot run packaged clients): package a Development client, launch it, open the
console (~), run `Ck.Diag.VerifyGCAssumptions`, observe log + presence/absence of the Fatal.

### 5.4 Companion lesson — "incidentally alive" (`56b344310`, 2026-05-28)

Symptom: standalone / `-nullrhi` runs destroyed the PlayerController ~1.2s after possession; networked
sessions fine. Cause (commit body): the per-entity replication-driver UObject had NO strong GC root —
every reference was weak (`TWeakObjectPtr`, `FSubObjectRegistry`) or GC-invisible: **UE GC does not trace
UPROPERTY refs inside EnTT fragments** (a fragment is plain C++ data, not reflected object state — the
rule is now doctrine, root CLAUDE.md "UObject refs in fragments"). A live netdriver's
UNetConnection→FObjectReplicator chain had been *incidentally* keeping it alive; remove the netdriver and
the first GC sweep reclaimed it, whose `BeginDestroy` cascaded into destroying the owning actor. Fix:
the fragment owns its objects via `TArray<TStrongObjectPtr<...>>` (current at
`Source/CkEcs/Public/CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Fragment_Params.h:137`),
weak-converted only at the wire boundary.

The transferable check, for ANY UObject an entity touches: **enumerate who actually roots it** — not
"something references it" but which reference is strong AND GC-visible. Then re-test with the masking
subsystem absent (here: no netdriver — standalone/`-nullrhi` is the repro harness, not an edge case).
GC/lifetime theory and the entity↔actor contract: load `ckecs-domain-reference`.

## 6. PIE-vs-packaged divergence checklist (beyond GC)

Walk every axis; each has a verified precedent.

1. **Error paths that only execute cooked.** `72199da89`: the non-editor branch of tag resolution logged
   `'[{}]'` with no argument → uncaught `fmt::format_error` → `std::terminate` in packaged builds only;
   the editor path never ran that line. Error paths are the least-executed code — read the non-editor
   branch of anything that diverges on `WITH_EDITOR`.
2. **Cook-time stripping / editor-only classes.** `700b5ef95`: project autotest wrappers inherit a class
   that doesn't exist in Shipping/Test, so the generator `#if EDITOR`-wraps the PROJECT wrapper bucket
   (and must NOT wrap the plugin bucket). Runtime modules must never depend on T5 editor modules
   (`Source/CLAUDE.md` tier table, `Source/EDITOR_MODULES.md`).
3. **Discovery/scan differences.** `362e8917a`: cue class discovery missed in packaged builds and needed
   an explicit fallback — asset/class iteration that works against the editor's registry can return less
   when cooked.
4. **Define gates flip per configuration.** The full 17-define × 5-config matrix is owned by
   `ck-macros-and-codegen` §2.4 (source of truth: `CkBuildConfig.Build.cs`, `SetBuildConfiguration`
   :65-198). The rows that most often explain a packaged-only divergence: ensure checks stay ON
   everywhere (`CK_DISABLE_ENSURE_CHECKS=0` incl. Shipping) but go silent in Test/Shipping
   (`_DEBUGGING=1`); `CK_DISABLE_ECS_HANDLE_DEBUGGING=1` outside Debug/Dev-editor; and
   `CK_BUILD_DEBUG_DRAW=0` only in Shipping.
   Same-pattern plugin gate: `WITH_CK_DEBUG_OVERLAY` = 0 only in Shipping
   (`Plugins/CkGameplayDebugger/Source/CkEntityDebugOverlay/CkEntityDebugOverlay.Build.cs:36-39`).
   `WITH_ANGELSCRIPT_CK` flips on engine-plugin presence, not configuration (CkBuildConfig.Build.cs:53-61).
5. **Ensure behavior differs by config AND run mode** (`CK_ENSURE_IF_NOT` expansion,
   `Source/CkCore/Public/CkCore/Ensure/CkEnsure.h:44-53`):
   - Test/Shipping (`CK_DISABLE_ENSURE_DEBUGGING=1`): the predicate IS evaluated and the recovery block
     DOES run — but with zero logging/dialog. An ensure "not firing" in Test is it firing silently.
   - `-unattended` / commandlet runs: full Error log, but never a dialog (`CkEnsure.cpp:221-222`,
     `0eb3208aa`) — grep the log, don't wait for UI.
   - The recovery block must therefore be a correct silent-failure path in every config (root
     non-negotiable #3); it becomes dead code (`if constexpr(false)`) only under the opt-in Profile
     override (`CK_DISABLE_ENSURE_CHECKS=1`, CkBuildConfig.Build.cs:190-195).
   - `CK_DISABLE_ECS_HANDLE_DEBUGGING=1` also strips request debug names and the request vtable —
     request structs change `sizeof` across configs (CkRequest_Data.h; depth: `ck-macros-and-codegen`).
6. **Config file differences.** Cooked targets read cooked/staged ini layers; a `[/Script/...]` setting
   present only in editor config silently vanishes. When a packaged run ignores "your" setting, diff the
   staged ini in the .pak/Staged directory against `Config/Default*.ini` (standard-UE axis; no Ck
   incident on record).

## 7. Environment / multi-session traps

Verifiable mechanics only. Items marked **[operator experience, 2026-06]** come from operator session
records in this environment, not from git/docs — the mechanism cited alongside each IS code-verified.
Environment setup from scratch: load `ck-build-and-env`.

1. **Overlapping or killed UBT runs delete module DLLs mid-build** [operator experience, 2026-06].
   Rule: before ANY build, sweep for a build already in flight —
   `Get-Process msbuild, dotnet, UnrealBuildTool -ErrorAction SilentlyContinue` (PowerShell). A killed
   run leaves rows 1-2 of the triage table on next boot; recovery is a plain rebuild.
2. **Two sessions sharing one engine ⇒ headless editor boots use `-skipcompile`** [operator experience,
   2026-06]. Verified mechanism: the flag is parsed at engine `LaunchEngineLoop.cpp:6591` and suppresses
   the boot-time "modules out of date → compile" path — exactly the path that would spawn a UBT run
   colliding with the sibling session's build.
3. **Editor running while you build or run file-mutating git ops** → DLL locks (§1.2) and corrupted
   hot-reload state. Detect per-project with the log-lock probe (§1.2) — not by process name.
4. **Two editor/headless instances of one project** → the AS generator's cross-process single-writer
   lock (`<Saved>/CkAngelscriptGenerator_RegenOwner.lock`) makes the second instance a read-only
   secondary: it compiles against the owner's generated files and writes nothing
   (`Source/CkAngelscriptGenerator/Claude.md:32,52`; the OS releases the lock on any process exit, so
   stale locks are impossible). If generated files seem frozen in one instance, check which process owns
   the lock. The dual-editor ping-pong incident that motivated it: `ck-failure-archaeology`.
5. **Never blanket-delete `Script/Generated/`** — the AS hot-reload watcher is mtime-based; any mtime
   change there triggers a full reload sweep (`Script/CLAUDE.md` §22.4).

