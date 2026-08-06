# CkGameSettings — design & implementation plan

**Date:** 2026-08-05
**Status:** GREEN-LIT 2026-08-05 — CTO review verdict GREEN-LIGHT WITH NON-BLOCKING NOTES (`../reviews/2026-08-05-CkGameSettings-CTO-review.md`); all 11 review notes folded into this doc (rev 2). Ready for Phase 0.
**Author session:** Fable 5 (research + design); implementation intended for an Opus executor with per-phase Fable audit.

---

## 1. Context — why this module

### 1.1 The AutoSettings inheritance (BusterBlock 5.5)

BB 5.5 shipped the marketplace **AutoSettings** plugin (Sam Bonifacio, vendored `1.31.0-UE5.6` with local patches). Source audit of `D:\Repositories\CkRepos\BusterBlock_5.5\Plugins\AutoSettings\Source` found:

- **What it is:** a CVar↔ini↔UMG binding layer. A "setting" is *literally a console variable* — no schema, no registry, `FString` end to end. Widgets (`UAutoSettingWidget` subclasses) carry the CVar name, the dirty state, and the apply/save/cancel logic; there is **no headless setting model**.
- **What BB actually used:** 31 setting rows (all `bAutoSave=true`, no Apply/Cancel UX), the resolution/window-mode value-mask split over `r.SetRes`, boot-time config→CVar replay, and one C++ hook (`bb.ScalabilityPreset` → `UGameUserSettings`). BB declared its 27 `bb.*` gameplay CVars itself via `TAutoConsoleVariable`; audio volume was hand-wired (`SettingsListener_BB_AC` → `SetSoundMixClassOverride`) because the plugin ships no audio surface.
- **What shipped dead:** the entire `AutoSettingsInput` module (~55% of the plugin) — legacy-input only, zero Enhanced Input support. BB rewrote keybinding against CkFoundation's `UCk_Utils_KeyBinding_UE` and left the plugin's version ticking every frame.
- **Faults to design against:** settings unenumerable without instantiating UI; console-variable sinks registered and never unregistered (leak per menu open); saved settings whose CVar registers after `OnPostEngineInit` **silently dropped**; `GConfig->Flush` to disk per change; two `AddToRoot`'d singletons; strictly global (per-player impossible by construction).
- **Worth keeping:** application-**order-preserving** ini writes (scalability CVars cascade; replay order matters), the three-state apply model (auto-save / live-preview / explicit apply with real revert), bidirectional widget↔CVar sync, value masks.

### 1.2 Market position (surveyed 2026-08-05)

- The engine absorbed the data layers: `UEnhancedInputUserSettings` (rebind persistence, per-LocalPlayer, profiles), `UGameUserSettings` (video/scalability), CommonUI (gamepad-ready navigation). AutoSettings 2.0 (July 2026) rebuilt exclusively on Enhanced Input — a compatibility-breaking rework.
- Declaration center of gravity is **data-driven** (data assets / data tables): AutoSettings 2.x, Game Settings Kit, SettingsWidgetConstructor.
- **Lyra's `UGameSettingRegistry` is the architectural high-water mark**: headless typed setting objects, edit conditions with player-facing reasons (`FWhenPlatformHasTrait`, `FWhenPlayingAsPrimaryPlayer`), the **Local/Shared scope split** (`ULyraSettingsLocal` = machine ini vs `ULyraSettingsShared` = per-player SaveGame), and type→row-widget visual mapping. As a product it is not reusable: C++-only authoring, stringly reflection-path data sources, multi-plugin extraction pain.
- 2026 table-stakes: pending/apply/revert with resolution confirm-countdown, hardware benchmark defaults, boot re-apply, EI-native rebinding, gamepad navigation. Differentiators: per-player generic settings, platform-conditional visibility, registry-driven widget generation.

### 1.3 What CkFoundation already has (reuse, do NOT duplicate)

| Surface | Location | Status |
|---|---|---|
| Key rebinding, complete (remap, conflicts, swap, reset, `SaveKeyBindings` → EI async save) | `Source/CkInput/Public/CkInput/CkKeyBinding_Utils.h` + `Subsystem/CkKeyBinding_Subsystem.h` | **Done — settings module only builds the page widget** |
| IMC scan paths (all mappable keys registered up front) | `CkInput/Settings/CkInput_Settings.h` | Done |
| Key glyphs | `CkInput/CkKeyIcon_Utils.h` | Done; promises `UCk_InputActionWidget_UE` which **does not exist** (deliverable here) |
| Typed CVar register/get/set/bind + change callbacks + definition registry | `Source/CkCVar` | Done for C++/BP. **AS-blocked** (`INTERNAL_*` + `BlueprintInternalUseOnly` get no AS wrapper) — separate follow-up, not blocking |
| CommonUI layer stack, `PushWidgetToLayer` + `TAG_UI_Layer_Menu`, `UCk_ActivatableWidget_UE`/`UCk_UserWidget_UE`, `CkCommonButton*`/text styles/TabBar | `Source/CkUI` | Done — the substrate the settings screen plugs into |
| Accelerant-widget doctrine (`CodeBuilt`/`Custom` presentation, zero uassets, design-time preview = runtime math) | `Source/CkCompass/Public/CkCompass/UI/`, `Source/CkMinimap/Public/CkMinimap/UI/` | The pattern to copy |
| Subsystem-shaped module precedent (feature folders, no ECS quartet, settings+CVar layering, fail-open watchdog) | `Source/CkLoadingScreen` | The structural template |

### 1.4 The genuine gaps (what this module builds)

1. **Runtime player-settings persistence** — the existing `UCk_Plugin_UserSettings_UE` is `Config = EditorPerProjectUserSettings` (editor-only ini; does not ship). Confirmed at `Source/CkSettings/Public/CkSettings/UserSettings/CkUserSettings.h:9`.
2. **Headless settings registry/schema** — nothing like it exists.
3. **Audio volume surface** — CkAudio has per-track volume only; no SoundMix/category-volume anywhere.
4. **Video/quality surface** — no `UGameUserSettings` interaction anywhere in the plugin.
5. **Settings-menu widget set** (including the promised `UCk_InputActionWidget_UE`).

BusterBlock is the **first consumer and reference vocabulary** (its 27 CVars, SoundMix wiring, scalability preset) — not the spec. The module is designed for future Ck projects.

---

## 2. Goals / non-goals

**Goals**
- G1. A setting is **data**: declared once (C++/BP/AS or data asset), enumerable, resettable, testable with zero UI.
- G2. Machine-vs-Player **scope split** first-class (Lyra's one great idea), with pluggable storage.
- G3. Loud failure: no silently dropped settings, no swallowed apply errors (non-negotiable #3).
- G4. Opt-in **accelerant packs** (Video, Audio) and **accelerant widgets** (registry-generated screen, keybinding page) — nice defaults, replaceable wholesale.
- G5. Full tri-environment API (C++ / Blueprint / AngelScript).

**Non-goals (v1)**
- Replication/authority (settings are local-machine — matches every surveyed product).
- Per-player *generic* settings beyond player 0 (contract supports it; not exercised; input is already per-player via EI).
- Upscaler toggles (DLSS/FSR/XeSS) — game-side pack material.
- Cloud storage implementation (the provider seam supports it; no impl shipped).
- Fixing CkCVar's AS gap (separate small follow-up).

---

## 3. Architecture

Four layers. Subsystem-shaped (CkLoadingScreen structure) — **no ECS quartet, no `FCk_Handle_GameSettings`**. The request-completion contract's subsystem carve-out applies: API is synchronous, no fabricated owner handles.

### 3.1 Layer 1 — setting model & registry

```
FCk_GameSettings_SettingDefinition (USTRUCT, BlueprintType)
  _Key                FName            // hierarchical, e.g. "audio.volume.master"
  _ValueType          ECk_GameSettings_ValueType { Bool, Int32, Float, String }
                                       // module-owned enum (review note 4): ECk_CVarType carries a fifth
                                       // member, Command (CkCVar_Data.h:24), nonsensical as a setting value;
                                       // owning the enum removes the invalid state and decouples the schema
                                       // spine from CkCVar (still a dep for FCk_CVarRef). One switch at the
                                       // CVar apply seam converts.
  _DefaultValue       FString          // typed accessors validate on read
  _Range              (optional)       // enum-mode + FFloatRange (house optionality idiom)
  _Options            TArray<FCk_GameSettings_SettingOption>  // FText Label + FString Value
  _DisplayName        FText
  _Description        FText
  _CategoryTags       FGameplayTagContainer
  _Scope              ECk_GameSettings_Scope { Machine, Player }
  _PersistencePolicy  ECk_GameSettings_PersistencePolicy { Provider, External }
                                       // review note 2: External = value lives in an external store the
                                       // engine already owns (e.g. UGameUserSettings for the Video pack);
                                       // read-through/write-through the typed handler, NEVER stored in the
                                       // provider — prevents a second source of truth that drifts on
                                       // benchmark runs or manual GameUserSettings.ini edits. Provider =
                                       // the normal case (audio, gameplay, accessibility).
  _ApplyBinding       CVar (FCk_CVarRef) OR typed handler (registered separately) OR None (pure stored value)
  _EditConditions     platform-trait tags + optional bound predicate + FText _DisabledReason
```

- **Registry owner:** `UCk_GameSettings_Subsystem_UE : UGameInstanceSubsystem`.
- **Registration:** `Request_RegisterSetting(s)` from C++/BP/AS; plus `UCk_GameSettings_Collection_PDA` data assets listed in project settings (scan-list precedent: `CkInput`'s `_MappingContextScanPaths`), auto-registered at subsystem init.
- **Access:** typed `Get_SettingValue_{Bool,Float,Int32,String}`, `Request_SetSettingValue` (takes a request struct — extension-point rule holds even in the subsystem carve-out), `Request_ResetToDefault`, `Request_ResetAll`, `Get_AllSettings`, `Get_SettingsByCategory`.
- **Change notification:** `BindTo_OnSettingChanged` / `UnbindFrom_OnSettingChanged` (per-key and wildcard), dynamic delegates (CkLoadingScreen's `BindTo_OnVisibilityChanged` precedent). Processors that care about a CVar-bound setting bind CVar callbacks natively — no ECS bridge needed.
- Registration is atomic per collection (one rejected required definition invalidates the collection — ensure + terminate, per non-negotiable #3).

**Explicitly rejected:** Lyra's reflection string paths (`GET_..._FUNCTION_PATH`) — stringly, AS-hostile, silent-failure-prone. Apply bindings are a `FCk_CVarRef` or a typed registered handler, nothing else.

### 3.2 Layer 2 — persistence & apply

- **Storage provider seam:** `UCk_GameSettings_StorageProvider_UE` — `UCLASS(Abstract)` UObject base with `BlueprintNativeEvent`s (review note 5), NOT a raw C++ interface. Rationale: the seam is cold-path (boot load + batched flush), and AS-first consumers are exactly where a custom provider ("route Player scope into my save system / SPUD / EOS cloud") would live — one UObject-shaped seam serves all three environments (non-negotiable #4) with no dual model. Narrow surface: load section, store value, flush.
- **Default provider (shipped):** ordered key-value store → `Saved/Config/<Platform>/CkGameSettings.ini`, **owning its serialization outright** (review note 7): hand-written ordered ini read/write (or at minimum a standalone `FConfigFile`, which constructs with `bCanSaveAllSections = true`, `ConfigCacheIni.cpp:1292`) — **never registered with GConfig, which the provider touches zero times**. This sidesteps rather than works around the confirmed 5.7.4 restriction (`ConfigContext.cpp:621-624` grants `bCanSaveAllSections` only to `"User"`-named/editor files; `FConfigFile::Write` otherwise consults a `[SectionsToSave]` allow-list), and guarantees **application order** is preserved (cascading scalability CVars replay correctly — `FConfigSection` is a `TMultiMap` with no ordering contract anyway). Machine scope in `[Machine]`; Player scope in `[Player.<PlatformUserId>]` sections keyed by **`FPlatformUserId`** (review note 8 — survives split-screen join/leave reordering, maps onto cloud identities; v1 exercises player 0 only).
- **Flush discipline:** dirty-mark + batched flush (timer + menu-close + app-deactivate + shutdown) — **not** per-change disk hits.
- **Boot apply:** at subsystem `Initialize`, replay stored `Provider`-policy values onto apply bindings. Timing fact (review note 1, confirmed against 5.7.4 engine source): in a packaged game `UGameEngine::Init` creates the GameInstance and runs subsystem `Initialize` (`GameEngine.cpp:1249-1251`, `GameInstance.cpp:128`) **before** `FCoreDelegates::OnPostEngineInit` broadcasts (`LaunchEngineLoop.cpp:3999→4007`) — i.e. *earlier* than AutoSettings' hook, not later. No engine-subsystem split is needed. Anything genuinely earlier (RHI init, device profiles, early `r.*`) is unreachable by any user-settings system and is owned by `GameUserSettings.ini`/`DefaultEngine.ini`, which the engine replays inside `UGameEngine::Init` (`GameEngine.cpp:1226-1235`) before the subsystem exists.
- **The two distinct "late" cases** (review note 3 — do not conflate; conflating them reintroduces AutoSettings' data-loss fault in loud form):
  - **(a) Registered setting whose bound CVar is missing** → deferred-apply queue, retried on a cadence; on timeout dropped **loudly** (`CK_ENSURE_IF_NOT` naming the key), mirroring the persistence-dispatcher NotReady pattern. The queue is *structural*, not defensive: `PostEngineInit`/`PostDefault`-phase modules' CVars cannot exist yet at replay time.
  - **(b) Stored value whose setting definition hasn't been registered yet** → retained indefinitely and applied at `Request_RegisterSetting` time, with **no** warning and **no** timeout — BP/AS registrations from game code after map load are the normal flow, not an error.
- **Listener hygiene:** every CVar sink/binding tracked and unbound on subsystem `Deinitialize` (the AutoSettings leak, inverted).
- **Apply model:** immediate-apply per setting by default; a subsystem-held pending-changes session — `Request_BeginPendingChanges` → stage → `Request_ApplyPendingChanges` / `Request_RevertPendingChanges` (house verb vocabulary, review note 9a) — for panels that want explicit Apply/Cancel with live preview. Single-session semantics: a second `Begin` while one is active ensures + rejects; an open session at `Deinitialize`/travel **auto-reverts** (safe default). Subsystem-held (not per-screen) so it survives widget destruction and is headless-testable.
- **PIE / multi-instance rule** (review note 6): each PIE client creates its own GameInstance → own subsystem → own boot replay onto **global process CVar state** shared with the editor and every other client; all instances share one `CkGameSettings.ini`. Provisional rule (Phase 1 decision, revisit there): **skip CVar-bound replay under PIE entirely** — the store remains the source of truth and `Get_SettingValue` stays correct; the editor's CVar state is never contaminated. `ShouldCreateSubsystem` returns false on dedicated servers. Registry + store bookkeeping stay alive in headless/null-RHI runs (CkLoadingScreen's bookkeeping-vs-presentation split) so AutoTests can run.
- Raw `GConfig` is used **nowhere** in this module (stronger than the CkSettings anti-pattern rule requires — see the default-provider bullet).

### 3.3 Layer 3 — built-in packs (opt-in accelerants)

- **Video pack:** `UGameUserSettings` bridge — resolution + window mode (confirm-with-countdown revert), VSync, FPS cap, `sg.*` scalability groups, overall-quality preset, hardware benchmark defaults on first run. All Video-pack definitions are **`_PersistencePolicy = External`**: values read-through/write-through typed handlers into `UGameUserSettings`, never stored in the provider (review note 2 — the engine loads and applies `GameUserSettings.ini` before the subsystem exists; a second copy would drift). The pack routes **exclusively through `GEngine->GetGameUserSettings()`** (class resolved from the project's `GameUserSettingsClassName` ini) and never assumes the concrete class (review note 11) — so no "bring your own subclass" hook is needed; a project's subclass extras (BB's `bb.ScalabilityPreset` successor) compose as game-registered settings with typed handlers.
- **Audio pack:** category volumes (Master/Music/SFX/Voice defaults) — introduces the missing driver: project-settings map of category tag → `USoundClass`, applied via `SetSoundMixClassOverride` (formalizes BB 5.5's hand-rolled `SettingsListener_BB_AC`).
- Packs register through the same public registration API as game settings; enabled per pack via `UCk_GameSettings_ProjectSettings_UE`. A game takes none, some, or all.

### 3.4 Layer 4 — widget accelerant (Compass/Minimap doctrine)

`Public/CkGameSettings/UI/`. Widget naming follows the **Compass axis on both file and class names** (review note 9b — the two precedents disagree; picking one and holding it): file `Ck<Module>UI_<Thing>Widget.h` → class `UCk_<Module>UI_<Thing>Widget`, no `_UE` suffix on feature widgets.

- `UCk_GameSettingsUI_ScreenWidget` (`CkGameSettingsUI_ScreenWidget.h`) — derives `UCk_ActivatableWidget_UE`; pushed via `UCk_Utils_UI_Layout_UE::PushWidgetToLayer(PC, TAG_UI_Layer_Menu, ...)`. Presentation enum:
  - `CodeBuilt` — **zero-asset default**: rows generated from the registry (category tabs from `_CategoryTags`, row type inferred from `_ValueType`/`_Options`), styled on the `CkCommon*` set.
  - `Custom` — the WBP owns the tree; `BindWidgetOptional` slots; the widget still owns bind/unbind, pending-changes session, and row generation into a provided container.
- Row widgets: `UCk_GameSettingsUI_RowWidget_Toggle` / `_Slider` / `_Select` (derive `UCk_UserWidget_UE`), with a Lyra-style **type→row-class mapping** struct in project settings so a game swaps row visuals without touching logic.
- Keybinding page: `UCk_GameSettingsUI_KeyBindingPageWidget` + capture prompt + conflict dialog, consuming `UCk_Utils_KeyBinding_UE` end-to-end (nothing new in the input layer — its surface already covers query/remap/conflicts/swap/reset/persist, verified in review).
- `UCk_InputActionWidget_UE` (`UCommonActionWidget` subclass, auto-refresh on device/remap) — delivered into **CkUI**, where `CkKeyIcon_Utils.h:31` promises it. Keeps the `_UE` suffix despite feature widgets dropping it (review note 9c): the name is promised verbatim, and CkUI's own widget bases carry the suffix.
- Design-time preview renders fake rows through the same generation path as runtime; misconfigs ensure at runtime and warn at design time (`DoReportMisconfig` pattern).
- **Zero uassets shipped.** Widget-only deps annotated in Build.cs.

---

## 4. Module facts

- **Name:** `CkGameSettings` (locked). Runtime, `LoadingPhase: Default`, Win64/Mac/Linux allowlist.
- **Tier:** T4. Deps: `CkCore, CkCVar, CkInput, CkLog, CkSettings, CkUI` (+ engine `UMG, Slate, SlateCore, CommonUI, EnhancedInput, DeveloperSettings`). `CkUI`/UMG **and `CkInput`** deps annotated *"settings widgets only"* (Compass precedent; review A3 — the core registry never touches CkInput, only the keybinding page does). No CkEcs dep unless something forces it (CkLoadingScreen ships without one).
- **Structure** (CkLoadingScreen-shaped):

```
Source/CkGameSettings/
  CkGameSettings.Build.cs, Claude.md
  CkGameSettings_Log.{h,cpp}, CkGameSettings_Module.{h,cpp}
  Public/CkGameSettings/
    CkGameSettings_Common.h                          // enums, FCk_GameSettings_SettingDefinition, option/request structs
    Subsystem/CkGameSettings_Subsystem.{h,cpp}       // registry + apply + pending-changes session
    Subsystem/CkGameSettings_Utils.{h,cpp}           // UCk_Utils_GameSettings_UE (BPFL mirror; the AS surface)
    Storage/CkGameSettings_StorageProvider.{h,cpp}   // UCk_GameSettings_StorageProvider_UE (Abstract UObject base) + default ordered-ini provider
    Collection/CkGameSettings_Collection.{h,cpp}     // UCk_GameSettings_Collection_PDA
    Packs/CkGameSettings_VideoPack.{h,cpp}
    Packs/CkGameSettings_AudioPack.{h,cpp}
    Settings/CkGameSettings_Settings.{h,cpp}         // UCk_GameSettings_ProjectSettings_UE + static accessor class
    UI/CkGameSettingsUI_ScreenWidget.{h,cpp}
    UI/CkGameSettingsUI_RowWidgets.{h,cpp}
    UI/CkGameSettingsUI_KeyBindingPageWidget.{h,cpp}
  (+ CkUI: CustomWidgets/InputAction/CkInputAction_Widget.{h,cpp})
```

- **AS exposure:** all public API as plain UFUNCTIONs on `UCk_Utils_GameSettings_UE` (never `BlueprintInternalUseOnly`), `_UE` suffix, no UFUNCTION overloads (house suffixes), delegates last, `AutoCreateRefTerm` where a delegate is optional. Generated namespace: `utils_game_settings`.
- **Docs:** module `Claude.md` + rows in `Source/CLAUDE.md` tier table and "I need to…" lookup.

---

## 5. Phases & gates

| Phase | Deliverable | Gate |
|---|---|---|
| **0 — Core** | Scaffold + definition/registry + typed get/set + change delegates + collection PDA | AutoTests: register/duplicate-reject/atomic-collection, typed access, invalid-input rejection (per ensure-boundary test rule), change-notify |
| **1 — Persistence & apply** | ini provider (order-preserving, batched flush), boot replay, deferred-apply queue, pending-changes session, provider seam | AutoTests: round-trip, replay order, late-CVar retry + loud timeout, reset-all, revert semantics; provider-swap fake |
| **2 — Packs** | Audio pack (SoundMix driver), Video pack (`UGameUserSettings` bridge via `GEngine->GetGameUserSettings()`, External policy, benchmark, confirm-countdown) | AutoTests: audio **asserted at the seam** (handler invoked with the right `USoundClass`/volume — verify first whether the toolbox harness has a live audio device; review note 10), video value mapping; `[EDITOR-VERIFY]`: audibility, resolution switch, benchmark, countdown UX |
| **3 — Widgets** | Screen (CodeBuilt/Custom) + rows + type→row mapping + keybinding page + `UCk_InputActionWidget_UE` | Gym level + `[EDITOR-VERIFY]` visual/nav pass (gamepad focus traversal); AutoTests for row-generation logic where headless |
| **4 — Close-out** | Claude.md, Source/CLAUDE.md rows, AS wrapper verification (all three environments), BB adoption example (migrate a slice of the 27 `bb.*` settings) | Full toolbox gate vs recorded baseline; comment audit |

Each phase lands as its own gate; implementation routes to an Opus executor with Fable audit per gate (per `meta-triage` / `ck-methodology`).

---

## 6. Risks & open questions

1. ~~**Boot timing**~~ — **RESOLVED by CTO review (note 1, engine-source-confirmed):** GameInstance-subsystem `Initialize` runs **before** `OnPostEngineInit` in packaged games — *earlier* than AutoSettings' hook. No engine-subsystem split. The deferred-apply queue remains structural (later-phase modules' CVars can't exist at replay time).
2. ~~**Custom-ini save quirk**~~ — **DELETED by design change (review note 7):** the default provider owns its serialization (ordered read/write, standalone `FConfigFile` at most) and never registers with GConfig, so the confirmed 5.7.4 restriction (`ConfigContext.cpp:621-624`) is never in play.
3. **PIE / multi-instance CVar contamination** (review note 6) — multiple PIE GameInstances each replaying onto shared process CVar state, and sharing one ini for flushes. Provisional rule in §3.2 (skip CVar-bound replay under PIE); **confirm or replace in Phase 1**.
4. **Video pack coupling** — interplay with whatever the current BB does for scalability (`bb.ScalabilityPreset` equivalent); most externally-coupled phase. Mitigated by External policy + `GEngine->GetGameUserSettings()`-only routing.
5. **Widget layer is the judgment-heavy part** — CommonUI focus/navigation polish, design-time preview parity. Sized as its own phase for that reason.
6. **Player-scope contract shipped but under-exercised** (player 0 only in v1) — keyed by `FPlatformUserId` in the schema now (review note 8) even though split-screen isn't tested.

---

## 7. Decisions locked during design (with rationale)

1. **Name `CkGameSettings`** (user-approved; avoids collision with editor-scoped `UCk_Plugin_UserSettings_UE`).
2. **Headless registry; settings are data, not widgets** — the direct inversion of AutoSettings' worst flaw.
3. **Rebinding stays on CkInput / `UEnhancedInputUserSettings`** — no new keybind model or persistence; settings module builds only the page UI.
4. **Scope `Machine`/`Player` first-class + pluggable storage provider with ini default** — Lyra's Local/Shared idea without its implementation; cloud/SPUD routing is a provider, not a fork.
5. **Typed apply bindings only** (CVar ref or registered handler) — Lyra's reflection string paths explicitly rejected.
6. **Both Video and Audio packs in v1, opt-in** — future-facing table-stakes.
7. **Widgets follow the Compass/Minimap accelerant doctrine** — `CodeBuilt`/`Custom`, zero uassets, CkUI/CommonUI substrate, registry-generated rows.
8. **Subsystem-shaped module** (CkLoadingScreen precedent) — no ECS quartet, no typesafe handle, synchronous API under the request-contract carve-out (but `Request_Set*` still takes request structs for extension safety).
9. **Replication out of scope** — settings are local-machine.
10. **Edit conditions in the schema from day one** (platform-trait tags + predicate + player-facing disabled reason) — cheap now, painful to retrofit.

### 7.1 Added by CTO review, 2026-08-05 (all accepted; review file has the full rationale)

11. **Module-owned `ECk_GameSettings_ValueType`** — `ECk_CVarType` rejected for its `Command` member (note 4).
12. **`_PersistencePolicy { Provider, External }` per definition** — Video pack is External (read/write-through `UGameUserSettings`, never stored in the provider) (note 2).
13. **The two "late" cases are distinct:** missing-CVar → deferred queue + loud timeout; stored-value-without-definition → retained silently, applied at registration, never a warning (note 3).
14. **Storage provider is a `UCLASS(Abstract)` UObject base** (BlueprintNativeEvents) so AS/BP can implement one; no C++-only interface (note 5).
15. **Default provider owns its serialization; zero GConfig anywhere in the module** (note 7).
16. **Player sections keyed by `FPlatformUserId`** (note 8).
17. **Pending-changes verbs:** `Request_BeginPendingChanges` / `Request_ApplyPendingChanges` / `Request_RevertPendingChanges`; single session, auto-revert on `Deinitialize`/travel (note 9a).
18. **Widget naming: Compass axis** on files and classes (`UCk_GameSettingsUI_<Thing>Widget`); `UCk_InputActionWidget_UE` keeps its promised name (notes 9b/9c).
19. **PIE provisional rule:** skip CVar-bound replay under PIE; `ShouldCreateSubsystem` false on dedicated servers; bookkeeping alive headless (note 6 — confirm in Phase 1).
20. **Video pack routes exclusively via `GEngine->GetGameUserSettings()`**; no bring-your-own-subclass hook (note 11).
