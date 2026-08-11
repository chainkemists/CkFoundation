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
