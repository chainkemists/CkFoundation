# CkLoadingScreen

**Purpose:** Dependency-gated loading screen. A GameInstance subsystem evaluates every tick whether
ANY condition needs the loading screen — map load, pending/seamless travel, connecting,
GameState not replicated, world not begun play, or any `ICk_LoadingProcess` holder — and drops the
screen only when nothing holds it, then holds an extra configurable window (world rendering
re-enabled) so texture streaming / PSO compilation warm up behind the cover.

**Depends on:** `CkCore`, `CkLog`, `CkSettings` (+ engine: Slate, UMG, PreLoadScreen, RenderCore).
**Used by:** game-side travel/readiness systems (BusterBlock's GameFlow layer is the worked example).

**Provenance:** restyled port of Epic's `CommonLoadingScreen` plugin from the Lyra sample as shipped
with the UnrealEngine-Angelscript fork (`Samples/Games/Lyra/Plugins/CommonLoadingScreen`, fork
5.7.4, ported 2026-07-09). The decision-function logic (`DoCheckForAnyNeedToShowLoadingScreen` /
`DoShouldShowLoadingScreen` condition order) is kept **verbatim** from Lyra — do not "improve" its
ordering without re-reading the original. Lyra's `CommonStartupLoadingScreen` module
(engine-startup PreLoadScreen) was deliberately NOT ported.

---

## Key API

- `UCk_LoadingScreen_Subsystem_UE` (`Subsystem/`) — the manager.
  - `Get_IsLoadingScreenShowing()`, `Get_DebugReason()`, `Get_NeedsLoadingScreen()` (test surface),
    `BindTo_OnVisibilityChanged` / `UnbindFrom_OnVisibilityChanged`.
  - C++ poll-model holders: `Register_LoadingProcessor` / `Unregister_LoadingProcessor`
    (`TScriptInterface<ICk_LoadingProcess>`).
- `ICk_LoadingProcess` (`LoadingProcess/`) — one method:
  `Get_ShouldShowLoadingScreen(FString& OutReason)`. GameState + its components + local
  PlayerControllers + their components are polled automatically, no registration needed.
- **Built-in streaming gate** (Ck addition, not in Lyra): the screen also holds while any
  streaming sublevel that should be loaded/visible is still pending — the
  "fall through the floor on slow machines" guard. Toggle: settings `_WaitForStreamingLevels`
  (default true).
- `UCk_LoadingProcess_Task_UE` (`LoadingProcess/`) — push-model holder for BP/AS
  (BlueprintCosmetic — no-op/null on dedicated servers):
  `Create(WorldContext, Reason, FCk_Time InTimeout)` → screen held → `Request_Unregister()`.
  `InTimeout > 0` arms a fail-open watchdog: past the deadline the task fires an ensure
  (loud in dev, silent in Test/Shipping) and stops holding — a leaked holder can never
  permanently black-screen a packaged build. Zero = no watchdog.
- `UCk_LoadingScreen_ProjectSettings_UE` (`Settings/`) — widget class, z-order, hold-secs,
  heartbeat/log intervals; debug toggles are CVar-backed.

## CVars

- `ck.LoadingScreen.HoldAdditionalSecs` — post-loading hold window (default 2s; skipped in editor
  unless the settings flag `_HoldLoadingScreenAdditionalSecsEvenInEditor` is set).
- `ck.LoadingScreen.LogReasonEveryFrame` — the "why is it up / why did it drop" firehose.
- `ck.LoadingScreen.AlwaysShow` — force visible.
- `ck.LoadingScreen.Disable` — kill switch (presentation only, see below).

## Headless / test behavior (deliberate design)

Presentation (widget, input block, render suppression, GC-on-hide) is suppressed for commandlets,
`-unattended`, null-RHI runs, and `ck.LoadingScreen.Disable` — but **holder bookkeeping still
runs**: `Register_LoadingProcessor`, the Task objects, and `Get_NeedsLoadingScreen()` all work, so
AutoTests can assert hold/release semantics without a viewport. `-NoLoadingScreen` on the command
line also suppresses (non-Shipping, Lyra parity).

## Deliberate divergences from Lyra (beyond restyle)

1. Presentation-suppression layer (above) — Lyra has none.
2. `Deinitialize` unbinds `PreLoadMapWithContext` (Lyra removes from `PreLoadMap`, which it never
   bound — an upstream bug).
3. Visibility event is a BP/AS-bindable dynamic delegate (`BindTo_OnVisibilityChanged`) instead of
   a native-only multicast.
4. Empty-reason holders fire `CK_ENSURE_IF_NOT` and report `(no reason given)` instead of stock
   `ensureMsgf`.
5. CVar names moved to the `ck.LoadingScreen.*` namespace.

## Known issues (inherited from Lyra)

- **The additional-hold window re-enables world rendering and never re-disables it.**
  `DoShouldShowLoadingScreen` clears `bDisableWorldRendering` during the `HoldAdditionalSecs`
  window so textures actually stream in; if `DoCheckForAnyNeedToShowLoadingScreen` bounces back
  true inside that window, nothing turns it off again. Upstream Lyra carries the same defect.

## Anti-patterns

- Don't implement `ICk_LoadingProcess` from Blueprint/AngelScript — the interface is C++
  poll-model only; script holders use `UCk_LoadingProcess_Task_UE`.
- Don't hold a Task without a release path — pair every `Create` with an owned lifetime, and
  pass a `TimeoutSeconds` watchdog for any hold whose release depends on an external event
  (net arrival, discovery) that might never come.
- Don't gate game logic on `Get_IsLoadingScreenShowing()` in headless contexts — presentation is
  suppressed there; gate on `Get_NeedsLoadingScreen()` instead.

## See also

- BusterBlock `docs/superpowers/notes/2026-07-08-loading-screen-system-plan.md` — the full system
  plan this module is Phase 1 of (game-side holders, MapDefinition travel wrapper, preload).
