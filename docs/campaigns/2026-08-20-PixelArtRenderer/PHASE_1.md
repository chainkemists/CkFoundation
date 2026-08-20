# PHASE 1 — Productionize CkPixelArtRender (state registry, CVars, lifecycle, tests)

> Entry: Phase 0 exit criteria met; D8 recorded in PROGRESS.md. Load `ck-macros-and-codegen`
> before writing any CK_ macro code. Editor CLOSED for builds.

## Executable spec

Write these C++ automation tests in **CkTests** FIRST (they compile red/failing against the
Phase-0 skeleton because the functions don't exist yet), under
`Plugins/CkTests/Source/CkTests/Private/UnitTests/CkPixelArtRender/`
(naming per the BB/CkTests convention: `FCkTest_PixelArtRender_<Scenario>`, registered
`"Ck.PixelArtRender.UnitTests.<Subject>.<Scenario>"`):

1. `Test_PixelArtRender_FractionExactness.cpp` — for every viewport width in
   {1280, 1366, 1367, 1600, 1920, 2560, 3440, 3840} × internal targets {320, 640, 644, 960}:
   `FMath::CeilToInt(ViewportW * UCk_Utils_PixelArtRender_UE::Get_ExactFraction(TargetW, ViewportW)) == TargetW`.
2. `Test_PixelArtRender_ConfigRoundtrip.cpp` — registry Set/TryGet/Clear roundtrip; TryGet on an
   unregistered world key returns unset.

Toolbox invocation + expected initial state (red), then green at exit:
`--test --test-pattern PixelArtRender` (new C++ specs need a relink + `--discover-fresh`).

## Steps

1. **State registry** `CkPixelArtRender_State.{h,cpp}`:
   - `FCk_PixelArt_RenderConfig` (plain struct, `CK_GENERATED_BODY` + private `_Members` +
     `CK_PROPERTY` accessors + `CK_DEFINE_CONSTRUCTORS`): `_Enabled` (bool, false),
     `_InternalHeight` (int32, 360), `_MarginTexels` (int32, 2), `_SnapEnabled` (bool, true),
     `_FilterMode` (enum `ECk_PixelArt_UpscaleFilter { BoxFilter, Nearest }` — Nearest is the
     debug filter; UENUM with 0 entry, `CK_DEFINE_CUSTOM_FORMATTER_ENUM`).
   - `class FCk_PixelArtRender_StateRegistry` — static, game-thread-only:
     `Set(const UWorld*, const FCk_PixelArt_RenderConfig&)`, `Clear(const UWorld*)`,
     `TryGet(const UWorld*) -> TOptional<FCk_PixelArt_RenderConfig>`; storage
     `TMap<TWeakObjectPtr<const UWorld>, FCk_PixelArt_RenderConfig>` with stale-key sweep on Set.
     All hooks that read it (`SetupViewFamily`, `SetupViewProjectionMatrix`,
     `BeginRenderViewFamily`, IsActive functor) are game-thread — assert
     `check(IsInGameThread())` in every accessor.
   - Public utils face `UCk_Utils_PixelArtRender_UE` (BPFL, minimal — most surface lands in
     Phase 4's subsystem): `Get_ExactFraction(int32 InTargetWidth, int32 InViewportWidth)`
     (static, pure math: `(InTargetWidth - 0.5f) / InViewportWidth`) so the executable spec has
     a seam.
2. **Replace the spike CVar** with the real set (defaults = "no override", folded like the
   Stylize CVars — `CkUsf_Stylize_CVars.{h,cpp}` is the exemplar): `ck.PixelArt.Enabled` (-1),
   `ck.PixelArt.InternalHeight` (-1), `ck.PixelArt.Margin` (-1), `ck.PixelArt.Snap` (-1),
   `ck.PixelArt.Filter` (-1), `ck.PixelArt.Debug.LogState` (0). Keep `ck.PixelArt.Spike` deleted
   by end of phase (grep returns 0).
3. **SVE reads the registry**: activation = registry-enabled (with CVar folds) for the context's
   world; all Phase-0 hardcodes removed. Per D8's chosen mechanism, fraction updates react to
   viewport resize every frame; disable path restores prior screen-percentage state (zero
   residue — verify by toggling 100× in a loop via console and checking `r.ScreenPercentage`
   and memory report deltas).
4. **Upscaler polish**: `FilterMode` permutation (`SHADER_PERMUTATION_BOOL("NEAREST_DEBUG")`),
   `OverrideOutput` path exercised (it is the last pass in the common case — verify the
   `check(SceneColor == ViewFamilyOutput)` engine contract holds by running once with
   `r.SecondaryScreenPercentage.GameViewport 50` where PrimaryUpscale is NOT last).
5. **Precondition sentinel** (D7 groundwork): a game-thread check when active —
   if `View.PrimaryScreenPercentageMethod != SpatialUpscale` on a frame where we are enabled,
   log ONE state-transition warning naming the AA method (this is the "TSR silently disables
   us" tripwire; full validation UX lands in Phase 4).
6. Run the executable-spec tests green; full suite scoped pattern + record.
7. Commit (module productionization; tests in CkTests commit separately).

## Exit criteria (measurable)

- Both spec tests green under `--test --test-pattern PixelArtRender --discover-fresh`.
- `rg -n "PixelArt.Spike" Source` → 0 hits.
- Toggle loop (100×) leaves `r.ScreenPercentage` at its pre-enable value; `[CkPixelArt]` state
  logs show clean enable/disable transitions.
- Full suite delta-zero vs baseline (names).

## Fences

- No `TObjectPtr`/strong refs in the registry — `TWeakObjectPtr` keys only (a fragment-style GC
  trap otherwise; see root CLAUDE.md "UObject refs").
- The SVE must never touch the registry off the game thread; `AddPasses` reads ONLY the baked
  copy inside the forked upscaler instance.
- No UFUNCTION overloads; `Request_*`/`Get_*` naming per root CLAUDE.md.
- Do not begin the Phase-2 snap work here even if trivial-looking — sign conventions get their
  own gates.
