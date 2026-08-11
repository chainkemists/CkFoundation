# PHASE 11 — Device visualizers (keyboard + gamepad) for CkIntentDebugger

> Opened 2026-08-10 on the maintainer's "proceed" after the design conversation (this
> phase's rulings quote it). Work lives in **CkGameplayDebugger** (dev, off `a71d10e` —
> the qol campaign's CkIntentDebugger landed there: timeline, layer stack, key state,
> resolution, near misses). Gate: this phase writes C++ — `--build --test`, pattern
> `Ck_AutoTest_In`, 123/123 by name, plus a clean plugin build. [P2-D4] holds (no full
> suite until campaign end).

## Goal

The debugger shows the player's physical devices: a **full-size keyboard** and a
**realistic gamepad** (greyed out when not connected). Presses are **instant flashes**;
holds are a **fill on the key**; analog controls read analog (trigger fill, stick dot).

## Standing constraint (maintainer, 2026-08-10)

**"These 'devices' will need to support overlaying in the game in the near future."**
The widgets must be loadable in a cooked game build — nothing about them may live in an
UncookedOnly module or reach into debugger-privileged reads.

## Rulings

- **[P11-D1] Widget home = `CkDebuggerCommon` (Runtime), data-decoupled.** The plugin
  already ships the pattern twice: `CkEntityDebugOverlay` is a Runtime in-game overlay
  module, and `CkDebuggerCommon` is the shared-widget home every debugger consumes
  (`SCkDebug_EventTimeline` precedent). The device widgets are pure presentation
  consuming a plain snapshot struct (`FCkDebug_DeviceSnapshot`: per-key press edge /
  held-run fraction toward the hold threshold / analog values, plus device-connected).
  Data acquisition stays in the CONSUMER: `CkIntentDebugger` (UncookedOnly) feeds it
  from fragments under its [P7-D4] privilege; the near-future game HUD feeds it from
  public reads. The Runtime widget code never touches CkIntent fragments — that is what
  keeps the overlay constraint and the [P7-D4] privilege from colliding.
- **[P11-D2] Rendering split.** Keyboard = PROCEDURAL Slate (static ANSI 104 layout
  table — FKey → row/col/width in key-units — painted as labeled rounded rects; crisp
  at any DPI, restyles with the debugger theme, zero assets). Gamepad = LAYERED ART:
  hand-authored SVG masters committed as source of truth (full SVG — gradients, soft
  shading), rasterized offline via ImageMagick to multi-scale PNGs shipped as the Slate
  brushes (UE's runtime SVG brush supports only a flat-fill subset — feeding it SVG
  directly would flatten the realism; ruled with the maintainer: "do the better
  version"). One brush layer per control so each flashes/fills/greys independently.
  States are NEVER baked into art — one image set, every state composited live.
- **[P11-D3] One state vocabulary for both devices.** Press = flash (tint snaps to
  accent on the press row, ~200ms decay so a one-frame press is visible — the gym's
  re-issued-beat trick). Hold = fill growing toward the HOLD-VERDICT threshold — a key
  that fills completely is the matcher declaring a hold, which makes the visualizer a
  verdict debugger, not a key monitor. Analog: triggers fill by axis value, stick caps
  translate in their wells (octant + deadzone made visible). Disconnected device =
  whole stack desaturated. Data drawn from what the MODULE ingested (record/sampler),
  never raw OS state — "dead key" vs "dead pipeline" stays diagnosable.

## Slices

- **11-1 — keyboard + state pipeline.** `FCkDebug_DeviceSnapshot` + flash/fill state
  helpers + `SCkDebug_DeviceKeyboard` in CkDebuggerCommon; a Devices panel in
  CkIntentDebugger fed from the existing ViewModel/DataCollector. Proves the pipeline
  end to end with zero asset risk. Gate + maintainer PIE.
- **11-2 — gamepad.** SVG masters + rasterization + layered brushes +
  `SCkDebug_DeviceGamepad` (ABXY, bumpers, triggers, D-pad, sticks, start/select),
  analog treatment, connected/greyed state. Gate + maintainer PIE.

## Rulings (implementation)

- **[P11-D4] Two-tier key fidelity, declared not hidden.** The record ring carries only MINTED
  keys, so only they can render exactly at any refresh cadence. Unminted keys (most of the
  keyboard) are witnessed through `Get_RoutedEventsThisFrame` — a per-pass array read at the
  refresh-gate cadence, so an edge landing between collector passes is missed. The panel says
  so in a caption; minted keys get the brighter bezel so the fidelity difference is visible,
  not secret. Held state self-heals (IsDown persists across passes). If the maintainer wants
  exact unminted edges later, the fix is an ungated cheap edge-capture tick — deliberately
  not built until asked.

## Rulings (round 5 — maintainer PIE of slice 11-1, 2026-08-10)

- **[P11-D5] No tabs.** "There is a lot of available space... fewer (or none) tabs" → NONE:
  the right side is now the timeline over two splitter rows — { Key/State | Devices } over
  { Resolution table | Near misses } — every view visible at once, each with its own section
  header. The underline-tab strip, `_DetailSwitcher` and `_ActiveTabId` are retired. The
  near-miss tab's count badge died with the tab; the panel's own banner + list carry it.
- **[P11-D6] Every device is always on screen, greyed when unavailable (maintainer).** The
  mouse is added beside the keyboard (procedural `SCkDebug_DeviceMouse`: L/R buttons, wheel
  with scroll nubs, two thumb buttons — same snapshot, same state vocabulary). The gamepad
  joins in slice 11-2 under the same rule (drawn always, `DeviceConnected=false` dims it).
- **[P11-D7] Edge capture is UNGATED — supersedes [P11-D4]'s deferral (maintainer hit the
  defect immediately).** Root cause of the stuck keys: unminted keys light via witnessed
  edges read from the router's per-pass array at the refresh-gate cadence; a RELEASE landing
  between collector passes was never seen, so `IsDown` latched forever (minted keys cannot
  stick — ring-derived; the stuck `D` was locomotion, unminted — mechanism fits the "some
  keys" observation exactly). Fix: `Tick_WitnessDeviceEdges()` runs every widget frame
  BEFORE the refresh gate — edge capture is truth, not presentation, so it may not be
  rate-limited; it reads two public values per source and stores plain values. Residual
  honesty: sub-frame taps coalesce; multiple world ticks per rendered frame can still hide
  an edge pair (declared in the panel caption).

## Rulings (round 7 — maintainer PIE of round 6, 2026-08-11)

- **[P11-D9] Losing viewport focus flushes recorded-down keys (CkInput production fix).** The
  maintainer's alt-tab stuck key ("that key is depressed until I 'reset' it by tapping it") is the
  Slate writer's own focus gate: `DoRecordEvent` drops EVERY event without viewport focus, so a
  release that happens after alt-tab (or after clicking any editor panel) never records — the whole
  pipeline holds the phantom press, not just the debugger. Fix at the writer: it tracks each
  (key, raw user index) it recorded a `Pressed` for, and its Slate `Tick` writes synthetic
  `Released` rows the moment the viewport is unfocused while any are outstanding — the pipeline's
  `FlushPressedKeys`. The gate/write split (`DoRecordEvent` gates, `DoWriteEvent` writes) exists so
  the flush can write at exactly the moment the gate would drop. Declared consequence: a hold does
  not survive a focus gap (the OS resends no edge on refocus; auto-repeat is dropped) — re-press to
  re-engage, which is the engine-standard semantics. Analog axes are not flushed. Untestable in the
  harness for the same reason as the double-click fix: autotests inject below Slate. [EDITOR-VERIFY].

## Rulings (round 8 — maintainer PIE of round 7, 2026-08-11)

- **[P11-D10] Timeline scrub/zoom = extend the shared `SCkDebug_EventTimeline`, not swap to
  `SCkDebug_ScrubTimeline`.** The maintainer named the StateMachine timeline as the reference;
  its interaction already lives in CkDebuggerCommon's `SCkDebug_ScrubTimeline` (the merged
  SM+GOAP scrub track) — but that widget is a single segment track, and the intent timeline is
  multi-LANE. So the multi-lane widget adopts the scrub track's interaction contract instead:
  widget-owned view window, wheel zoom about the cursor, RMB/MMB/Ctrl+LMB drag pan, LMB drag
  scrub (markers still click-select first), F re-attaches the live-following full view. All of
  it OPT-IN (`AllowPanZoom`, `OnScrubbed`) so the GOAP consumer is untouched. The label gutter
  is now MEASURED from the lane labels (was a fixed 56px — long intent names sat under the
  early-frame pips, the round-8 screenshot), with a boundary line, and out-of-view markers are
  culled rather than edge-clamped. A frame-axis consumer supplies `OnFormatTick` ("f1234" —
  the old hardcoded "s" suffix lied on a frame axis). The dock carries pan/zoom across
  lane-set rebuilds via `Get_ViewStart/Get_ViewDuration` + `Set_View` AFTER the first
  `Set_Content` (before it, the fresh widget's 0..1 range clamps the view away).

## Rulings (round 10 — maintainer PIE of round 9, 2026-08-11)

- **[P11-D11] The Slate writer's focus gate is DIRECT-only (third CkInput production fix).** The
  maintainer's "multiple keys appeared to have been pressed (but I never pressed so many keys)"
  screenshot lit exactly S·L·O·M·O·Space·0·`.`·1·Enter + the console key — their own `slomo 0.1`
  console command. The console's input box is a DESCENDANT of the game viewport widget, so
  `HasAnyUserFocusOrFocusedDescendants` recorded console typing into the game's input pipeline.
  Gate is now `HasAnyUserFocus()` (the viewport itself): console/chat/any focused text field
  pauses recording, and the [P11-D9] flush releases whatever was held at that boundary, so the
  strictness cannot strand a phantom press. Declared consequence: recording also pauses while any
  UMG widget holds keyboard focus — correct for the same reason the console case is.

## Rulings (round 12 — the instrumented repro's verdict, 2026-08-11)

- **[P11-D12] The collector never caps the frame pull — the ring is the one history knob.** The
  r10.1 on-screen readout (`data 3251-3491`, exactly 240 rows; `pan#441` proving input arrived
  and applied) exposed the true root cause of every park/pan round since round 9: the ViewModel
  passed `MaxRecordedFrames = 240` into the collector — a debugger-side cap written when the gym
  ring was also 240 (cap == ring made it unobservable). Raising the ring to 1800 (11-1h) changed
  nothing the debugger could see: its whole world stayed 240 frames, so any ≥240-frame view
  window was PINNED to the data range, the full range slid at live rate every frame ("parked"
  yet moving), and there was nothing outside it to pan to. Every widget-side fix in 11-1i was
  correct but could not surface — the SM timeline "works perfectly" precisely because it has no
  collector cap. The cap is DELETED (the pull is bounded by `Get_FrameCount`, i.e. the ring
  itself); a game tunes history via `_RingCapacity`, one knob, owned by the feature. The r10.1
  diagnostic readout stays until the maintainer confirms park/pan in PIE, then comes out.

## Rulings (round 13 — maintainer confirmed park/pan working, 2026-08-11)

- **[P11-D14] History is the DEBUGGER'S OWN RECORDING, sized by one number in the UI —
  supersedes [P11-D13] (the `ck.Intent.RingCapacityFloor` CVar, removed in the same change per
  the no-backcompat rule; it never shipped past a local gate).** The maintainer rejected the
  floor design's complexity ("why can't I just write a number of how much history I want?") and
  the simpler design is also the CORRECT one — CkIntent anti-pattern 5 prescribes exactly this
  consumer shape: "a consumer that needs more history should record rows out of the ring as
  they arrive." The ViewModel now appends each refresh's new rows (by frame index) to a
  per-source history capped at `_FrameHistoryCap` (default 1800, min 120), and the merged
  history REPLACES the snapshot's frames so every downstream reader (timeline, devices,
  key/state, scrub) sees deep history without knowing it exists. The timeline header's
  "history" `SCkDebug_NumericEditor` sets the cap — effective IMMEDIATELY, no ring resize, no
  next-PIE caveat; the honest limit (tooltip): recording starts when the window opens. A
  backwards frame index restarts the recording (same-world sampler recomposition); world
  changes clear it wholesale. Gym ring reverted to its honest 240-frame working window.

## Rulings (round 14 — history as a debug FRAGMENT, 2026-08-11)

- **[P11-D15] History is an ECS debug fragment, compiled out in Shipping — supersedes [P11-D14]
  (the ViewModel-side recording, deleted before it ever gated).** The maintainer named the house
  pattern ("that's how we've been adding debug to other features... a fragment on the entity,
  consumed by the debugger... compiled out in shipping builds") and it is strictly better than
  Slate-side recording: a processor captures every frame whether or not any debugger window is
  open, and the same public read surface serves the future in-game overlay. New CkIntent feature
  quartet `CkIntentHistory` (mimics the sampler): `FCk_Handle_IntentHistory`, ParamsData
  (capacity, default 1800), `FFragment_IntentHistory_Current` (flat append-and-trim rows + dedup
  cursor), `FProcessor_IntentHistory_Record` (unrated, matcher's group, copies rows newer than
  its cursor), `UCk_Utils_IntentHistory_UE` (Add requires a sampler; ring-style
  `TryGet_FrameAtOffset`; `Request_SetCapacity` as a DECLARED immediate mutator). Shipping-gated
  per the CkStateMachine/Debug precedent: `#if !UE_BUILD_SHIPPING` on the recorder, reflected
  surface stays (UHT), Add/read stubs answer empty. The debugger's collector prefers history
  frames over the ring transparently; the timeline's "history" field retunes capacity through
  the ViewModel's one sanctioned mutation — a ruled carve-out to the module's read-only
  contract, recorded in CkIntentDebugger's CLAUDE.md. Gym composes it at 1800; ring stays 240.
  **Round-14b addendum:** the maintainer's pattern audit ("did you reference existing debug
  fragments?") caught a naming delta — every house debug fragment carries `Debug` in its
  symbols (`FFragment_AStar_Debug`, `FFragment_SmDebug_*`, …) and the SM suite lives in a
  `Debug/` subfolder. Renamed on their "do it": feature token is now **IntentDebugHistory**
  uniformly (`CkIntent/Debug/CkIntentDebugHistory_*`, `FCk_Handle_IntentDebugHistory`,
  `FProcessor_IntentDebugHistory_Record`, `UCk_Utils_IntentDebugHistory_UE`,
  `utils_intent_debug_history` in AS).

## Slice log

- **11-1 — keyboard + state pipeline (2026-08-10, orchestrator-inline): INSTALLED, gate in
  flight.** CkDebuggerCommon (Runtime): `Devices/CkDebug_DeviceTypes.h` (plain-value snapshot
  contract), `CkDebug_KeyboardLayout.{h,cpp}` (ANSI 104 in key units, decorative
  EKeys::Invalid caps for PrtSc/Menu), `SCkDebug_DeviceKeyboard.{h,cpp}` (one SLeafWidget
  paints every cap: rounded suite brush, press flash 15f decay, bottom-anchored verdict fill,
  minted bezel, disconnected dim — all timing in snapshot frames, widget owns no clock).
  CkIntentDebugger: types + collector gain `MintedKeys` (`Get_AllButtons`/`TryGet_KeyForButton`)
  and `RoutedKeyEvents` (public `Get_RoutedEventsThisFrame`, axis events skipped); ViewModel
  gains the witnessed-key map + `Get_KeyboardDeviceSnapshot()` rebuilt per Tick (hold-verdict
  thresholds joined from the selected layer's Resolutions rows); new
  `SCkIntentDebugger_DevicesPanel`; window gains the Devices underline tab (switcher index 3).
  Gate: `--build --test` pattern `Ck_AutoTest_In` (`BuildTest-Phase11Slice1.log`).

- **11-1b — round-5 rework (2026-08-10, orchestrator-inline): INSTALLED, gate
  `BuildTest-Phase11Slice1b.log`.** Per [P11-D5..D7]: tabless window layout (splitter
  quadrants + section headers), `SCkDebug_DeviceMouse` (new, CkDebuggerCommon), shared state
  helpers hoisted to `ck::debug_devices` in `CkDebug_DeviceTypes.h`, ungated
  `Tick_WitnessDeviceEdges` in the ViewModel (collector's `RoutedKeyEvents` retired with its
  gated witnessing — `MintedKeys` stays), snapshot renamed `Get_DeviceSnapshot` (one snapshot,
  every device), nav-cluster cap labels shortened (they collided at small units).

- **11-1c — round-6 layout/timeline rework (2026-08-10, orchestrator-inline): INSTALLED, gate
  `BuildTest-Phase11Round6.log`.** Maintainer's four findings: (1) panes not resizable — the
  vertical arrangement was SVerticalBox FillHeight, not splitters; now EVERY boundary is an
  SSplitter handle. (2) Timeline text scrunched — lane step was 16px against 12px micro-font
  labels; now 22px, min height 140, and the timeline dock FILLS its (resizable) splitter
  slot instead of auto-sizing. (3) The perpetual red bars = still-open phase spans of
  intents whose last transition was Failed, drawn to the live edge — honest data rendered as
  an eternal alarm; ruled **[P11-D8]**: terminal phases (Completed/Failed) clamp to a
  20-frame PULSE at the frame they happened (markers + tooltips keep the full truth), only
  in-flight phases draw open-to-live. (4) Wasted layer-stack space — maintainer's guess
  confirmed (the pane was sized for real-game stacks; the gym has 2 layers); Resolution
  table + Near misses moved UNDER the layer stack in the left column (vertical splitter),
  right column = timeline over { Key/State | Devices }. User-draggable pane DOCKING was
  considered and declined — it means an FTabManager rework; resizable splitters + this
  arrangement cover the need at a fraction of the surface.

- **11-1d — round-7 layout + focus-loss flush (2026-08-11, orchestrator-inline): INSTALLED, gate
  ✅ GREEN 123/123 (`BuildTest-Phase11Round7.log`, fresh build, compile first try).** Maintainer's directed layout: the three tables (Layer stack | Resolution table |
  Near misses) become a HORIZONTAL row on top; below it, timeline over { Key/State | Devices }
  takes the full window width — "timeline and key/state/devices take up the most horizontal room
  and they seem to need it". Outer splitter flipped to vertical; every boundary stays a splitter
  handle. Plus the [P11-D9] CkInput focus-loss flush (production fix, CkFoundation).

- **11-1e — round-8 timeline interaction + actionable outlines (2026-08-11,
  orchestrator-inline): INSTALLED, gate ✅ GREEN 123/123 (`BuildTest-Phase11Round8.log`,
  fresh build, compile first try).** Maintainer's four findings: (1) "keys that
  actually do something should be outlined" — `FCkDebug_DeviceKeyState.IsActionable` (minted ∪
  any layer's Key captures ∪ registered matcher keys; entries minted for never-pressed captured
  keys so the rim shows without history), drawn as an Accent rim (the badge brush one step
  larger under the cap — keeps the corner radius) on keyboard + mouse; captions updated.
  (2) Q/block invisible in the timeline — grammar-free presses have no intent lane by
  construction; NEW per-button lanes between the layer and intent lanes (held-run spans +
  press markers off the record ring, scrub-selectable). (3) scrub/zoom/pan per [P11-D10].
  (4) pips-over-text = fixed 56px gutter; now measured. Files: CkDebuggerCommon
  `CkDebug_DeviceTypes.h`, `SCkDebug_DeviceKeyboard.cpp`, `SCkDebug_DeviceMouse.cpp`,
  `SCkDebug_EventTimeline.{h,cpp}`; CkIntentDebugger ViewModel + TimelineDock + Window caption.

- **11-1f — round-8b "still unable to scrub" (2026-08-11, orchestrator-inline): INSTALLED,
  gated with 11-1g ✅ GREEN 123/123 (`BuildTest-Phase11Round8c-3.log`).** Three inspection-confirmed defects in the 11-1e scrub (cannot PIE-reproduce here;
  the first is the dominant mechanism): (1) **the timeline drew NO scrub cursor** — the model
  took the scrub (SCRUB @ label + Key/State panel followed), but the only timeline-visible
  feedback was a marker highlight on an EXACT frame match, which a drag almost never hits, so a
  working scrub rendered as nothing happening; new `CursorTime` attribute draws a full-height
  Warn cursor line (+ top cap) at the scrubbed frame. (2) any LMB press within the 12px pick
  radius of a marker became a selection click and never STARTED a drag — fatal once the button
  lanes densified the track; a marker click now lands on the marker's exact frame and then the
  same press continues as a scrub drag. (3) a lane-set rebuild mid-drag destroyed the widget
  holding mouse capture, killing the drag; the dock now defers rebuilds while
  `Get_IsInteracting()` (hash stays stale, rebuild lands after the drag ends).

- **11-1g — round-8c, maintainer's discriminating observation (2026-08-11): INSTALLED,
  gated with 11-1f ✅ GREEN 123/123 (`BuildTest-Phase11Round8c-3.log`).** "The timeline stops moving [while holding right-click] … pan does nothing … on
  release it starts moving again" — two mechanisms, both fixed: (1) a held mouse button on an
  editor widget engages Slate's responsive-UI THROTTLE, which pauses PIE itself — the freeze
  was the throttle, and an input debugger pausing the very input it measures is doubly wrong;
  both drag replies now carry `PreventThrottling()` (the editor-slider idiom). (2) pan "did
  nothing" because the view defaulted to the FULL data range — a structural no-op, nothing
  outside it to pan to; the SM track never feels this because it opens with a 10s window. New
  `InitialViewDuration` argument; the dock opens with a live-following 600-frame (~10s) window,
  so right-drag immediately pans into ring history. GATE-INFRA NOTE (root-caused after FOUR
  exit-255 kills at four random stages — editor boot ×2, mid-compile, UBA start): SIBLING
  toolbox sessions were cycling gates concurrently on the `_Other` worktree lane
  (`E:\Repos\CkPlugins_Other` build + `E:\Repos\BusterBlock_Other` Gauntlet, own engine
  tree) from ~07:26 — exactly when this session's greens turned to kills. Concurrent
  toolbox activity on one machine is the known false-red machinery; not a code failure
  (each attempt's compile/discovery was green as far as it ran). Resolution: wait for the
  sibling lane to drain, then regate — never kill its processes.

- **11-1h — round-9 scrub-while-live + scrubbed devices + deeper ring (2026-08-11,
  orchestrator-inline): INSTALLED, gate ✅ GREEN 123/123 (`BuildTest-Phase11Round9.log`).** Maintainer's three findings: (1) "can only
  scrub when paused" — starting a scrub never DETACHED the live-following window, so the view
  slid out from under the drag re-mapping the cursor every frame (pausing the game is what
  accidentally made it usable); a scrub drag now PARKS the window (`_FollowLive = false`;
  data keeps accumulating, only the window stands still) and "Go live" re-glues it via new
  `Refollow_Live()`. (2) "history is not too long" — the pan wall is the gym sampler's ring:
  `k_RingCapacity` 240 (~4s) → 1800 (~30s) in `CkPlaygroundGym_Shared.as` (the debugger
  renders only what the module retains; games wanting deep history raise their own ring).
  (3) scrubbing now moves the DEVICES too: the snapshot builds as-of the displayed frame
  (minted keys walk the ring to that frame exactly; flash/fill age against it; witnessed
  unminted keys are best-effort off their latest edge pair — the declared tier difference).
  The Key/State panel already followed the scrub via `TryGet_DisplayedFrame`.

- **11-1i — round-10 park/pan really-fixed + console-typing gate (2026-08-11,
  orchestrator-inline): INSTALLED, gate ✅ GREEN 123/123 (`BuildTest-Phase11Round10-2.log`,
  incl. the round-10b reference-alignment addendum below).** "Drag does NOT park / I still cannot pan"
  root-caused to TWO compounding view-state bugs in the 11-1g/h work: (1) follow was RE-DERIVED
  from geometry after every rebuild carryover and every Do_ApplyView — a freshly parked window
  still touching the live edge re-attached on the first lane-set rebuild (seconds after the
  drag); follow is now carried as explicit STATE (`Get_IsFollowingLive`/`Set_FollowLive`) and
  the dock enforces scrubbed⇒parked every refresh. (2) the duration-0 "full view" collapse was
  a TRAP: any zoom-out to (or past) the data range — or ANY rebuild while the ring was younger
  than the 600-frame window — collapsed the view into a state where pan and park are hardcoded
  no-ops, permanently; the collapse is removed (duration = requested window, effective view
  caps at the data edges; F now applies a real full-range window). Plus [P11-D11] (see
  rulings). Files: `SCkDebug_EventTimeline.{h,cpp}`, `SCkIntentDebugger_TimelineDock.cpp`,
  CkInput `CkInputSlate_Preprocessor.{h,cpp}` + `CLAUDE.md`.
  **Round-10b addendum — the maintainer asked "did you look at the StateMachine timeline?
  we solved all these issues before": correct call, and the process failure is recorded —
  the ScrubTimeline's HEADER was read but its IMPLEMENTATION was not; the mechanics were
  re-invented and re-broke solved problems (its `OnMouseButtonDown` even carries the
  freeze-on-scrub comment verbatim for the round-10 park bug). Alignment pass applied:
  `Do_ApplyView` never derives follow (only `Set_View` does, once — owners override);
  pan/zoom explicitly detach follow; scrub gained the reference's 5% edge auto-pan.
  Reference lacks `PreventThrottling` (the SM timeline pauses PIE mid-drag too) — kept as
  this widget's improvement. Also fixed own C2440 from the first round-10 gate:
  `HasAnyUserFocus()` returns `TOptional<EFocusCause>`, needs `.IsSet()`.**

- **11-2 groundwork (same session): gamepad master art AUTHORED, awaiting maintainer art
  verdict.** `Source/CkDebuggerCommon/Resources/Devices/Gamepad_Master.svg` — stylized-real
  Xbox-class layout, per-control `<g id="layer_*">` groups (body, LT/RT, LB/RB, stick wells +
  caps, D-pad base + 4 arms, ABXY, guide/view/menu), gradient-shaded, verified by
  ImageMagick rasterization (renders correctly incl. gradients). Remaining once the art is
  approved: per-layer split + committed rasterization script → multi-scale PNGs, brush
  registration, `SCkDebug_DeviceGamepad` (flash/fill overlays, stick-cap translation, analog
  trigger fills), collector analog witness (AnalogAxis routed events), connected-state feed.

## NOT in this phase

The in-game overlay HOST (game HUD wiring, input toggles, cook settings) — the widgets
are built overlay-ready; the host is the near-future workstream that consumes them.
No CkIntent/CkInput production changes. Push never (ship conversation).
