# AngelScript Literal-Asset Write-Back — Implementation Plan

**Date:** 2026-08-28
**Target module:** `CkAngelscriptGenerator` (CkFoundation)
**Engine:** UE 5.5/5.6 Chainkemists fork with Hazelight AngelScript — `D:\Repositories\UnrealEngine-Angelscript`
**Status:** GREEN-LIGHT after CTO review (rev 2). Both blocking findings applied. Not yet implemented.
**Review:** [2026-08-28-as-literal-asset-writeback-CTO-review.md](../../reviews/2026-08-28-as-literal-asset-writeback-CTO-review.md)

---

## 1. Problem

A literal AS asset is declared as `asset <Name> of <UClass> { <initializer statements> }` in a `.as`
file (canonical doc: [Script/ARCHITECTURE.md:528-600](../../../Script/ARCHITECTURE.md)). The AS
preprocessor rewrites that declaration into a cached `Get<Name>()` getter plus
`void __Init_<Name>(<Type>)` holding the body (`AngelscriptPreprocessor.cpp:3951-4035`), and the
object is created into the `/Script/AngelscriptAssets` package (`Bind_UObject.cpp:406-500`).

Consequence: the asset has no `.uasset` backing it, so **edits made in the editor's details panel
cannot be saved**. Ctrl+S produces `"Cannot save asset declared as an angelscript asset literal"`
(`AngelscriptEditorModule.cpp:387`). The engine's only write-back is `UCurveFloat`-specific and
routes through `FAngelscriptManager::ReplaceScriptAssetContent`, which requires the VS Code
extension to be attached as a debug-server client (`AngelscriptManager.cpp:940-948`).

**Goal:** a "Write Back to Script" button in the asset editor that patches the live property values
back into the `asset … of … { }` block in the owning `.as` file. No VS Code dependency, no Python.

---

## 2. Scope

### In scope (v1)

- Toolbar button in the asset editor for qualifying literal AS assets.
- Surgical patch of the asset block: replace / insert / delete individual assignment lines.
- Scalars, enums, strings, names, text, structs.
- **Object references** — soft, hard, class, and native-class refs, emitted as generated
  `assets::` accessor expressions.
- All-or-nothing write with a pre-write confirmation dialog showing the exact line diff.
- Automation tests over the pure text-patching and expression-resolution layers.

### Explicitly deferred

- **Containers** (`TArray`/`TSet`/`TMap`), including containers of object refs. Under the §5.1
  predicate these are *invisible* rather than blocking: a container populated by `.Add()` in the
  body matches the text-produced snapshot, so it never enters the patch set. A container the user
  edits **in the details panel** does enter it, cannot resolve, and correctly aborts the write.
- `#if EDITOR` auto-wrapping of a single emitted line.
- Cross-file literal-asset accessor discovery (scanning for hand-authored `return Asset_Foo;`
  wrappers).
- Weak / lazy / interface properties — skipped loudly.
- Fixing `UCkAssetRegistrySubsystem`'s own map-wipe defect (§7.3) — pre-existing, flagged only.

---

## 3. Verified ground truth

Every claim below was read at the cited location. All were independently spot-checked during CTO
review; none refuted.

| # | Fact | Evidence |
|---|---|---|
| 1 | Literal-asset syntax is `asset <Name> of <Type>`; preprocessor emits `__Asset_<Name>`, caching `Get<Name>()` property, and `__Init_<Name>(<Type>)`. Asset-name uniqueness is enforced **globally across modules**, not merely per module. | `AngelscriptPreprocessor.cpp:3953`, `:3974-3999`, `:4004-4016` |
| 2 | Assets are created into `FAngelscriptManager::Get().AssetsPackage`; the package stores `ScriptAssetFilename` / `ScriptAssetLineNumber` metadata per object, via the non-deprecated `FMetaData&` form of `GetMetaData()` in this fork. | `Bind_UObject.cpp:440-466` |
| 3 | Reading that metadata is established practice in-tree. | `CkAutoTestMapPopulator.cpp:1060` |
| 4 | On hot reload the existing instance is **reset from the CDO** for every `!ContainsInstancedObjectProperty()` property, then `__Init_` re-runs. | `Bind_UObject.cpp:477-488` |
| 5 | A directory watcher on the script roots triggers that hot reload on an external file write; it queues reloads only for `.as` extensions, so one temp+move write fires exactly one reload. | `AngelscriptEditorModule.cpp:405-418` |
| 6 | Every asset-editor toolbar's parent menu is `AssetEditor.DefaultToolBar`. Per-editor names are `AssetEditor.<AppName>.ToolBar`, where `<AppName>` is `<ClassName>Editor` **only for simple asset editors with exactly one edited object**, else `GetToolkitFName()`. We hook the parent, so this does not affect the design — do not key off the per-editor name. | `AssetEditorToolkit.cpp:52`, `:1229-1255` |
| 7 | `UCk_Utils_Reflection_UE::Get_PropertyDefaultValueLiteral` returns the literal `nullptr` for **every** object-ish property, unconditionally. `Get_StructLiteral` recurses through it. | `CkReflection_Utils.cpp:664-674`, `:529` |
| 8 | `Collect_StructFieldOverrides` diffs against a fresh `InitializeStruct` buffer, **not** the owning class CDO. | `CkReflection_Utils.cpp:722-736` |
| 9 | `AssetPathToFunctionName` is `TMap<FString,FString>` (bare fn name); namespaces live in a separate `ActiveNamespaces` set. | `CkAssetRegistrySubsystem.h:187`, `:195` |
| 10 | That map is `Reset()` per config, and re-seeding only fires when it is empty. | `CkAssetRegistrySubsystem.cpp:531`, `:1259-1263` |
| 11 | Accessor names go through `_DUP{N}` dedup resolved against sorted-scan order — **the accessor name is not derivable from the asset name**. | `CkAssetRegistrySubsystem.cpp:676-685`, `:534-536` |
| 12 | Editor-only assets emit their accessor wrapped in `#if Editor` / `#endif`. | `CkAssetRegistrySubsystem.cpp:691`, `:699` |
| 13 | `FCk_AssetReferenceProviderRegistry` exists in CkCore as a registry-inversion so "neither side links the other"; exposes `Get_HasAnyProvider()`. | `CkAssetReferenceProvider.h:64-113` |
| 14 | A bare class name is a legal `TSubclassOf` expression in AS. | `CkUtils_Actor.as:223`, `:248` |
| 15 | `nullptr` assigns to `TSubclassOf` and `TWeakObjectPtr` in AS. | `CkGym_StationSm.as:151`, `CkTests_EntitySpawnParams.as:15506` |
| 16 | A same-file literal asset is referenceable by bare name; cross-file ones use a hand-authored namespace wrapper. | `CkIskmRenderer_Assets.as:58`, `:87-91` |
| 17 | `CkModuleRules` adds `AngelscriptCode` + `WITH_ANGELSCRIPT_CK=1` to every Ck module automatically. | `CkBuildConfig.Build.cs:53-60` |
| 18 | `CkAngelscriptGenerator` already links `PropertyEditor` and `ToolMenus`. | `CkAngelscriptGenerator.Build.cs` |
| 19 | `FAngelscriptCodeModule::GetPostLiteralAssetSetup()` broadcasts `(Asset, Name)` **after** `__Init_` has run — the seam for the §5.1 snapshot. | `AngelscriptCodeModule.h:45`, `Bind_UObject.cpp:496-503` |
| 20 | `nullptr` assigns to `TSoftObjectPtr` — null-handle `ImplicitConstructor` and `opAssign(T handle_only Object)` binds both exist. `TSoftClassPtr` likewise. | `Bind_TSoftObjectPtr.cpp:442`, `:454`, `:590` |
| 21 | `FAngelscriptModuleDesc::FCodeSection` stores `{RelativeFilename, AbsoluteFilename, Code}` where `Code` is the **processed** source, so `__Init_<Name>(` is findable in it. `GetModuleContainingLiteralAsset` exists. | `AngelscriptManager.h:235`, `:1108-1121` |
| 22 | CkFoundation already re-runs `__Init_<Name>` on an arbitrary instance (`Prepare` + `SetArgObject(0, Instance)`) — the mechanism the §5.1 scratch re-init reuses. | `CkDeferredAssetInit_AngelScript.cpp:345-375` |

---

## 4. The three correctness traps

**Trap A — silent reference destruction.** Fact 7. Reusing `Get_PropertyDefaultValueLiteral` for an
object property rewrites `_Skeleton = assets::load::SK_Mannequin();`
(`CkIskmRenderer_Assets.as:31`) into `_Skeleton = nullptr;`. `Get_StructLiteral` recurses through the
same helper, so nested struct fields carry the identical hazard. **Object-ish properties must be
dispatched before that helper is ever consulted.** Do not modify `CkReflection_Utils.cpp:664-674` —
its `nullptr` is correct and documented for its original caller.

**Trap B — a partial write is actively destructive.** Fact 4 + Fact 5. After our write the watcher
fires, `__Init_` re-runs, and every non-instanced property is reset from the CDO. So "skip the
unresolvable property, write the rest" **destroys the user's in-editor edit to the skipped property
in the same gesture that claimed to save their work.** Resolution must therefore be complete
*before* the first byte is written.

**Trap C — the patch-set predicate is not the button-enable predicate.** Diffing against the class
CDO to decide *what to write* is wrong, and was the original plan's central defect:

- A hand-authored accessor line exists *because* its value differs from the CDO
  (`_Skeleton = assets::load::SK_Mannequin();` vs a null CDO). CDO-diffing flags every such line as
  changed and regenerates its RHS on every write-back. Survival would depend on the resolver
  emitting byte-identical text, not on the line being left alone.
- Worse, all three canonical literal-asset patterns in `Script/ARCHITECTURE.md` §13 populate a
  **container** via `.Add()` — `GameplayTags.Add(…)`, `_Parameters.Add(Tint)`, `_Sequences.Add(…)`.
  Containers are deferred, so under CDO-diffing they would enter the patch set, fail to resolve, and
  abort the entire write. **v1 could not have written back a scalar edit on any flagship in-repo
  asset.**

The correct patch-set predicate is "**differs from the value the current file text produces**"
(§5.1). "Differs from the class CDO" remains the *button-enable* condition, as specified by the
requester — the two are different questions and only the latter was ever settled.

---

## 5. Design

### 5.1 Pipeline

Three stages, in order. Stage 2 never runs unless stage 1 says the value changed; stage 3 never runs
unless stage 2 resolved *everything*.

**Stage 1 — value-space diff against the text-produced state.**

Build a *scratch baseline*: `NewObject` of the asset's class into the transient package, reset it
from the CDO, then execute `__Init_<Name>` against it (mechanism: Fact 22). That object is, by
construction, exactly what the current file text produces. Diff each property with
`FProperty::Identical(live, scratch)`.

- Equal → the property is **not in the patch set**; its line is never touched, whatever its text.
  The no-churn guarantee is now literal, not contingent on resolver output.
- Containers populated by `.Add()` in the body match the baseline and never enter the patch set —
  which is what makes them genuinely deferrable rather than a blocking hole.
- Only what the user actually changed in the details panel is a candidate for writing.

*Alternative baseline:* subscribe to `GetPostLiteralAssetSetup()` (Fact 19) and snapshot there.
Rejected as primary because CkFoundation's own Phase-2 deferred-asset heal re-runs `__Init_` later
(`CkDeferredAssetInit_AngelScript.cpp`), so a load-time snapshot can go stale; the scratch re-init
is computed fresh at button press and has no staleness window. Build it lazily — button press only.

> **Risk to verify at implementation time:** `__Init_` bodies may call global functions
> (`assets::AutoTests_CkTests_Level()` at `CkTests_AutoTestMapConfig.as:34`). Per ARCHITECTURE §13
> they are initializer blocks, and CkFoundation already re-runs them routinely in the heal sweep, so
> side effects on a scratch object should be benign. Confirm on a real asset before shipping.

**Stage 2 — expression resolution.** Every property in the patch set is resolved to an AS expression
string. Object-ish leaves dispatch to the accessor resolver (§5.3); everything else to the existing
CkCore leaf formatter.

**Stage 3 — all-or-nothing write.** If any patch-set property failed to resolve, abort: dialog lists
each offender (property, object path, reason), file untouched. Only when all resolve do we patch and
write.

> Writing the resolvable subset anyway is available only as an explicit second user choice, with the
> CDO-stomp consequence (Trap B) stated in the dialog.

### 5.2 Text patching

Locate the file: package metadata `ScriptAssetFilename` (Fact 2) when non-empty and present on disk;
otherwise scan `FAngelscriptModuleDesc::Code[].AbsoluteFilename` from
`FAngelscriptManager::Get().GetModuleContainingLiteralAsset(<Name>)` for `__Init_<Name>(` (Fact 21).
**If both fail: loud abort, file untouched** — never guess a path.

Do **not** trust `ScriptAssetLineNumber` for positioning — it is an AS-context line number against
preprocessed code. Instead regex the original file for `asset\s+<Name>\s+of\s+<Type>` and brace-match
its body, skipping matches inside comments (the preprocessor does the same via
`ShallowCheckIsInComment`, `AngelscriptPreprocessor.cpp:3961`). Asset names are globally unique by
construction (Fact 1).

Per patch-set property, at brace-depth 0 within the body:

| Situation | Action |
|---|---|
| in patch set, existing `Name = …;` line | replace the RHS, preserve leading whitespace and any trailing comment |
| in patch set, no line | insert before the closing brace |
| in patch set, value now equals the **class CDO** and the property is object-ish-or-scalar with a line | delete the line (user reverted to default since load) |
| not in patch set | no-op |

Comments, locals, `Add()` loops, and every unrecognised line are left untouched. Preserve the file's
existing line endings (CRLF vs LF) and BOM — note `Try_AtomicWrite`'s `ForceUTF8WithoutBOM` strips a
pre-existing BOM, which would churn the whole file.

Write via atomic temp+move, mirroring `Try_AtomicWrite`
(`CkAngelscriptGenerator_StubSynthesizer.cpp:131-150`).

**Confirm-time freshness guard (required).** The patch is computed from a file read at button press,
but the confirmation dialog may sit open for minutes. In that window the `.as` can change on disk —
a VS Code *save* (distinct from the unsaved-buffer conflict in §7.4), or a watcher-driven hot reload
that additionally replaces the live object through the `REPLACED_ASSET_` rename path
(`Bind_UObject.cpp:420-427`). Because the write is a whole-file replace, a stale snapshot would
clobber newer content. **After the user confirms, re-read the file and verify its bytes still match
the snapshot; if not, discard and restart the diff.** Same standard as Trap B.

### 5.3 Object-reference resolution

**Source of truth: parse the generated `.as` accessor files.** Not the in-memory
`AssetPathToFunctionName` map (Facts 9, 10 — no namespace per entry, wiped per config), and not
recomputation from discovery (Fact 11 — `_DUP{N}` makes names non-derivable). The generated file is
the only statement of what compiles today. Extend the existing `SeedMapsFromGeneratedFiles()` parsing
shape (`CkAssetRegistrySubsystem.cpp:1169-1249`) to capture, per entry: object path, namespace,
function name, whether a `_Class` accessor exists, and whether the entry sits inside `#if Editor`.
Re-parse on button press — user-initiated, rare, milliseconds.

| Case | Emitted expression |
|---|---|
| Soft object ref | `<ns>::<Fn>()` — compare `FSoftObjectPath` strings, never load either side |
| Hard object ref | `<ns>::load::<Fn>()` — house style, `CkIskmRenderer_Assets.as:31` |
| Blueprint class ref | strip `_C`, require the class-accessor flag → `<ns>::[load::]<Fn>_Class()` |
| Native `UClass` ref | bare class name (Fact 14) |
| Same-file literal asset | bare asset name (Fact 16) |
| Live null, baseline non-null | explicit `nullptr` — a deliberate override, never conflated with unresolvable. Valid for object, `TSubclassOf`, `TWeakObjectPtr`, and `TSoftObjectPtr` (Facts 15, 20) |
| Live null, class-CDO null | delete line if present |
| Nested in a struct | own `(live, baseline)` recursion — see §5.4 |

**Loud-skip cases** (each aborts the write with its own distinct message):
cross-file literal asset (no machine-derivable expression; hint the `iskm_assets::` wrapper idiom
from `CkIskmRenderer_Assets.as:87-91`); editor-only accessor targeted from a non-`#if EDITOR` block;
no accessor under any discovery root; **no provider registered at all** — reported distinctly from
"no accessor exists", per the `Get_HasAnyProvider()` doctrine at
`CkAssetReferenceProvider.h:52-55`.

**`FText` is emitted as a plain literal** (`FText::FromString("…")`), matching what
`Get_AngelscriptDefaultExpression` already does for the `Text` kind. This **loses localization
namespace/key identity** — accepted, documented, lossy behaviour for v1. Note it in the confirmation
dialog when any `FText` is in the patch set.

### 5.4 Struct recursion

Do **not** call `Collect_StructFieldOverrides`. Fact 8: it diffs against a zero-init
`InitializeStruct` buffer, so a class whose CDO customises a struct field would show phantom edits
and churn lines the user never touched. Write a dedicated recursion carrying `(livePtr, baselinePtr)`
as a pair, dispatching object-ish leaves to §5.3 and reusing only the existing leaf formatter for
scalars.

*Optional, not required for v1:* hoist the pair-walker into `CkReflection_Utils` beside
`Collect_StructFieldOverrides`, parameterised on a leaf callback, giving that helper a
correctly-grounded sibling. Skip unless it falls out naturally.

### 5.5 UI

Dynamic section on `FAssetEditorToolkit::DefaultAssetEditorToolBarName` — the parent menu of every
asset-editor toolbar (Fact 6). The entry is added only when the context's single edited object
qualifies, so there is zero footprint in every other asset editor. Read the edited object from
`UAssetEditorToolkitMenuContext::GetEditingObjects()`.

- **Visible when:** outer is `FAngelscriptManager::Get().AssetsPackage` **and** the declared class is
  native (`Cast<UASClass>` null, not a `UBlueprintGeneratedClass`).
- **Enabled when:** at least one property differs from the class CDO (requester's condition, §9.3).
  A toolbar enabled-attribute is a polled `TAttribute` — a full-property `Identical` sweep per frame
  per open editor is not acceptable. **Cache the result with a dirty flag driven by
  `FCoreUObjectDelegates::OnObjectPropertyChanged`,** or compute once at section construction.
- **Tooltip:** lists what will be written and what cannot resolve.

*Rejected:* `RegisterCustomClassLayout` (needs per-class registration; a class with its own
customization would lose the button). Chaining `GAssetEditorToolkit_PreSaveObject` (a single global
`TFunction` already owned by the AS plugin, `AngelscriptEditorModule.cpp:450-460` — init-order
fragile).

---

## 6. Module placement and file layout

**`CkAngelscriptGenerator`.** It already owns `.as` emission, `.as` source scanning
(`SelfHeal/CkAngelscriptGenerator_AsSourceScanner.*`), the accessor naming rules, and config
discovery — and already links `PropertyEditor` + `ToolMenus` (Fact 18). Zero new plumbing.

`CkCoreEditor` was the original target and is **not viable**: its deps are `CkCore` + `CkLog` only,
and taking `CkAngelscriptGenerator` would drag the ECS stack into the core details module, against
the tier rule at `Source/CLAUDE.md:132`.

*Fallback if the patcher must ever live in CkCoreEditor:* extend `FCk_AssetReferenceProviderRegistry`
(Fact 13) with a sibling query `(FSoftObjectPath, ECk_ScriptAccessorKind) -> TOptional<{Expression,
IsEditorOnly}>`, hosted in CkCore and implemented by CkAngelscriptGenerator. Registration precedent:
`CkAssetRegistrySubsystem.cpp:288-294`. **Documented fallback only** — write-back is irreducibly
AS-specific (parses `.as`, emits AS expressions, reads AS metadata), so routing it through CkCore
would generalise an interface with exactly one conceivable implementor.

Layout mirrors `SelfHeal/`:

```
Source/CkAngelscriptGenerator/WriteBack/
    CkAngelscriptGenerator_AssetBlockPatcher.{h,cpp}    // pure text: locate, brace-match, patch
    CkAngelscriptGenerator_AccessorResolver.{h,cpp}     // pure: generated-file parse -> expression
    CkAngelscriptGenerator_AssetWriteBack.{h,cpp}       // toolbar entry + orchestration + dialog
Source/CkAngelscriptGenerator/Tests/
    Test_AssetBlockPatcher.cpp
    Test_AccessorResolver.cpp
```

Namespace `ck::angelscriptgenerator::write_back`, mirroring `ck::angelscriptgenerator::self_heal`.
No anonymous namespaces (unity builds) — use the filename-derived named namespace per
`CkFoundation/CLAUDE.md`.

---

## 7. Risks

1. **Scratch re-init side effects** (§5.1). `__Init_` bodies can call global functions. Believed
   benign — CkFoundation already re-runs them in the heal sweep — but confirm on a real asset.
2. **Hot-reload round trip is the safety net, not the risk.** A dropped or wrong value surfaces
   immediately when `__Init_` re-runs, rather than silently. Verify the round trip on a real asset
   before declaring the feature done.
3. **`AssetPathToFunctionName` map-wipe (Fact 10) is a pre-existing defect** in the subsystem's own
   delete-guard. This plan routes around it by parsing generated files. Do not fix it here; file a
   follow-up.
4. **VS Code holding the `.as` file with unsaved changes** — our atomic write wins on disk and the
   editor shows a conflict. Acceptable; note it in the confirmation dialog. The *saved*-in-between
   case is closed by the §5.2 freshness guard.
5. **INFERRED, worth one check:** whether watcher-queued reloads are processed inside a modal
   dialog's Slate loop. The §5.2 freshness guard makes the answer immaterial either way.

---

## 8. Test gate

Pure-function tests over the patcher and the resolver, in the style of
`CkAngelscriptGenerator/Tests/Test_StubSynthesizer.cpp` (temp-file fixtures). The patcher and
resolver must be callable without an editor — keep all UObject/editor coupling in
`CkAngelscriptGenerator_AssetWriteBack.cpp`.

**Patcher:** replace-existing line; insert-new; delete-reverted; insert into an empty body;
namespace-nested block; `#if EDITOR`-guarded block; comment preservation; `Add()`-loop preservation;
unrecognised-line preservation; brace-matching with a nested struct literal in the body; **braces
inside string literals and comments**; **a commented-out `asset X of Y` earlier in the file**;
**the same property assigned twice in one body** (define and test which line is patched);
**trailing-comment preservation on RHS replace**; **property-name prefix collision** (`_Mesh` vs
`_MeshScale`); **CRLF vs LF preservation**; **BOM preservation**.

**Resolver:** soft / hard / BP-class / native-class / same-file-literal-asset emission; null vs
unresolvable distinction; `_DUP{N}` name honoured from the generated file; editor-only entry
targeted from a non-editor block rejected; no-provider vs no-accessor reported distinctly;
**duplicate asset path across two generated accessor files**.

**Pair-recursion (§5.4):** nested struct with a changed scalar leaf; nested struct with a changed
object leaf; struct whose CDO customises a field (must produce no phantom edit).

**Integration (manual, on a real asset):** edit a value in the editor → write back → confirm the
`.as` diff is exactly the intended lines → confirm hot reload re-runs and the value survives.
Use an asset whose body contains an `.Add()` container (e.g. `CkIskmRenderer_Assets.as`) to prove
Trap C is closed.

**Build + tests run via the Unreal Toolbox only** (`/build-test` skill). Never `Build.bat`,
UnrealBuildTool, or `UnrealEditor-Cmd` directly.

---

## 9. Settled decisions — do not relitigate

1. Surgical line patch, never whole-body regeneration. Bodies contain arbitrary AS code.
2. Direct atomic file write. Not `ReplaceScriptAssetContent` (needs VS Code attached).
3. **Button-enable** condition is "differs from the class CDO", per the requester's wording — the
   button is live whenever the asset carries any non-default value. This is *not* the patch-set
   predicate; see §5.1 and Trap C.
4. Native-class-only gate on visibility, per the requester's parent-must-be-native rule.
5. Accessor source is the generated `.as` files, not the in-memory map and not recomputation.
6. All-or-nothing write, for the Trap B reason.
7. Home is `CkAngelscriptGenerator`.
8. Patch-set predicate is "differs from the text-produced baseline" (§5.1) — added rev 2 after CTO
   review.
