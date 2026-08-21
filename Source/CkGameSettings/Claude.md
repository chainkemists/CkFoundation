# CkGameSettings

**Purpose:** user-facing game settings as DATA — a headless registry (declare → typed get/set →
change notification → persistence) with opt-in Audio/Video packs and an optional widget layer.
The AutoSettings replacement: settings exist and are testable with zero UI; widgets are styling
over the same registry. Subsystem-shaped module (CkLoadingScreen precedent) — no ECS quartet.

**Depends on:** `CkCore`, `CkCVar`, `CkLog`, `CkSettings` (core) + `UMG/Slate/CommonUI/
EnhancedInput/InputCore/CkEcs/CkUI/CkInput` (widget layer ONLY — the core registry never includes
from these).
**Used by:** games directly; nothing else in CkFoundation depends on it.

---

## Key API (all mirrored on `UCk_Utils_GameSettings_UE` with a WorldContext first param; AS: `utils_game_settings::`)

### Declare
- `FCk_GameSettings_SettingDefinition{Key, ValueType, DefaultValue}` + fluent optionals:
  `Set_Scope` (Machine/Player), `Set_PersistencePolicy` (Provider = stored in the provider;
  External = read/write-through registered accessors, NEVER stored), `Set_ApplyBindingType`
  (None/CVar/Handler) + `Set_CVar`, `Set_DisplayName/Description/CategoryTags`,
  `Set_MinValue/MaxValue` (strings; empty = unbounded; numeric only), `Set_Options`
  (value must match one), `Set_EditConditions` (platform-trait tags + player-facing
  `_DisabledReason`).
- Numeric PRESENTATION knobs (row rendering only, never the stored value):
  `Set_OptionalDisplayPrecision` (max decimals — trailing zeros are always trimmed, so `90.00`
  reads `90` and `1.50` reads `1.5`; negative = by type), `Set_OptionalDisplayScale` (readout
  multiplier — a 0..1 volume shown as 0..100 sets 100), `Set_OptionalStepSize` (snap granularity
  for both readout and commit; non-positive = Int32 1 / Float a hundredth of the range).
- `Request_RegisterSetting(s)` — batch registration is ATOMIC: one invalid definition rejects the
  whole batch (ensure + nothing registered). `Request_RegisterCollection` /
  `UCk_GameSettings_Collection_PDA` assets listed in project-settings scan paths auto-register at
  subsystem init.

### Access
- Typed `Get_SettingValue_{Bool,Int32,Float,String}` (ensure + fallback on unknown key or type
  mismatch), `Get_SettingDefinition`, `Get_AllSettingKeys` (registration order),
  `Get_SettingKeysByCategory` (empty query = everything), `Get_IsSettingRegistered`.
- `Request_SetSettingValue_*` (takes the request struct) — validates (registered, type, range,
  options), stores, applies, fires the change delegate iff the value changed. Idempotent same-value
  set returns true without firing. `Request_ResetToDefault` (rejects External keys — their schema
  defaults are placeholders), `Request_ResetAllToDefaults` (skips External).
- `BindTo_OnSettingChanged(Key, Delegate)` / `UnbindFrom_...` — per-key; `Key == None` = wildcard.

### Apply routing
- **CVar-bound:** registered CVar → `INTERNAL_Set_*`; unregistered CVar → deferred queue retried at
  1 Hz, LOUD ensure after `ck.GameSettings.DeferredApplyTimeoutSecs` (value retained, never
  deleted). In PIE only the terminal CVar WRITE is skipped.
  **Registration REJECTS a CVar binding onto a console variable `UGameUserSettings` owns** — the
  twelve `sg.*` scalability groups, `r.VSync`, `t.MaxFPS` (`ck::game_settings::Get_IsCVarOwnedByGameUserSettings`).
  `UGameUserSettings` mirrors those in its own fields and rewrites them from that copy on every
  `ApplyNonResolutionSettings`, which the ENGINE triggers on an F11 fullscreen toggle and on any
  resolution change — so the setting would revert at a moment the player cannot connect to it, and
  its stored value would still read as the one they chose. This is exactly the split-brain the
  AutoSettings plugin worked around by demanding a `UGameUserSettings` subclass with an emptied
  `ApplyNonResolutionSettings`; the Video pack's External policy makes `UGameUserSettings` the one
  store instead, and this guard keeps a new setting from re-opening the hole. Declare such a value
  External over the matching `UGameUserSettings` accessor.
- **Handler-bound:** `Request_RegisterApplyHandler_*` — registering a handler for a stored value
  applies the CURRENT value immediately.
- **External:** `Request_RegisterExternalAccessors_*` (getter + setter pair) — reads route through
  the getter, writes through the setter; nothing lands in the provider.
- Stored value with NO definition yet = a silent orphan, applied when its definition registers.

### Persistence
- Storage provider: `UCk_GameSettings_StorageProvider_UE` (Abstract, BlueprintNativeEvents — AS/BP
  implementable). Default: ordered hand-serialized ini at
  `Saved/Config/<Platform>/CkGameSettings.ini` ([Machine]/[Player.<Id>] sections, unknown lines
  preserved, atomic flush). Flush is debounced (2s), plus engine-pre-exit /
  app-deactivate / `Request_FlushStorage`. `Request_ReloadFromStorage` re-runs the boot merge.

### Sessions + packs
- Pending-changes session: `Request_BeginPendingChanges` → sets apply LIVE but record priors →
  `Request_ApplyPendingChanges` (commit + flush) / `Request_RevertPendingChanges` (re-apply
  priors). Single session at a time.
- `Request_RegisterAudioPack` (config: SoundMix + per-category SoundClass/key/default) — one Float
  volume setting per category driving `SetSoundMixClassOverride`, stamped 0..100 percent for
  display. `Request_RegisterVideoPack` — 12 External `video.*` keys over
  `GEngine->GetGameUserSettings()` (window mode, resolution, vsync, fps cap, quality preset, the
  classic seven `sg.*` groups). Both idempotent; auto-run at init when the project-settings
  toggles are on.
- **A game may own a video key's presentation**: register `video.<key>` yourself (label, options,
  category, ordering) BEFORE calling `Request_RegisterVideoPack`, and the pack contributes only
  its accessor bridge. Such a definition must carry the pack's contract — `PersistencePolicy::External`
  + `ApplyBindingType::Handler` — or the pack ensures and leaves that key unbound. Doing this
  means turning the init-time toggle OFF and calling the pack explicitly, so the game registers
  first (BusterBlock's `BB_GameSettings_Video.as` is the reference adopter).
- `Request_RunHardwareBenchmark` (headless no-op), `Request_SetResolutionWithConfirmWindow` +
  `Request_ConfirmResolution` (expiry reverts; nothing saved until confirm/revert).

### Widget layer (`UI/`, optional)
The native widgets own PLUMBING only — the WBP owns every tree (no hidden code-built fallbacks,
2026-08-06 maintainer directive; same doctrine applied to the Compass ribbon + Minimap frame).
Value CONTROLS and row containers are required `BindWidget`s (missing/mistyped = WBP compile
error); labels, readouts and chrome are `BindWidgetOptional`. Enforcement is WBP-compile-time
only — headless native instantiation (tests, gym) runs with null slots, so every slot stays
null-guarded in C++:
- Rows: `UCk_GameSettingsUI_RowWidget_{Toggle,Slider,Select,Dropdown}` — subclass in a WBP, bind
  `_DisplayNameText` + the control slots (`_ValueCheckBox` / `_ValueSlider`+`_ValueText` /
  `_PrevButton`+`_NextButton`+`_ValueText` / `_ValueComboBox`). `InjectSetting(ctx, key)` binds by
  key, rebind-safe. Slider commits on capture release only; Select cycles options or steps
  rangeless numerics. Dropdown (ComboBoxString, mouse/keyboard) is NEVER a resolution default —
  games opt in via `_SelectRowClassOverride`; it commits by index so duplicate labels stay
  unambiguous, and renders display-only for definitions without options.
- Screen: `UCk_GameSettingsUI_ScreenWidget` (ActivatableWidget layer participant) — bind
  `_RowContainer` (required), `_CategoryTabBar`, `_ApplyButton`/`_CancelButton`
  (UCommonButtonBase). Tabs = root segment of each setting's first category tag ("General" for
  uncategorized); no tab bar = flat list. Row classes resolve: the definition's own `_OptionalRowClassOverride`
  (per-setting escape hatch, beats everything) → project-settings
  `_RowClassOverrides`/`_SelectRowClassOverride` → native defaults (soft classes async-loaded).
  `_AutoApplyMode` (default) writes live; off = pending-changes session (Apply/Cancel).
  `OnRowGenerated` event = the WBP's styling hook.
  `Get_CuratedKeysForCategory(Category)` (BlueprintNativeEvent) is the CURATION hook: return the
  keys that category shows, in order; empty (the default) keeps every registered key in
  registration order. This is how a hand-authored page declines rows a pack registers — they stay
  live and driven, just off the screen — and how it fixes an order the registry cannot know.
- Keybinding page: `UCk_GameSettingsUI_KeyBindingPageWidget` + row — pure consumer of
  `UCk_Utils_KeyBinding_UE`/`UCk_Utils_KeyIcon_UE`. Bind `_RowContainer`, overlays, buttons; set
  `_RowWidgetClass` to the game's row WBP; `OnRowCreated` inserts category headers. Modal capture
  (Esc cancels), swap/overwrite/cancel conflicts, save-on-close. Conflict PRESENTATION is the
  game's choice: `_ConflictOverlay` bound = inline treatment; unbound = `OnConflictDetected(Text)`
  fires instead — show your own modal and answer via
  `Request_ResolveConflict(Swap/Overwrite/Cancel)` (rejected when nothing is pending).
- `UCk_InputActionWidget_UE` (CkUI) — CommonActionWidget that resolves its glyph through the
  player's mappable key profile, so a prompt still resolves while its Mapping Context is unapplied
  (every row of a rebinding screen) and refreshes on remap. Owned by CkUI, not this module.

---

## Anti-patterns

1. Don't read settings through `GConfig`/`UGameUserSettings` for Provider-policy keys — the
   registry is the source of truth; the provider file is its artifact.
2. Don't store video.* anywhere — they are External by design (`GameUserSettings.ini` owns them).
3. Don't call `Request_SetSettingValue_*` per drag tick from a custom slider — commit on release
   (the shipped Slider row is the reference).
4. Don't reset External settings to schema defaults — the registry rejects it on purpose.
5. Don't author widget trees in C++ subclasses — bind the slots in a WBP; unstyled native widgets
   deliberately render nothing.
6. Don't register the same key twice — re-registration is rejected; keys are global per
   GameInstance.
7. Don't reach for a CVar binding to drive a graphics quality, vsync or frame-limit value — those
   CVars have a second writer (`UGameUserSettings`) that wins on the next F11. Registration rejects
   it; the Video pack key or an External declaration over the `UGameUserSettings` accessor is the
   route. The pack currently covers seven of the twelve `sg.*` groups — GlobalIllumination,
   Reflection, Shading and Landscape have no pack key, so a game wanting those declares them
   External itself rather than binding the CVar.

---

## Tests

`Ck.CkGameSettings.{Registry×8,Store×3,Packs×1,UI×2}` C++ specs (pattern-run only — the toolbox
no-pattern suite excludes name-based `Ck.*` families) + 9 `Ck_AutoTest_GameSettings_*` AS
AutoTests in CkTests. Gym: "Game Settings" (CkTests) — plumbing surface; styling verification
belongs to the consuming game's WBPs.

## See also

- `docs/specs/2026-08-05-CkGameSettings-design.md` — the CTO-reviewed design of record.
- `docs/campaigns/2026-08-05-CkGameSettings/` — campaign PROGRESS + phase docs + BB migration plan.
- `CkInput` — keybinding/key-icon substrate the page consumes.
