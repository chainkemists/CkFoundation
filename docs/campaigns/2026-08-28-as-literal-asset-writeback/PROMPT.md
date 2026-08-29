# Executor Handoff — AngelScript Literal-Asset Write-Back

**For:** a fresh Opus implementation session. You are implementing; the design is done and
CTO-reviewed. Read this file, then [PLAN.md](PLAN.md) in full before writing code.

**Repo:** `D:\Repositories\CkRepos\CkPlugins_Other` (branch `dev`)
**Engine:** `D:\Repositories\UnrealEngine-Angelscript` (read-only reference — do NOT edit engine code)
**Target module:** `Plugins/CkFoundation/Source/CkAngelscriptGenerator/`
**Design record:** [PLAN.md](PLAN.md) · **Review:** [CTO review](../../reviews/2026-08-28-as-literal-asset-writeback-CTO-review.md)

---

## 1. What you are building, in one paragraph

AngelScript "literal assets" are data assets declared in `.as` source as
`asset <Name> of <UClass> { … }`. They have no `.uasset`, so edits made in the editor's details
panel **cannot be saved** — Ctrl+S produces `"Cannot save asset declared as an angelscript asset
literal"`. Build a **"Write Back to Script"** toolbar button that surgically patches the changed
property values back into the `asset … of … { }` block in the owning `.as` file, atomically, with a
confirmation dialog. Object references are in scope and must emit the project's generated
`assets::` accessor expressions, not raw paths.

---

## 2. Read these before writing code

**Mandatory, in this order:**

1. [PLAN.md](PLAN.md) — the design. §3 is a 22-row evidence table; §4 is the three traps; §5 is the
   design proper. Do not deviate from §9 (settled decisions) without raising it.
2. `Plugins/CkFoundation/CLAUDE.md` — style doctrine of record. Function formatting
   (`auto Name() -> Ret`), `_Member` naming, `Get_`/`Request_`/`Do*` prefixes, the `NOT` macro,
   brace-on-next-line for `if` bodies, `auto` aggressively, `{}` construction, **no anonymous
   namespaces** (unity builds — use a filename-derived named namespace).
3. `Plugins/CkFoundation/Source/CLAUDE.md` — module topology and the tier rule the module placement
   leans on.
4. `Plugins/CkFoundation/Script/ARCHITECTURE.md` §13 — asset definitions. This is the contract your
   emitted text must satisfy.

**The closest existing analogue — mirror its shape:**
`Source/CkAngelscriptGenerator/SelfHeal/` — `CkAngelscriptGenerator_StubSynthesizer.{h,cpp}` and
`CkAngelscriptGenerator_AsSourceScanner.{h,cpp}` (textual `.as` scanning, surgical edit, atomic
write) plus `Tests/Test_StubSynthesizer.cpp` (temp-file fixture style). Your files should read like
siblings of these.

---

## 3. The three traps — internalise before designing any function

These are the reasons the plan looks the way it does. Violating one silently destroys user work.

**Trap A — silent reference destruction.** `UCk_Utils_Reflection_UE::Get_PropertyDefaultValueLiteral`
returns literal `nullptr` for **every** object-ish property, unconditionally
(`CkReflection_Utils.cpp:664-674`), and `Get_StructLiteral` recurses through it (`:529`). Calling it
on an object property turns `_Skeleton = assets::load::SK_Mannequin();` into `_Skeleton = nullptr;`.
**Dispatch object-ish properties to the accessor resolver before that helper is ever consulted.**
Do not modify `CkReflection_Utils.cpp:664-674` — its `nullptr` is correct for its original caller.

**Trap B — a partial write is actively destructive.** After your write, the AS directory watcher
fires, `__Init_` re-runs, and every non-instanced property is reset from the CDO
(`Bind_UObject.cpp:477-488`). So "skip the unresolvable property, write the rest" **destroys the
user's edit to the skipped property in the same gesture that claims to save their work.** Resolve
everything first; abort the whole write otherwise.

**Trap C — the patch-set predicate is not the button-enable predicate.** Do **not** select what to
write by diffing against the class CDO. A hand-authored accessor line exists *because* it differs
from the CDO, and every canonical in-repo asset populates a container via `.Add()` — CDO-diffing
would put deferred containers into the patch set and abort every write. Select by diffing against
**the value the current file text produces** (PLAN §5.1). "Differs from class CDO" is only the
button-enable condition.

---

## 4. Phases

Each phase has a verification gate. Do not start a phase until the previous gate is green.
**Report the gate result as a delta against the baseline you captured in Phase 0** — "baseline N
failing {names} → still N {names}", never a bare "tests pass".

### Phase 0 — Baseline and open questions

- Capture the pre-existing build + automation-test baseline: pass/fail counts and **the names of any
  already-failing tests**. You cannot claim "no regressions" later without this.
- Confirm the base commit and that the working tree is clean of anything you did not author. A
  sibling session may be active in this worktree — see §6.
- **Settle open question 1 (PLAN §7.1):** does re-running `__Init_<Name>` on a scratch instance have
  side effects? Bodies may call global functions (`assets::AutoTests_CkTests_Level()` at
  `CkTests_AutoTestMapConfig.as:34`). CkFoundation already re-runs these in its heal sweep
  (`CkDeferredAssetInit_AngelScript.cpp:345-375`), so this is expected to be benign — confirm it on a
  real asset rather than assuming.
- **Settle open question 2 (PLAN §7.5, INFERRED):** are watcher-queued reloads processed inside a
  modal dialog's Slate loop? The §5.2 freshness guard makes the answer immaterial, so this is
  information, not a blocker.

**Gate:** baseline numbers recorded; both questions answered in writing.

### Phase 1 — Pure text patcher

`WriteBack/CkAngelscriptGenerator_AssetBlockPatcher.{h,cpp}`, namespace
`ck::angelscriptgenerator::write_back`. No UObject or editor coupling — this must be testable
headless.

Locate the block (metadata `ScriptAssetFilename` → module code-section scan for `__Init_<Name>(` →
**loud abort if both fail**), regex `asset\s+<Name>\s+of\s+<Type>` skipping comment matches,
brace-match the body, then apply replace / insert / delete per PLAN §5.2. Preserve leading
whitespace, trailing comments, line endings (CRLF vs LF), and any BOM.

**Gate:** `Tests/Test_AssetBlockPatcher.cpp` green across every case in PLAN §8 "Patcher" — including
braces inside strings/comments, a commented-out `asset X of Y` earlier in the file, duplicate
property assignment, `_Mesh` vs `_MeshScale` prefix collision, empty-body insert, CRLF/LF and BOM.

### Phase 2 — Accessor resolver

`WriteBack/CkAngelscriptGenerator_AccessorResolver.{h,cpp}`. Also pure.

Parse the **generated** `.as` accessor files as the source of truth — not the in-memory
`AssetPathToFunctionName` map (no namespace per entry, wiped per config) and not recomputation
(`_DUP{N}` makes names non-derivable). Extend the parsing shape at
`CkAssetRegistrySubsystem.cpp:1169-1249` to capture object path, namespace, function name, whether a
`_Class` accessor exists, and whether the entry sits inside `#if Editor`. Emit per PLAN §5.3.

Every loud-skip case gets its **own distinct message**. In particular "no provider registered at
all" must be reported differently from "no accessor exists" — see the `Get_HasAnyProvider()` doctrine
at `CkAssetReferenceProvider.h:52-55`.

**Gate:** `Tests/Test_AccessorResolver.cpp` green across every case in PLAN §8 "Resolver".

### Phase 3 — Diff engine

Scratch-baseline construction (`NewObject` → reset from CDO → execute `__Init_<Name>`, mechanism at
`CkDeferredAssetInit_AngelScript.cpp:345-375`) plus the `(live, baseline)` pair recursion for structs
(PLAN §5.4). Do **not** call `Collect_StructFieldOverrides` — it diffs a zero-init buffer and would
produce phantom edits.

**Gate:** pair-recursion tests green (PLAN §8 "Pair-recursion"), including the struct whose CDO
customises a field, which must produce **no** phantom edit.

### Phase 4 — UI and orchestration

`WriteBack/CkAngelscriptGenerator_AssetWriteBack.{h,cpp}` — all UObject/editor coupling lives here.

Dynamic section on `FAssetEditorToolkit::DefaultAssetEditorToolBarName`, reading the edited object
from `UAssetEditorToolkitMenuContext::GetEditingObjects()`. Visibility and enablement per PLAN §5.5 —
**cache the enabled state off `FCoreUObjectDelegates::OnObjectPropertyChanged`**; a per-frame
`Identical` sweep is not acceptable. Confirmation dialog shows the exact line diff, the lossy-`FText`
note when relevant, and the VS Code unsaved-buffer caveat.

**Implement the confirm-time freshness guard** (PLAN §5.2): after the user confirms, re-read the file
and verify its bytes still match the snapshot; if not, discard and restart the diff.

**Gate:** editor builds clean via the toolbox; button appears on a real literal asset and is absent
everywhere else.

### Phase 5 — Integration verification

On a real asset whose body contains an `.Add()` container — `CkIskmRenderer_Assets.as` is the
intended specimen — edit a scalar in the details panel, write back, and confirm:

1. The `.as` diff is **exactly** the intended line(s) and nothing else.
2. The container's `.Add()` lines are byte-identical (proves Trap C is closed).
3. Hot reload re-runs and the written value survives the `__Init_` round trip.

**Gate:** all three confirmed by observation, not inference. Re-run the full automation suite and
report the delta against the Phase 0 baseline.

---

## 5. Build and test — non-negotiable

Build and run automation tests **only** through the Unreal Toolbox, per the `/build-test` skill
(`CkAuto/UnrealToolbox.exe`). **Never** invoke `Build.bat`, UnrealBuildTool, or
`UnrealEditor-Cmd.exe -ExecCmds="Automation RunTests …"` directly — the toolbox owns engine
resolution, the machine-wide build lock, watchdogs, and structured results. This holds even if older
docs or scripts in the repo show raw invocations.

A "completed" toolbox notification is a proxy, not ground truth. Gate on the real artifact — the
editor log verdict, the process exit, the on-disk file. If output looks mid-flight despite a done
signal, it probably is.

**Stale-green trap:** if you edit after a build snapshotted, re-run the full gate on the final binary
before claiming anything. A green run from the wrong binary is worse than no run.

---

## 6. Guardrails

- **Scope.** Stage only files you authored. Never blanket `git add <dir>` — this machine often has a
  sibling session in another worktree, and a blind add silently reverts their committed work.
  Enumerate by name anything dirty you did not author and leave it alone.
- **Do not commit or push unless asked.** Report status; let the requester decide.
- **Do not fix the `AssetPathToFunctionName` map-wipe defect** (PLAN §7.3). It is pre-existing and
  out of scope. File a follow-up note instead.
- **Do not edit engine code** under `D:\Repositories\UnrealEngine-Angelscript`. It is reference only.
- **Do not modify `CkReflection_Utils.cpp:664-674`.** Trap A.
- If a settled decision (PLAN §9) turns out to be wrong, stop and raise it — do not silently
  re-architect.

---

## 7. Definition of done

- All five phase gates green, with the Phase 5 integration checks confirmed by observation.
- Automation suite delta reported against the Phase 0 baseline, with any inherited failures named
  and attributed.
- New tests cover every case listed in PLAN §8.
- A short status covering: what you ran and its result; what you inferred but did not confirm; what
  only the requester can verify from where they sit; and the one claim you'd most expect to be
  wrong.
