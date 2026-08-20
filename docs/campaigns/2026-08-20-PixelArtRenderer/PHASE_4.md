# PHASE 4 — CkPixelArt module: subsystem, preset, settings, 3-environment surface

> Entry: Phases 1–2 done (Phase 3 may land before or after; nothing here reads CkCamera).
> The exemplar to mimic END TO END is the CelShade suite:
> `Source/CkUsf/Public/CkUsf/Stylize/CkUsf_CelShadeSubsystem.{h,cpp}` + `CkUsf_CelShadePreset.h`
> + `CkUsf_CelShade_Params.h` + `CkUsf_Stylize_ProjectSettings.h` + `CkUsf_Stylize_CVars.{h,cpp}`.
> Read all five before writing a line (non-negotiable #1).

## Executable spec

AS AutoTest FIRST (red), CkTests `Script/CkPixelArt/CkAutoTest_PixelArt_SubsystemContract.as`:

1. `Subsystem::GetWorldSubsystem(UCkPixelArt_Subsystem)` valid in the test world.
2. `Get_IsEnabled()` false by default (no project default preset in the test config).
3. `Request_SetSettings` with a params struct (internal height 360, snap on) then
   `Get_Settings()` roundtrips every field.
4. `Request_SetEnabled(Enable)` in the test world (nullrhi lane): the subsystem must either
   enable OR fail loudly with the precondition report naming what blocked it — assert that
   `Get_PreconditionReport()` is non-empty in exactly the failure case, and that the state is
   internally consistent (`Get_IsEnabled()` matches what the request reported). No silent
   half-state.
5. `Request_ResetToDefaults()` returns settings to CDO defaults.

Toolbox: `--test --test-pattern PixelArt --discover-fresh`.

## Steps

1. **Module skeleton** `Source/CkPixelArt/` — Runtime/**Default** phase, T4. Build.cs deps:
   Core, CoreUObject, Engine, DeveloperSettings, GameplayTags, CkCore, CkEcs, CkLog, CkSettings,
   CkUsf, **CkPixelArtRender**. uplugin entry + Source/CLAUDE.md tier-table row (both modules)
   in the same commit.
2. **Params struct** `FCk_PixelArt_Params` (USTRUCT, `CK_GENERATED_BODY`, private UPROPERTYs +
   `CK_PROPERTY`, `CK_DEFINE_CONSTRUCTORS` with no essentials — all optional):
   - Renderer half: `_ResolutionMode` (`ECk_PixelArt_ResolutionMode { FixedHeight = 0, TexelsPerPixel }`),
     `_InternalHeight` (int32, 360, clamp 90–1080), `_TexelsPerPixel` (int32, 4, clamp 1–16),
     `_MarginTexels` (int32, 2, clamp 0–8), `_SnapEnabled` (ECk_EnableDisable, Enable),
     `_UpscaleFilter` (`ECk_PixelArt_UpscaleFilter`).
   - Look half (consumed in Phase 5; declare now so the struct is stable): `_ApplyLook`
     (ECk_EnableDisable, Enable) + `FCk_PixelArt_LookParams _Look` (its fields specified in
     PHASE_5.md — declare the nested struct now with the Phase 5 field list).
3. **Preset** `UCkPixelArt_Preset : UDataAsset` — one `FCk_PixelArt_Params _Params` +
   `CK_PROPERTY_GET`, mirroring `UCkUsf_CelShadePreset`'s shape.
4. **Project settings** `UCk_PixelArt_ProjectSettings_UE : UCk_Plugin_ProjectSettings_UE`
   (`meta = (DisplayName = "Pixel Art")`, config lands in DefaultCkFoundation.ini):
   `TSoftObjectPtr<UCkPixelArt_Preset> _DefaultPreset` (unset = off) — mirror
   `CkUsf_Stylize_ProjectSettings.h:26-69` incl. the settings-utils face.
5. **Subsystem** `UCkPixelArt_Subsystem : UWorldSubsystem`
   (`UCLASS(NotBlueprintable, BlueprintType, DisplayName = "CkSubsystem_PixelArt")`), the
   CelShade lifecycle VERBATIM-adapted: `ShouldCreateSubsystem` guards dedicated server;
   `Initialize` subscribes CVar-changed only; `OnWorldBeginPlay` → `DoApply_ProjectDefault()`
   with the engine-safe-blocking-loads gate + `OnFEngineLoopInitComplete` deferral;
   `_SettingsExplicitlySet` wins over the project default.
   UFUNCTION surface (Category `"Ck|Utils|PixelArt"`, DisplayName `"[Ck][PixelArt] …"`, static
   getter with WorldContext meta):
   - `static UCkPixelArt_Subsystem* Get_PixelArtSubsystem(const UObject* InWorldContextObject);`
   - `void Request_SetEnabled(ECk_EnableDisable InEnabled);`
   - `void Apply_Preset(const UCkPixelArt_Preset* InPreset);`
   - `void Request_SetSettings(const FCk_PixelArt_Params& InParams);`
   - `FCk_PixelArt_Params Get_Settings() const;`
   - `void Request_ResetToDefaults();`
   - `bool Get_IsEnabled() const;`
   - `FCk_PixelArt_PreconditionReport Get_PreconditionReport() const;` (struct: array of
     `FCk_PixelArt_PreconditionFailure { _Name, _CurrentValue, _RequiredValue, _FixCVar }` rows)
   - `void Request_Apply_RecommendedCVars();` (the D7 bounded escape: sets AA=None, dynamic res
     off, secondary SP 100 — each logged Display-level with old→new; stores priors; the disable
     path restores them).
   The enabled path writes `FCk_PixelArt_RenderConfig` into the Phase-1 registry for its world;
   the disabled/Deinitialize path clears it (zero residue).
6. **Precondition validation (D7)**: `DoGet_PreconditionReport()` checks — AA method is not
   TSR/TAA-upsampling (read the cvars), dynamic resolution disabled, ScreenPercentage show flag
   on. `Request_SetEnabled(Enable)` with failures: `CK_ENSURE_IF_NOT` with the full report text
   `{ records the report; does NOT enable; returns }` — never silently force cvars
   (non-negotiable #3; the escape is the explicit `Request_Apply_RecommendedCVars`).
7. **CVar fold-in**: the `ck.PixelArt.*` set from Phase 1 folds into effective settings the
   CelShade way (`DoGet_EffectiveSettings` — CVars are read on the way to the renderer config,
   never written into `_Settings`).
8. **AS/BP verification**: after regen, `Script/Generated/` must contain the subsystem's
   surface (subsystems bind via `Subsystem::GetWorldSubsystem` — verify with a one-line AS
   snippet in the AutoTest). Grep-check the generated file names into PROGRESS.md.
9. Spec test green; scoped suite; commit.

## Exit criteria

- Spec AutoTest green (fresh discovery).
- Enable in a real PIE/standalone world drives the Phase-1/2 renderer end-to-end from ONLY
  `Apply_Preset` (no console setup beyond `Request_Apply_RecommendedCVars`). `[EDITOR-VERIFY]`
- Precondition report populated + enable refused when TSR forced on; enable succeeds after
  `Request_Apply_RecommendedCVars`. (AutoTest-able headlessly if the AA cvar is settable in the
  test world; otherwise record as EDITOR-VERIFY with exact console steps.)
- Full suite delta-zero vs baseline.

## Fences

- Mirror CelShade's lifecycle EXACTLY — the lazy-creation, blocking-loads deferral, and
  `_SettingsExplicitlySet` semantics each exist because of a real incident; do not "simplify".
- Params fields: enum-mode + value pairs, never `TOptional`, never bare bools where
  `ECk_EnableDisable` fits (root CLAUDE.md).
- No writes to `_Settings` from the CVar layer.
- The subsystem never talks to the SVE directly — only through the state registry (the render
  module must stay consumable without CkPixelArt loaded).
