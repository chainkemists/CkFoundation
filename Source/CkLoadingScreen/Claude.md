# CkLoadingScreen

**Purpose:** Dependency-gated loading screen. A GameInstance subsystem evaluates every tick whether
ANY condition needs the loading screen — map load, pending/seamless travel, connecting,
GameState not replicated, world not begun play, or any `ICk_LoadingProcess` holder — and drops the
screen only when nothing holds it, then holds an extra configurable window (world rendering
re-enabled) so texture streaming / PSO compilation warm up behind the cover.

**Depends on:** `CkCore`, `CkLog`, `CkSettings` (+ engine: Slate, UMG, MoviePlayer, PreLoadScreen,
RenderCore).
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

## The transition screen (MoviePlayer layer) — the OTHER half

The subsystem above is a **game-thread** screen: a UMG widget in the viewport. It cannot cover the
part of a map load where the game thread is *blocked* — during `LoadMap` there is no tick, so a UMG
screen freezes on its last frame (and on a first-ever travel may not exist yet). That frozen window
is what the transition layer covers.

`TransitionScreen/` is a pure-Slate widget handed to the engine's **MoviePlayer**, which renders it
from the Slate loading thread while the game thread is stalled. Off by default
(`_TransitionScreenEnabled`); a project opts in from its own ini. It absorbs the mechanism of the
`AsyncLoadingScreen` marketplace plugin BusterBlock retired (2026-08-19) — ~250 lines of the
plugin's 3,090 were worth keeping.

- **`SCk_LoadingScreen_Transition`** — tint plate → optional backdrop (ScaleToFill) → corner logo →
  optional tip. Animation is **paint-driven**: `OnPaint` accumulates `Args.GetDeltaTime()` and drives
  the logo's opacity. There is no Tick and no timer out there — the thread that would run them is the
  blocked one. Every brush arrives pre-resolved as `FDeferredCleanupSlateBrush`; the widget takes
  brushes, literals and FTexts, and **touches no UObject after construction**.
- **`FCk_LoadingScreen_TransitionAttributesBuilder`** — the mode → `FLoadingScreenAttributes`
  translation, split out from the module so the flag matrix is unit-testable
  (`Ck.LoadingScreen.TransitionAttributes.*`). The MoviePlayer is inert in automation, so these flags
  are the only part of the layer automation can assert on.
- **Texture lifetime** lives in the subsystem: `Initialize` async-loads the logo + backgrounds and
  keeps the `FStreamableHandle` as a member. That handle IS the GC root. The module resolves brushes
  with `FSoftObjectPath::ResolveObject()` — never `TryLoad` — so a not-yet-resident texture degrades
  to "no backdrop", never to a sync load on the game thread at travel start.

### The handshake (`_TransitionWaitForGameThreadScreen`, default on)

Handshake mode sets `bWaitForManualStop=true` + `bAutoCompleteWhenLoadingCompletes=false`, and the
subsystem stops the movie once its own widget is mounted — so the movie tears down with the UMG
screen already in the viewport and the "single frame of raw world" seam cannot happen.

Two things about it are load-bearing:

- **`bAllowEngineTick=true` is MANDATORY in handshake mode.** The manual-stop wait loop blocks the
  game thread; without the engine tick the subsystem never ticks, so nothing can ever reach
  `StopMovie` and the game deadlocks at the end of every load. A unit test asserts no mode can ever
  emit `bWaitForManualStop && !bAllowEngineTick`.
- **The signal is "mounted", not "painted".** `FDefaultGameMoviePlayer::WaitForMovieToFinish` swaps
  the main window's content to the movie for the whole wait, so a viewport-mounted widget *cannot*
  paint until after `StopMovie`. Requiring a paint would burn the fail-open cap on every travel.
- **Fail-open cap: 5s** after `IsLoadingFinished()` with no widget mounted → stop anyway + `Warning`.
  A cosmetic handshake must never be able to hold the game hostage.

### Boot logos — NOT via `_StartupMoviePaths` at the current loading phase

`_StartupMoviePaths` + the eager `SetupLoadingScreen` in `StartupModule` mirror the retired plugin,
but this module loads at **`ELoadingPhase::Default`**, and `LaunchEngineLoop` calls `PlayMovie()` for
the startup screen *before* that (PreLoadingScreen modules 3748 → `PlayMovie` 3810 → Default modules
4617). The retired plugin loaded at `PreLoadingScreen`, which is why its passthrough worked.

So the setup call is **guarded and is a no-op today**, and projects should route boot logos through
the engine's own `[/Script/MoviePlayer.MoviePlayerSettings] StartupMovies` (read at 3507). Moving
this module to `PreLoadingScreen` would drag `CkCore`/`CkLog` up with it — a framework-wide change
`Source/CLAUDE.md` explicitly discourages.

### Mode A (`_TransitionModeA_UMGHandOff`) — experimental, ships dark

Hands the configured UMG widget to the loading thread instead of the Slate one. UMG ticked on the
Slate loading thread is the Ultra-plugin pattern: works in the common case, **unverified with the
AngelScript VM**. Takes parity flags deliberately — when the loading-thread widget *is* the
game-thread widget there is no seam for a handshake to cover, and manual-stop on an unhardened path
is the one failure mode that can wedge a boot. Falls back to the Slate screen if its widget fails to
build.

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
- **Don't let the transition widget touch a UObject after construction.** It runs on the Slate
  loading thread. Pre-resolve everything into `FDeferredCleanupSlateBrush`. The retired plugin's
  `TryLoad()`-in-`Construct` (a sync load on the game thread at travel start) and its un-rooted
  `TArray<UTexture2D*>` module cache (a GC hole) are the counter-examples — both were deliberately
  NOT ported.
- **Don't duplicate the presentation-suppression condition list.** Call
  `UCk_LoadingScreen_Subsystem_UE::Get_IsPresentationSuppressed()`; two copies eventually disagree.

## See also

- BusterBlock `docs/superpowers/notes/2026-07-08-loading-screen-system-plan.md` — the full system
  plan this module is Phase 1 of (game-side holders, MapDefinition travel wrapper, preload).

## CkSnapshot holds the screen for the whole of a load

A snapshot load creates a `UCk_LoadingProcess_Task_UE` at `Request_Load` and releases it only at ready-to-resume —
after every payload has applied, the requests they issued have drained, physics has stepped and probe overlaps have
converged. Before that, the built-in show reasons all cleared once the fresh world had begun play with no pending
travel, so the screen dropped at the post-travel boot and the player watched the entire rebuild.

Three properties this leans on, all of them deliberate:

- **The holder is GameInstance-scoped**, outered to this subsystem, so it survives the level travel a load performs.
- **It passes `FCk_Time{}` — no wall-clock watchdog.** A load does not run at 60 fps: it runs a blocking `LoadMap`,
  package loads and PSO warm-up, so a timeout sized for a healthy frame rate fires on a healthy slow load and drops
  the screen over a half-rebuilt world, which is the one thing the screen is up for. The loader's own frame caps are
  the bound, and every one of them names itself when it fires.
- **The screen's own timers are wall-clocked** (`FPlatformTime::Seconds`), so the load's game-time freeze cannot
  stall the very thing the player is looking at. Those reads are on the wall-time allow-list for exactly this reason.

`Create` returns `nullptr` on a dedicated server, so an unset holder is a normal state rather than a missed one, and
the release path is idempotent and unconditional on every exit route the load can take.

The fade is **all holders released**, not "the loader finished": ready-to-resume is a statement about the WORLD, and
a project is free to keep its own gates (BB holds one for store readiness) up afterwards.
