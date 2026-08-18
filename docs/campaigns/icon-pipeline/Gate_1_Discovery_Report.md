# Icon Pipeline Campaign — Phase A, Gate 1 Discovery Report

Authored 2026-08-17 (read-only discovery; no source edits). All paths relative to
`D:/Repositories/CkRepos/BusterBlock/Plugins/` unless noted. Every claim is CONFIRMED against
source unless marked INFERRED.

---

## 0. Documents consulted

| Document | What it said |
|---|---|
| `CkFoundation/CLAUDE.md` | Doctrine: naming (`ECk_`/`FCk_`/`UCk_Utils_`), `CK_ENSURE_IF_NOT` (never stock ensure), per-module `ck::<feature>` logging via `CK_DEFINE_LOG_FUNCTIONS`, `CK_DEFINE_CUSTOM_FORMATTER_ENUM` on every UENUM, module-authoring rules |
| `CkFoundation/Source/CLAUDE.md` | Module decision tree: "style tokens for editor-tool Slate UI → `CkEditorTools` + `CkStyle::`" — **Runtime on purpose, consumed by CkGameplayDebugger's Runtime modules**. Tier table: runtime never depends on T5 editor modules |
| `CkFoundation/Source/EDITOR_MODULES.md:16-19` | CkEditorStyle = "shared icon/color/font style for all CkFoundation editor UIs". **STALE**: it says "reference via `FCk_EditorStyle::GetStyleSet()`" — that symbol exists nowhere in Source (grep: 0 hits). Real API: `UCk_Utils_EditorStyle_UE::Get()` (`CkFoundation/Source/CkEditorStyle/Public/CkEditorStyle/CkEditorStyle_Utils.h:94`). Source wins; doc flagged |
| `CkGameplayDebugger/Source/CkDebuggerCommon/CLAUDE.md` | Icon-registry doctrine: `:23-26` registry scanned from `Resources/Icons/**`, feature modules must adopt it, never register module-local sets; `:50-86` severity iconography (one tone → one glyph, pinned by test); `:77-78` null-brush contract (unknown id → nullptr → widgets draw nothing); `:73-76` icons deliberately NOT taken from `FAppStyle` because several debugger modules **ship in packaged Development/DebugGame builds**; `:100-101` all glyphs go through `SCkDebug_Icon`/`SCkDebug_IconToggle` |
| `CkGameplayDebugger/Source/CkDebuggerCommon/_Design/2026-08-09-style-liveness-and-completeness.md` | R7: no per-frame brush allocation — all variants come from brushes registered once in `FCkDebuggerStyle` |
| `CkGameplayDebugger/PROGRESS.md:50`, `docs/specs/2026-07-10-ecs-debugger-redesign.md:189` | Icon-system history: monochrome white SVGs, tinted per-feature at draw time — a deliberate, campaign-reviewed design |

---

## 1. Inventory

### 1.1 Slate style infrastructure (Ck-family plugins)

| # | Item | Status | Evidence |
|---|---|---|---|
| 1 | `UCk_Utils_EditorStyle_UE` / set `CkFoundation_EditorStyle` | EXISTS | `CkFoundation/Source/CkEditorStyle/Public/CkEditorStyle/CkEditorStyle_Utils.cpp:36-40` (create+register), `:53` (unregister), content root `CkFoundation/Resources/Editor` `:38`. Module type **UncookedOnly, PreDefault** (`CkFoundation.uplugin`). Guarded against `IsRunningGame()` (`:30-31`) — editor-only by design |
| 2 | `FCkAnimationToolboxStyle` | EXISTS | `CkFoundation/Source/CkAnimationEditor/Public/CkAnimationEditor/Toolbox/CkAnimationToolbox_Style.cpp:21` register / `:32` unregister; PNG brush `:60` |
| 3 | `FCkCueToolboxStyle` | EXISTS | `CkFoundation/Source/CkCueEditor/Public/CkCueEditor/Toolbox/CkCueToolboxStyle.cpp:18/:24`; content root via `IPluginManager` `:51`, engine Slate root `:56` |
| 4 | `FCkDebuggerStyle` / set `CkDebuggerStyle` — THE debugger suite set | EXISTS | `CkGameplayDebugger/Source/CkDebuggerCommon/Public/CkDebuggerCommon/Styles/CkDebuggerStyle.cpp:34/:45`; content root `:81` (path-only `UCk_Utils_IO_UE::Get_PluginsDir`, comment explains avoiding Projects dep); **icon scan** `:173-199`: every `Resources/Icons/**.svg` → `CkDebugger.Icon.<BaseName>`, `FSlateVectorImageBrush` 16×16, filenames sorted for deterministic General-pool hashing; `Get_IconBrush` `:216` (`GetOptionalBrush` → nullptr on unknown); `Get_GeneralIconPool` `:208` |
| 5 | `FCkDebuggerCommonStyle` | EXISTS | `.../Styles/CkDebuggerCommonStyle.cpp:38/:49`; `IPluginManager::FindPlugin` + `CK_ENSURE_IF_NOT` on failure; **re-registers the same corpus** as `CkCommon.Icon.<BaseName>` 16×16 (`:181-196`) |
| 6 | `FCkDebuggerLauncherStyle` | EXISTS | `CkGameplayDebugger/Source/CkDebuggerLauncher/Private/Styles/CkDebuggerLauncherStyle.cpp:24/:32`; content root `:64`; **re-registers the corpus a third time** as `CkDebuggerLauncher.Icon.<BaseName>` at **24×24** (`:124`) |
| 7 | `FCkGoapDebuggerStyle` | EXISTS | `CkGameplayDebugger/Source/CkGoapDebugger/Public/CkGoapDebugger/CkGoapDebuggerStyle.cpp:25/:36` — color/box brushes only, no icons |
| 8 | `IMAGE_BRUSH`-family macro definitions | EXISTS (local) | `CkEditorStyle_Utils.cpp:9-10` — `IMAGE_PLUGIN_BRUSH_PNG` / `IMAGE_PLUGIN_BRUSH_SVG`. Engine `SlateStyleMacros.h` not used elsewhere in Ck code; debugger constructs `FSlateVectorImageBrush` directly |
| 9 | Brush-accessor utilities | EXISTS | Two idioms: BPFL `UCk_Utils_EditorStyle_UE` (`CkEditorStyle_Utils.h:94`) with settings-driven `Register_CustomSlateStyle` (callers: `CkEditorStyle/Settings/CkEditorStyle_Settings.cpp:231-241`, `CkEcsEditor_Module.cpp`, `CkInventoryEditor_Module.cpp`); and static-class accessors `F*Style::Get_IconBrush(FName)` (items 4-6) |
| 10 | Content-root sources | EXISTS | Path-only `UCk_Utils_IO_UE::Get_PluginsDir` (`CkEditorStyle_Utils.cpp:38`, `CkDebuggerStyle.cpp:81`) AND `IPluginManager::FindPlugin` (`CkCueToolboxStyle.cpp:51`, `CkDebuggerCommonStyle.cpp:~130`, `CkDebuggerLauncherStyle.cpp:~60`). IPluginManager is already in use |
| 11 | Style sets in CkTests / GitLink | ABSENT | `rg "RegisterSlateStyle" CkTests GitLink` → 0 hits |

Module-local style *headers* also exist that are token namespaces, not style sets (e.g.
`CkGameplayDebugger/Source/CkAStarDebugger/Public/CkAStarDebugger/CkAStarDebuggerStyle.h` — domain
color ramps; no `FSlateStyleSet`).

### 1.2 Existing resources

| Item | Status | Evidence |
|---|---|---|
| `CkFoundation/Resources/Editor` | EXISTS | 18 files: 8 bespoke asset-class SVGs (`Icon_EntityScript.svg` — 128×128, fill-based, mask/fill-rule; NOT the debugger idiom) + 16px/64px PNG twins |
| `CkFoundation/Source/CkPmg/Resources` | EXISTS | Noto fonts + **their OFL license .txt files beside the assets** — the house pattern for third-party asset licensing; staged `NonUFS` (`CkPmg.Build.cs:54-55`) |
| **`CkGameplayDebugger/Resources/Icons` — the icon corpus** | EXISTS | **206 SVGs: 58 semantic at root + 148 decorative pool in `General/`.** Uniform format: `viewBox="0 0 16 16"`, `fill="none"`, `stroke="#FFFFFF"`/`white`, stroke-width 1.5, round caps/joins (grep: 206/206 `fill="none"`, 206/206 16×16 viewBox). **First-party** — no license file in the plugin besides the repo's own `CkGameplayDebugger/LICENSE.md`; no MDI/Pictogrammers/other-set provenance anywhere (`rg -i "material design|pictogrammers|@mdi|game-icons|lucide|feather|fontawesome"` → 0 relevant hits) |
| `CkGameplayDebugger/Resources/GraphEditor` | EXISTS | 5 PNGs (copies of engine GraphEditor art), registered `CkDebuggerStyle.cpp:117-131` |
| `CkGameplayDebugger/Source/CkDebuggerCommon/Resources` | EXISTS | 2 glow PNGs + `Devices/Gamepad_Master.svg` |
| Third-party license tracking precedent | EXISTS | License file beside asset (CkPmg OFL), per-lib LICENSE in `CkThirdParty/Public/CkThirdParty/<lib>/`, campaign-doc license copy (`docs/campaigns/voxelnav-port/LICENSE.Nav3D.txt`) |

### 1.3 Existing tooling and codegen

| Item | Status | Evidence |
|---|---|---|
| Script tooling | EXISTS | `CkFoundation/Source/CkScripts/` (support dir, no Build.cs): PowerShell + Python, manual invocation — `Export-CkAssets.ps1`, `Show-CkAssetExport.ps1`, `CkLfsLocks.py`, `CkEcsTemplateReplacer.ps1` (stale) |
| In-editor codegen | EXISTS | `CkAngelscriptGenerator` (Editor module, PreDefault) emits `Script/Generated/*`; deterministic **write-if-changed** (load existing, compare, save UTF8-no-BOM: `CkAngelscriptEntityScriptParamsGenerator.cpp:488,557`); AutoTest wrapper generator same shape (`AutoTests/CkAutoTestWrapperGenerator.cpp:507,530`). Generated files committed, banner-marked |
| Codegen wired into UBT | ABSENT | No UBT-hooked generation anywhere in the plugin ecosystem |
| npm / node | ABSENT | 0 `package.json` across CkFoundation/CkGameplayDebugger/CkTests. House scripting languages: PowerShell (primary), Python |

### 1.4 Conventions confirmed

- Naming `ECk_`/`FCk_`/`UCk_Utils_`: pervasive (e.g. `ECk_IconSize`, `CkEditorStyle_Utils.h:14-24`).
- `CK_ENSURE_IF_NOT` already used in style code (`CkDebuggerCommonStyle.cpp` plugin-lookup guard, `CkDebuggerLauncherStyle.cpp:~60`).
- Enum reflection: `CK_DEFINE_CUSTOM_FORMATTER_ENUM` per UENUM; `CkCore/Public/CkCore/Enums/CkEnums.h`.
- **Module placement**: any icon registry the debugger consumes must be **Runtime-capable** — CkDebuggerCommon is a Runtime module shipping in packaged Dev builds (`CkDebugger.uplugin`; `CkDebuggerCommon/CLAUDE.md:73-76`). `CkEditorStyle` is **UncookedOnly** → cannot host it. `CkEditorTools` is Runtime **by design** for exactly this consumer (`Source/CLAUDE.md` T1 row).
- **Staging**: `RuntimeDependencies.Add(..., StagedFileType.NonUFS)` precedent exists and already covers all debugger icons (`CkDebuggerCommon.Build.cs:66-75` — with a comment explaining why; `CkDebuggerLauncher.Build.cs:28-34`; `CkPmg.Build.cs:54-55`).

### 1.5 The consumer baseline (recorded, untouched)

- **Mechanism**: three style sets scan `Resources/Icons/**` into prefixed string keys (§1.1 items 4-6). Consumers resolve via `F*Style::Get_IconBrush(FName)` — **58 call sites** (non-style, non-test), of which **~20 pass inline `FName{TEXT("...")}` literals** (e.g. `SCkSaveDebuggerWindow.cpp:464` `"Pin"`, `:547` `"Cassette"`). Inspector contract: `Get_IconName()` (`CkDebuggerInspector_Base.h:32,36`). Tone→glyph single mapping: `ck::debug_axes::Get_ToneIconId` (`CkDebuggerAxes.h:329`). Archetype fallback: hash-pick from `Get_GeneralIconPool()` (`CkDebuggerStyle.cpp:208`) — a runtime-enumerated pool.
- **Failure mode**: unknown id → `GetOptionalBrush` → nullptr → `SCkDebug_Icon`/`SCkDebug_IconToggle` draw nothing (`CkDebuggerCommon/CLAUDE.md:77-78`). This is **silent-blank**, not the engine default-brush-plus-warning path. Their own docs name it "the silent-nullptr class of bug" (`CLAUDE.md:81`), mitigated today by spec tests (`CkDebuggerToneIcons.spec.cpp` asserts each tone glyph resolves in BOTH sets; also `CkDebuggerStyle.spec.cpp`, `CkEcsDebugger_InspectorIcons.spec.cpp`, `CkDebuggerLauncherCatalog.spec.cpp`).
- A few unicode markers in display strings exist (`SCkAggroDebuggerWindow.cpp:614` `▶`, `SCkAudioDebuggerWindow.cpp:1958` `→`) — Phase B inventory fodder.

---

## 2. Does a style set to EXTEND exist, or must one be created?

**Created.** No CkFoundation-hosted, runtime-capable icon style set exists:

- `CkFoundation_EditorStyle` exists but is UncookedOnly + purposed for editor asset icons/thumbnails — wrong lifetime and wrong charter.
- `FCkDebuggerStyle` is the right *shape* but lives in the consumer plugin, which Phase A must not touch.

Recommended home: a new style set **inside `CkEditorTools`** (Runtime, T1, already the shared
styling home consumed by every debugger module) — a new module is not warranted for ~3 files.

---

## 3. Constraint conflicts (the "do not be agreeable" section)

The constraint block's premise predates what is actually in this repo. Point by point:

1. **"Icon set: MDI"** — conflicts with repo reality. The exact consumer already carries a
   deliberate, documented, test-pinned corpus of **206 first-party icons** in a uniform visual
   language: 16×16, `fill="none"`, white 1.5px strokes. That idiom **matches Epic's Starship
   icons** (engine `Content/Slate/Starship/Common/check.svg`: `fill="none" stroke="white"`;
   152/152 sampled Common SVGs are `fill="none"`) — the campaign's own goal of matching Starship
   chrome argues *for* the house corpus. MDI glyphs (filled paths on a 24-grid — the `-outline`
   variants are also filled paths, just outline-styled artwork) would introduce a second, heavier
   visual language and licensing bookkeeping the current corpus doesn't carry.
2. **"Every SVG must have `fill=#FFFFFF` written onto its paths"** — true **for MDI assets
   specifically**; as a general rule the requirement is *white ink*. White **stroke** tints
   identically (brush tint multiplies), which is how all 206 existing icons and Epic's own render
   today. If MDI is vendored, the recolour step is real and needed — for MDI only.
3. **"Register 16x16 and 20x20"** — the per-size-brush principle is sound and confirmed, but the
   sizes in actual use here are **16** (everywhere) and **24** (launcher,
   `CkDebuggerLauncherStyle.cpp:124`). Register 16/24, not 16/20.
4. **"`GetBrush` logs and returns the default brush"** — confirmed in this engine
   (`Engine/Source/Runtime/SlateCore/Private/Styling/SlateStyleSet.cpp:299-330`: warning +
   `GetDefaultBrush()`). But the house accessors use `GetOptionalBrush` → nullptr
   (`SlateStyleSet.cpp:332`), and null-tolerant widgets make failure **fully silent**. The typed-id
   motivation stands — arguably stronger than the constraint stated.
5. **`@mdi/svg` (npm)** — no node/npm usage exists anywhere in the ecosystem. If MDI is chosen,
   vendor from a pinned GitHub release download, not npm.
6. **NonUFS staging** — applies (debugger modules package into non-editor builds) and is already
   solved by precedent; a new CkFoundation icon folder needs the same one `foreach` in its host
   module's Build.cs.
7. **SVG rasterizer subset** — not independently re-verified against engine source; CONFIRMED
   operationally: 206 stroke/`<g>`/`<circle>`-based icons render in-editor today, and none use
   `<style>` blocks. (INFERRED: the "no `<style>`/CSS" limit stands; nothing in the corpus tests it.)

---

## 4. Design options

Facts shared by all options: generator = **PowerShell** in `Source/CkScripts/` (house tooling
language; no UBT-hook precedent → manual run, committed output, write-if-changed determinism
mirroring the AS generator); generated C++ = banner-marked files in the host module; style set =
new set in `CkEditorTools`, symmetric register/unregister in the module's Startup/Shutdown
(follow `CkDebuggerStyle.cpp:34/:45`, no `IsRunningGame` guard — this set must live in packaged
builds, like the debugger sets and unlike `CkEditorStyle`); accessor = static-class
`Get_Brush(ECk_Icon, size)` in the `FCkDebuggerStyle` idiom (a C++-only Slate surface, not a BPFL —
matching the consumer's precedent); per-size keys registered at 16 and 24.

### Option 1 — As specified: vendor an MDI subset + manifest + generator

`Resources/Icons/Mdi/` + upstream LICENSE + pinned-version stamp + Apache-2.0 §4(b) modification
notice. Generator validates manifest names against the pinned MDI `meta.json`, writes
`fill="#FFFFFF"` onto paths, emits enum + key table + registration.

- **For**: 7000+ glyph catalog — new concepts never need authoring; pinned upstream; exactly what
  the campaign text says.
- **Against**: second visual language beside 206 house icons (Phase B then forces either a mass
  visual replacement — including 148 decorative pool glyphs like `Axe`/`Backpack` with no clean MDI
  equivalents — or a permanent hybrid); licensing surface; the recolour machinery exists only to
  serve MDI; diverges from the Starship stroke idiom.

### Option 2 — Same pipeline, first-party corpus as the vendored source **(RECOMMENDED)**

Everything Phase A specifies — manifest (semantic id → SVG file), deterministic loud-failing
generator, generated typed enum + key table + registration, accessor — with the **existing
house-style corpus as the asset source**. Phase A seeds a canonical `CkFoundation` icon folder with
10-15 glyphs copied from the debugger corpus; Phase B canonicalizes the rest and deletes the
directory-scan mechanism. Generator validation: manifest entry ↔ file exists (loud failure on
typo), 16×16 viewBox, white-ink lint, no `<style>`/CSS/filters.

- **For**: zero licensing; preserves the reviewed visual language and its Starship consistency;
  Phase B is mostly mechanical (same pictures, typed names); the pipeline deliverables are
  identical to Option 1's.
- **Against**: a genuinely new concept needs an authored glyph (house style is a ~10-line
  hand-writable SVG) instead of a catalog pick. An MDI *import* path (convert + recolour one glyph
  into the corpus, provenance recorded in the manifest) can be added later without changing the
  pipeline.

### Option 3 — Minimal: no manifest; generator scans the vendored folder

Enum generated from filenames; a missing file is impossible by construction.

- **For**: simplest; folder is the single source of truth.
- **Against**: violates the campaign's semantic-name requirement (call sites name the file, i.e.
  the picture — though house filenames are already largely semantic: `Severity_Error`,
  `Attribute`); no seam to record variant fallbacks or provenance; file renames hit call sites
  directly with no mapping layer to absorb them.

---

## 5. Recommendation

**Option 2.** The campaign's actual goal — typed, compile-checked identifiers replacing
hand-written string keys, with a deterministic loud-failing generator — is achieved identically
under any asset source. The asset-source choice is separable, and the repo's own history (a
deliberate, campaign-reviewed, spec-tested, 206-icon first-party corpus in the exact consumer,
matching the engine's own icon idiom) argues against importing a second visual language plus a
licensing surface to solve a problem this codebase does not have. If the MDI catalog is wanted
anyway, Option 1 is buildable as specced — with sizes corrected to 16/24, npm avoided, and the
Phase B visual-replacement cost accepted explicitly.

## 6. Decisions needed at this gate

1. **Asset source**: house corpus (Opt 2) / MDI (Opt 1) / folder-scan minimal (Opt 3)?
2. **Host module**: `CkEditorTools` (recommended) vs a new module?
3. **Phase A seed**: copies of existing debugger glyphs (recommended — proves Phase B's path) vs
   freshly authored ones?
4. **Manifest format**: JSON (recommended — `.ckexport`/`GauntletTests.json` precedent) vs psd1/ini?
5. **Generator language**: PowerShell (recommended) vs Python? Both have CkScripts precedent.
