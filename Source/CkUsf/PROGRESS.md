# Stylize campaign — PROGRESS.md (living log)

## Current state  <!-- supersedes everything below; update at EVERY gate and session end -->
**As of 2026-08-06 (CkFoundation `9967963b4`+wt, CkTests `fd26b553`+wt):** **Gate 1 DONE** —
extension + pattern library + permanent probe + strengthened compile gate all landed and verified
(12/12 `-nullrhi`; 6/6 real-RHI serial; mutation test proves the gate has teeth; 0 latent roster
failures). Landed as the commit carrying this entry (CkFoundation) + its CkTests sibling; NOT pushed.
**Baseline being diffed against:** toolbox `--test-pattern Usf`, Development editor,
**10/10 passed, 0 failed** (32s): Ck_AutoTest_UsfOutline_{BatchedMembers, CascadeDependents,
IskmApplyRemove, IsmShadowInstances, VatShadowCustomData}, GeneratesUsableMasters,
MultiPassRendersToTexture, NiagaraSpriteContract, OutlineStencilAlloc,
FRigVMFunction_DeltaFromPreviousFloat (engine-side pattern match, ignorable). Captured
2026-08-06 after deleting the stale `utils_world_space_widget.as` (see dated entry).
**Gate 2 DONE 2026-08-06** (landed as the commit carrying this entry + CkTests sibling; baseline
now 15/15 standard / 9/9 real-RHI serial). Gym visuals are the maintainer's [EDITOR-VERIFY]
(steps in the Gate-2 dated entry).
**Next action:** Gate 3 (CelShade) entry pre-flight + executor dispatch.
**Blocked on:** nothing.

## Decision log
| Date | Decision | Why | Revisit when |
|---|---|---|---|
| 2026-08-06 | Three PP-domain looks + per-effect world subsystems + DA presets; NOT a SceneViewExtension | House machinery covers it; see PROMPT.md locked table | Never (maintainer-approved) |
| 2026-08-06 | Cut moving-object temporal stabilization | Needs temporal history a blendable lacks; experimental + artifact-prone in source | If CkUsf ever grows a real SVE/history mechanism |
| 2026-08-06 | Cel illumination = SceneColor/max(BaseColor,eps) | Only PP-available approximation | Gate 3 gym verdict — prewritten pivot in Gate_03 observations table |
| 2026-08-06 | Global settings as MID params, not a LUT | LUT is for per-pixel preset addressing; globals don't need it | Gate 1 param-count proof failure |
| 2026-08-06 | Build in CkPlugins checkout | Maintainer directed work here | — |
| 2026-08-06 | Opus subagents execute gate work items; Fable audits gate exits | Triage routing (b)/(d), maintainer-approved | An executor result failing audit twice |
| 2026-08-06 | One gym PER effect, shipped inside its feature gate (2–4), Solid Outline gym pattern; stations act as preset SELECTORS (effects are view-wide) over a shared judge scene | Maintainer asked for proper gyms like the outline's; per-gate delivery because each gate's [EDITOR-VERIFY] needs its gym | — |

## Dated entries (append-only, newest first)

### 2026-08-06 — Gate 2 (ScreenDither) EXIT: executed, adversarially audited, fixed, re-verified
- Delivered: `Looks/ScreenDither.ush` (27 params), `Stylize/CkUsf_ScreenDither_Params.h/.cpp`
  (+4 enums), `CkUsf_ScreenDitherPreset`, `CkUsf_ScreenDitherSubsystem` (outline-subsystem
  mechanics, changed-only MID writes), AS look + 6 presets, `Test_Usf_ScreenDither.cpp` (3 tests),
  gym "Stylize: Screen Dither" (preset-selector stations + judge scene + Exec cmds), Claude.md
  section. New tracked master `M_CkUsf_Look_ScreenDither.uasset`.
- Audit (fresh-context adversarial pass): ACCEPT-WITH-FIXES. CONFIRMED+FIXED: C1 grid space
  (now `GetSceneTextureViewSize(PPI_PostProcessInput0)`; display-res blocks), C2 palette encode-
  space match, C3 empty-CustomPalette silent black → loud reject + zero mutation (+ tests), C5
  null-world test (pins no-crash+nullptr, outline precedent), C6 gym hysteresis (+4 polish).
  Verified-clean list (enum contracts, 27-name MID seam vs .uasset, cache staleness, passthrough
  exactness, NN#3 shapes) in the audit record.
- Ran (post-fix): standard `-nullrhi` Usf → **15/15** (baseline 12/12); real-RHI serial CkUsf →
  **9/9**, ScreenDither.ush force-compile CLEAN; churn restored.
- Known/accepted: `AddExpectedError(-1)` proves suppression-tolerant whitelist not fire-count
  (house idiom; zero-mutation asserts are the real proof); HDR-output saturate clip inert on SDR;
  PixelScale tastefulness at display res = preset knob (`_PixelScale`).
- **[EDITOR-VERIFY] (maintainer)**: PIE → Tab → "Stylize: Screen Dither" → walk the 6 stations
  (Balanced fine texture / SubtleColor subtle / RetroPixel pixelated+reduced / FourColorHandheld
  EXACTLY 4 greens with ordered dither / AnimatedGrain film grain, no blotches / Off perfectly
  clean); `Ck_GymStylizeDither_CycleDebug` (Pattern/QuantError/NoDither/Downsampled);
  master must exist (else `Ck_Usf_GenerateLooks ScreenDither` once).

### 2026-08-06 — Gate 1 EXIT: compile gate strengthened, mutation-proven; all exit criteria met
- Executor follow-up landed (Fable-audited, full diff read):
  `Validate_LookShaders` → exported `Validate_LookShaderCompile(Material, LookName, OutErrors,
  InForceSynchronousCompile)` in `CkUsf_Generator.h/.cpp`. Reads
  `GetMaterialResource(GMaxRHIShaderPlatform)->GetCompileErrors()` — the 5.7 signature takes
  EShaderPlatform (feature-level overload deprecated, engine `MaterialInterface.h:617`; Fable's
  audit initially flagged this call and was WRONG — verified against engine source). A failed job
  sets both the error list and a null shader map (engine `ShaderCompiler.cpp:2177`); errors are
  authoritative, missing-map alone is a warning (49/49 false positives when gated on it).
- Force-compile is opt-in and DESTRUCTIVE (measured: forced masters render black —
  `MultiPassRendersToTexture` failed when forced across the roster); generation path passes
  `false`, only throwaway test masters force. `CkTests.Build.cs` RHI dep reverted (no longer needed).
- Mutation evidence: undefined identifier in StylizeProbe.ush — before: PASSED (twice, cache-busted);
  after: FAILS with `/CkUsf/Looks/StylizeProbe.ush:69:50: error: use of undeclared identifier ...`.
  Mutation reverted.
- Ran: (a) `--no-nullrhi --parallel 1` CkUsf unit pattern → 6/6, 0 shader failures, 0 latent roster
  errors; (b) standard `-nullrhi` Usf pattern → 12/12 vs 10/10 baseline; (c) GeneratedLooks churn
  restored.
- Inferred (executor-flagged, unconfirmed): `GetCompileErrors()` proven for HLSL errors only;
  a translation-time material-graph error may still surface as the no-error-text warning path.
  Acceptable: the generator's own force-compile validation covers translation at authoring time.
- Gate 1 exit checklist: all items ticked in Gate_01_Foundation.md (amendments recorded there).

### 2026-08-06 — Gate 1 work items landed (executor) + Fable audit; ONE item remains (compile-gate teeth)
- Executor delivered work items 1–3 + 5, item 4 as the permanent `StylizeParamCount` test.
  Files: `CkUsf_LookDefinition.h` (+4 enum entries, `_PostProcessWorldPosition`),
  `CkUsf_Generator.cpp` (+4 wiring rows, WorldPosition opt-in at all 3 PP sites),
  `CkUsf_LookValidator.cpp` (+4 reserved names, non-PP warning), `Common.ush` (+4 fields),
  NEW `StylizeCommon.ush` + `Looks/StylizeProbe.ush`,
  NEW CkTests `Test_Usf_StylizeContract.cpp` (+`RHI` dep in CkTests.Build.cs — approved deviation).
- Ran (executor, audited from its report): standard harness 12/12 passed vs 10/10 baseline
  (+StylizeParamCount, +StylizeSceneTextureNegative). Confirmed by Fable: full diff read;
  4-file extension exactly per contract; default-trio invariant untouched.
- **CONFIRMED (executor mutation-test, 2026-08-06): the shader-compile gate is TOOTHLESS** —
  `Validate_LookShaders`' `IsCompilingOrHadCompileError` is just `Res==nullptr || ShaderMap==nullptr`
  (engine Material.cpp:1649) and passes an undefined function even cache-busted; the toolbox
  harness also runs `-nullrhi`, so ALL CkUsf generation tests (incl. shipped GeneratesUsableMasters)
  skip vacuously in the standard lane. Pre-existing weakness, exposed not introduced.
  50-param verdict therefore: GENERATES proven (62 inputs declared AND connected); COMPILES not
  yet proven. Follow-up dispatched to the executor: strengthen `Validate_LookShaders` to read real
  `FMaterialResource` compile errors, expose it to tests, mutation-test it, re-run scoped
  `--no-nullrhi`. Gate 1 does NOT exit until this lands.
- Fable audit fixes applied inline: `CkUsf_Stylize_QuantizeSteps`/`QuantizeLuminance` clamped
  (input exactly 1.0 overshot to N/(N-1)); 75 regeneration-churned GeneratedLooks .uasset restored
  via `git checkout --`.
- Follow-ups recorded, not chased: (a) toolbox real-RHI parallel lanes both running
  `Generate_AllLookMaterials` collide on SavePackage (ERROR_ALREADY_EXISTS, editor dies) — harness
  fix needed, out of campaign scope; (b) `StylizeSceneTextureNegative`'s roster arm regenerates +
  SAVES all shipped PP masters (churns Content in real-RHI runs — restore after).

### 2026-08-06 — Gate 1 pre-flight: engine facts CONFIRMED; generator/validator insertion points named
- Engine verification (agent-run against D:/Repositories/UnrealEngine-Angelscript 5.7.4, audited):
  - **GBuffer reads in MD_PostProcess: CONFIRMED.** No id filtering for PP domain
    (`HLSLMaterialTranslator.cpp:8627-8634` allowlists only MD_DeferredDecal); decode cases for
    PPI_BaseColor/Metallic/Roughness/Specular at `MaterialTemplate.ush:3344-3353`; full
    SceneTextures UB (SetupMode All incl. GBuffers, `DeferredShadingRenderer.cpp:3234`) bound
    identically for EVERY chain (`PostProcessing.cpp:631` used at :819/:861/:1139/:1517).
    CORRECTION to plan wording: availability does NOT differ by location — pre-TAA is chosen for
    temporal stability only. Missing GBuffer degrades to BLACK silently (`SceneTextures.cpp:1074`).
    Rejections that can fire: forward shading (:8688), mobile (:8696). Substrate: decode differs,
    ids same; PP-material-under-Substrate gate inferred-safe, unconfirmed (project doesn't use it).
  - **Trap: PPI_SceneColor is REJECTED in PP domain** (`HLSLMaterialTranslator.cpp:8656`) — we
    already use PPI_PostProcessInput0; never add a PPI_SceneColor wiring row.
  - **PP WorldPosition: CONFIRMED.** `PostProcessMaterialShaders.usf:267-271` overwrites
    SvPosition.z with LookupDeviceZ before CalcMaterialParametersPost → SvPositionToTranslatedWorld
    (`MaterialTemplate.ush:4683`, `Common.ush:1520`) → AbsoluteWorldPosition. Caveat: at
    after-tonemap/SSR locations the reconstruction is dynamic-res scaled (:4686) — WorldPosition
    consumers (CelShade/HandDrawn) sit at AfterDOF; ScreenDither (AfterTonemapping) never reads it.
  - **PPI ids: CONFIRMED** (`MaterialSceneTextureId.h:12-87`): BaseColor=5, Specular=6,
    Metallic=7, WorldNormal=8, Roughness=11, CustomDepth=13, PostProcessInput0=14, AO=24,
    CustomStencil=25. Note :24 — BaseColor/Specular "can be modified on read by the ShadingModel";
    PPI_StoredBaseColor=26/PPI_StoredSpecular=27 are the raw variants (candidate if metals read oddly).
  - **Managed SceneTexture wiring is load-bearing**: usage is RECORDED by compiling the expression
    (`HLSLMaterialTranslator.cpp:8542,8624-8625`); raw Custom-node lookups alone read a dummy.
- Generator/validator read (Fable, direct): extension points are
  `CkUsf_Generator.cpp:162` (`Get_SceneTextureWiring` table — everything downstream iterates it),
  `:496-560` (PP branch, WorldPosition opt-in insertion), `:175` (`Get_EffectiveSceneTextures` —
  default-trio invariant untouched), `CkUsf_LookValidator.cpp:15` (`kReservedParamNames` KEEP-IN-SYNC),
  `:404-411` (non-PP `_SceneTextures` warning shape to mirror for the WorldPosition opt-in).
  New managed input names: SceneBaseColor/SceneMetallic/SceneRoughness/SceneSpecular.
- Ran: gym-pattern read (`CkUsfOutlineGym_*.as`, `CkTests_GymRegistry.as:68`) → per-gate gym work
  items written into Gates 2–4.
- Baseline build+test attempt log:
  1. REFUSED — config-flip guard (asked DebugGame, last-built=Development). Re-ran `--config=Auto`.
  2. AS_COMPILE_FAILED (exit 76) — INHERITED red, not ours (our diff was markdown-only). C++ build
     itself succeeded. Cause confirmed: `Script/Generated/utils_world_space_widget.as` (untracked,
     mtime 2026-08-06 01:44, authored by an earlier editor session) declared the WorldSpaceWidget
     utils as taking `FCk_WorldSpaceWidget_Spec` — a type that exists NOWHERE in current C++
     (current signatures take `FCk_Fragment_WorldSpaceWidget_ParamsData`,
     `CkWorldSpaceWidget_Utils.h:30-48`). Stale generated file from a binary that briefly had a
     renamed type; it blocks editor boot BEFORE the generator can regenerate it (self-heal:
     "no strategy applies"). Recovery: deleted the stale file (regenerates from the real binary
     on next boot); baseline attempt 3 in flight.

### 2026-08-06 — campaign chartered
- Research: two doc-crawl agents produced the full yShade capability inventories → persisted as
  Plan/Research_yShade_HandDrawn.md + Plan/Research_yShade_CelDither.md (session-independent).
- Confirmed: CkUsf architecture facts from direct reads — LookDefinition fields
  (`CkUsf_LookDefinition.h`), PP look shape (`EdgeOutline.ush`), outline subsystem mechanics
  (`CkUsf_OutlineSubsystem.h`), outline preset shape (`CkUsf_OutlinePreset.h`), `Common.ush`
  input structs + existing dither/remap helpers (lines 486–500).
- Confirmed: no PP world-position helper exists in Common.ush (grep over Shaders/CkUsf).
- Inferred (unconfirmed, Gate 1 verifies): GBuffer SceneTexture reads legal at
  `SceneColorAfterDOF` in a PP material; `UMaterialExpressionWorldPosition` depth-reconstructs in
  PP domain; ~50 Custom-node inputs generate + compile.
- Follow-ups recorded, not chased: renderer-module (ISM/ISKM) cel-pattern sync processors;
  BusterBlock checkout will need this work pulled through its CkFoundation submodule eventually.

## Open items
| Item | Status | Next step |
|---|---|---|
| Gate 1 pre-flight | Resolved 2026-08-06 | — (baseline + engine facts in dated entries) |
| Gate 2 entry pre-flight | Resolved 2026-08-06 | — |
| Gate 2 gym [EDITOR-VERIFY] | Open (maintainer) | Steps in the Gate-2 dated entry |
| Gate 3 entry pre-flight | Open | Gate_03 entry criteria (stencil/outline reads; GBuffer evidence re-check) |

**Rule: no completion claim may be written anywhere in this file while any row here is unresolved.**
