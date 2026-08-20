# CkPixelArt

**Purpose:** The game-facing half of the pixel-art renderer. Owns the configuration (params, preset,
project default), gates enabling on the engine settings that make a pixel-art image possible, and
applies the `PixelArt` CkUsf look. The rendering itself lives in `CkPixelArtRenderer`.

**Depends on:** `CkCore`, `CkEcs`, `CkLog`, `CkSettings`, `CkUsf`, `CkPixelArtRenderer` (+ engine
DeveloperSettings / GameplayTags).

**Used by:** nothing yet in the suite — this is the module a game project talks to.

---

## Key API

Everything is on `UCkPixelArt_Subsystem` (a `UWorldSubsystem`):

- `Get_PixelArtSubsystem(WorldContext)` — static getter. In AngelScript the context is filled in, so
  it reads `UCkPixelArt_Subsystem::Get_PixelArtSubsystem()`.
- `Apply_Preset(Preset)` / `Request_SetSettings(Params)` / `Get_Settings()` / `Request_ResetToDefaults()`
- `Request_SetEnabled(ECk_EnableDisable)` / `Get_IsEnabled()`
- `Get_PreconditionReport()` — empty when the renderer can run; otherwise one row per blocking
  setting, each carrying its current value, the required value, and the console command that fixes it.
- `Request_Apply_RecommendedCVars()` — the explicit escape that moves those settings, remembering
  priors so disabling restores them.

Shipped presets (`Script/CkPixelArt/CkPixelArt_Presets_Assets.as`): `DA_PixelArt_Crisp16`,
`DA_PixelArt_SoftRamp`, `DA_PixelArt_RendererOnly`.

## The two halves are independent, on purpose

`_ApplyLook` and the renderer settings do not depend on each other:

- Renderer only (`DA_PixelArt_RendererOnly`) is a sharp, snapped, low-resolution image with no
  stylization. This is the control every visual verdict about the look has to be read against.
- Look only (renderer disabled, look applied through CkUsf directly) is an ordinary full-resolution
  stylization.

Pairing them is composition, not coupling — which is also why the look asset lives in `CkUsf` and
never references the renderer's state.

## Enabling is gated, and the gate is loud

`Request_SetEnabled(Enable)` recomputes the precondition report first. If anything is unsatisfied it
ensures with the full report and leaves the renderer OFF — there is no half-enabled state, and
`Get_IsEnabled()` tells the truth afterwards. Preconditions checked:

| Setting | Required | Why |
|---|---|---|
| `r.AntiAliasingMethod` | 0 (None) or 1 (FXAA) | TSR and TAA switch the view to temporal upscaling, which structurally disables the spatial upscale slot the renderer occupies |
| `r.DynamicRes.OperationMode` | 0 | Dynamic resolution fights the screen percentage the renderer drives |
| `ShowFlag.ScreenPercentage` | on | With it off the scene never renders at the internal resolution |

The renderer never silently moves any of these. `Request_Apply_RecommendedCVars` is the only writer, it
logs every change at Display level with old → new, and it writes at `ECVF_SetByConsole` — because the
situation it exists to rescue is usually one where somebody typed the offending value into the console,
and a `SetByCode` write would be silently dropped underneath it. The escape also sets the show flag, so
it can clear every row it reports; an escape that cannot is a trap.

**Two things about those console variables that are easy to get wrong, and were:**

- **They are PROCESS-wide, so the lease is too.** Priors live in a refcounted process-level table, not on
  the subsystem. Held per world, a second PIE world's teardown restores values out from under a first
  world that is still rendering — and the second world holds no prior of its own to put back, because its
  own apply was a no-op on an already-moved variable.
- **A console variable remembers WHO set it last, and the restore has to put that back too.** Writing the
  number back at this module's own priority pins the variable there: after one apply/restore cycle it
  would ignore project settings, device profiles and scalability for the rest of the process. The restore
  writes the value at the CURRENT priority (so the write cannot be dropped) and then rewrites the original
  priority bits. `r.ScreenPercentage` has the same treatment in the render module, where getting it wrong
  silently killed the resolution-quality slider.

`Get_PreconditionReport()` recomputes every call and is never cached, including right after a
refusal. A cached refusal would keep reporting the problem after the caller had fixed it, which is
exactly the moment someone is reading it and trusting it.

## Lifecycle

Mirrors the CkUsf Stylize subsystems, and each step of that shape exists because of a real incident:

- `ShouldCreateSubsystem` refuses on a dedicated server (nothing renders there).
- `OnWorldBeginPlay` → `DoApply_ProjectDefault()`, which defers through
  `OnFEngineLoopInitComplete` when the engine is not yet safe for blocking loads — a packaged game
  runs its startup map's BeginPlay from inside `FEngineLoop::Init`, before that flag flips.
- `_SettingsExplicitlySet` makes game code win over the project default, and the deferred path checks
  it — otherwise the project row would silently undo a value gameplay already chose.
- `Deinitialize` clears the world's registry entry and restores any moved console variables.

## Anti-patterns

- Talking to the scene view extension. The state registry is the only channel, and keeping it that
  way is what lets `CkPixelArtRenderer` be consumed without this module.
- Keying the look's lazy-creation on the MID alone. The MID is outered to the subsystem and outlives the
  world-spawned actor that carries the blendable, so a check that only asks about the MID reports "already
  built" forever once the actor is gone, and the look silently stops rendering. `DoEnsure_LookEffect`
  validates all three and rebuilds — it is a reconciler, not a cache.
- Folding `ck.PixelArt.*` overrides into `_Settings`. The render module applies them on the way OUT
  of the registry; folding them in would make an override indistinguishable from a setting and leave
  it behind when the CVar goes back to -1.
- Forcing CVars from the enable path. That is the "no fallbacks that hide problems" rule
  (root `CLAUDE.md` non-negotiable #3) — the report plus the explicit escape is the sanctioned shape.
- Nesting a params struct in the preset. An AngelScript `asset` block assigns reflected properties by
  name and cannot reach into a nested struct, which is why the preset mirrors the params flat.
- Assuming a PIE image is the verdict. See the supported-feature matrix in
  [../CkPixelArtRenderer/Claude.md](../CkPixelArtRenderer/Claude.md).
