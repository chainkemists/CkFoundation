# Stylize campaign — mission brief (PROMPT.md)

> **Written:** 2026-08-06. STABLE content only — current state lives in [PROGRESS.md](PROGRESS.md).
> **This doc dies when:** the campaign ships and CkUsf/Claude.md carries the permanent contract.
> On death: delete it (with the Plan/ gate files), per the CkNavigation post-ship-cleanup precedent.

## Goal

CkUsf gains the full capability set of the two yShade plugins (Easy Hand-Drawn Shading; Easy Cel
Dither Shading) as three PostProcess-domain looks with per-world runtime subsystems and data-asset
presets: **HandDrawn** (paint/ink/shadow-strokes/paper), **CelShade** (quantized bands, halftone
transition patterns, sky/metallic/specular/rim treatments, its own outline, per-object pattern
selection via Custom Stencil), and **ScreenDither** (post-tonemap palette reduction, ordered/noise
dithering, pixelation). Everything is clean-room re-implemented from the documented feature
inventory (Plan/Research_yShade_*.md) — no yShade code is ever read or copied.

## Success criteria

1. In a gym, applying the CleanAnime cel preset shows banded cel shading with halftone band
   transitions in PIE; switching presets at runtime visibly swaps the style without hitches.
2. A mesh whose Custom Stencil = base+1 renders RoundDots transitions while neighbors render the
   global pattern; a mesh at base−1 renders no transitions. `Request_SetCelPattern` on an entity
   achieves the same without touching components directly.
3. Applying the RetroPixel dither preset shows a pixelated, palette-reduced final frame; the
   4-Color Handheld preset reduces to 4 colors with visible ordered dither.
4. Applying the StorybookInk hand-drawn preset shows ink contours + hatched shadows + paper grain;
   the world-attached stroke space stays glued to static geometry under camera motion.
5. Every pre-existing look regenerates byte-identically after the SceneTexture/WorldPosition
   generator extension (negative test, mirroring NiagaraSpriteContract).
6. CkTests: generation tests for the three looks, subsystem settings round-trips, and
   invalid-input rejection tests all pass; the three environments (C++, BP, AS) can each drive
   settings end-to-end.
7. All purely-visual claims are covered by `[EDITOR-VERIFY]` steps with exact clicks — never
   claimed done by an agent.

## Constraints & locked decisions

| Decision | Choice | Why |
|---|---|---|
| Rendering vehicle | PostProcess-domain looks via the existing generator, NOT a SceneViewExtension | The look pipeline already provides material assembly, validation, MID runtime, blendable placement; mimicry beats invention (non-negotiable #1) |
| Runtime shape | One `UWorldSubsystem` per effect, mirroring `UCkUsf_OutlineSubsystem` (hidden view actor + PP component + cached MID; write only changed params) | Shipped house exemplar for exactly this feature shape |
| Settings flow | `UCkUsf_<Effect>Preset : UDataAsset` (public fields, per `UCkUsf_OutlinePreset` precedent) + reflected `FCk_Usf_<Effect>_Params` struct (house ParamsData style) with `Apply_Preset` / `Get_Settings` / `Request_SetSettings` | Two-level flow mirrors yShade's own doctrine; presets authorable in AS |
| Blendable locations | HandDrawn + CelShade at `SceneColorAfterDOF` (pre-TAA); ScreenDither at `AfterTonemapping` | Stencil-derived looks REQUIRE pre-TAA (house doctrine); palette reduction must see final LDR (yShade does the same) |
| Cel illumination | Reconstructed as `SceneColor / max(BaseColor, eps)` pre-tonemap | Material blendables cannot read deferred lighting directly; standard PP approximation; Metallic group compensates on metals |
| Stencil contract | Configurable base (default 200), base+0..9 = pattern in enum order, base−1 suppresses; disjoint from outline's 240–255; overlap validated loudly | Preserves yShade's documented contract; coexists with entity outlines |
| Per-object entity API | `Request_SetCelPattern(Handle, Pattern, Scope)` target fragment + per-renderer sync processors, mirroring DESIGN_EntityOutlines | Same problem shape as entity outlines |
| Defaults/ranges | Our own, tuned in the gym | yShade docs publish none (verified) |
| Checkout | This CkPlugins checkout | Maintainer directed the work here |
| Execution routing | Opus subagents execute gate work items; Fable audits at gate exits | Triage routing (b)/(d); maintainer approved |

## Non-goals

- **Moving-object temporal stabilization** (cel, experimental in source): needs temporal history a
  material blendable does not have; source's own docs warn of trail artifacts. Documented as a
  limitation instead.
- **Forward / mobile / Substrate support**: source plugin doesn't support them either
  (cel pass is deferred-only); ScreenDither inherits whatever the PP material path supports.
- **Scoped-override actor components** (yShade's `*ShadingComponent`): redundant beside presets +
  subsystem calls in an ECS codebase.
- **Reading or porting yShade code**: capabilities only, from the docs crawl. Clean room.
- **yShade CVar-name compatibility** (`r.YShade.*`): ours are `ck.Usf.*`.

## Reading list

- Gate index: [PLAN.md](PLAN.md); research: `Plan/Research_yShade_HandDrawn.md`,
  `Plan/Research_yShade_CelDither.md`.
- Mimicry references: the Outline feature (`Outline/CkUsf_OutlineSubsystem.h/.cpp`,
  `CkUsf_OutlinePreset.h`, `CkUsf_Outline_Fragment.h`, `CkUsf_Outline_Utils.h`,
  `CkUsf_Outline_Processor.cpp`, `DESIGN_EntityOutlines.md`), `Looks/EdgeOutline.ush` (PP
  neighbor-tap look), `Looks/SolidOutline.ush` (stencil-driven PP look), `Common.ush`
  (stdlib; IGN dither, remap, triplanar), `CkUsf/Claude.md` (parameter contract, traps),
  CkUsfEditor generator + validator, `CkTests/.../UnitTests/CkUsf/` (generation-test shape).

## Things ruled out — do not re-investigate

| Ruled out | Why | Evidence |
|---|---|---|
| SceneViewExtension port | Duplicates the look pipeline; loses validator/MID/AS integration | CkUsf/Claude.md architecture |
| Params-LUT texture for GLOBAL settings | LUT exists for per-pixel preset addressing (outline); global settings are plain MID params — simpler, validator-covered. Revisit ONLY if the ~50-input cel master fails to generate/compile (Gate 1 proves it) | Outline LUT rationale; Gate 1 work item |
| Reading GBuffer lighting directly in a blendable | Not exposed to PP materials | Engine PP material inputs |
| Per-preset stencil ALLOCATION for cel patterns | yShade's contract is direct-value (base+index), not allocation; allocation would break the documented mesh-authoring workflow | Research_yShade_CelDither.md §per-object |
