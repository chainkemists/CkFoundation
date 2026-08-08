# Phase 0 — Research findings

> **Written:** 2026-08-07. **Status:** 🟡 In progress — 0A and 0F are blocked on a human.
> **This doc dies when:** Phase 1 is signed off; its conclusions move into `PROMPT.md` decisions and
> `Source/CkIntent/Claude.md`.

Every claim is labelled **VERIFIED** (I read it — file:line given), **INFERRED** (reasoning, with
what would confirm it), or **UNKNOWN** (needs a spike).

---

## Headline: two more collisions with shipping code

Both are the same class as B1/B2 — the design invents something CkFoundation already owns.

### ~~N6~~ — DEAD, superseded by D14 / DESIGN_InputLayering.md

> ⚠ **DO NOT IMPLEMENT N6.** The finding below (CkUI owns input suspension) is factually correct and
> worth knowing, but its *recommendation* — that CkIntent observe CkUI's suspension — was rejected by
> the maintainer in favour of layered consumption. A UI layer with a catch-all `Consume` capture is
> suspension, structurally. CkUI keeps `SuspendInput` for its own CommonUI filtering, and is intended
> to become a layer on this stack later. Kept for history only.
>
> The one item that survived N6's death and **has no owner**: PIE focus gating (a preprocessor
> otherwise eats editor typing). Assigned to the Slate writer in Phase 1.

### N6 (historical) — `CkUI` already owns input suspension

**VERIFIED.** `UCk_Utils_UI_UE::SuspendInput(const APlayerController*, FName InReason)` and the
`ULocalPlayer` overload return a token `FCk_Handle_InputSuspension`, released by `ResumeInput`
(`CkUI_Utils.h:161-178`). It is refcounted (`InputSuspensionCounter`, `CkUI_Utils.cpp:37`), backed by
`UCk_UI_Subsystem` holding `TMap<uint32, FCk_Handle_InputSuspension> _ActiveSuspensions`
(`CkUI_Subsystem.h:57-125`), and already consumed by `CkUI_PrimaryGameLayout`'s
`_TransitionSuspensionHandles` (`:226`). Its own doc comment: *"Suspends all input for a player
**using CommonUI's input filtering**."*

The design proposes `ECk_Input_CapturePolicy { Always, GameViewportFocusOnly, Suppressed }` as
CkIntent's own runtime-settable gate. Shipping that as-authored creates a **second answer to "is
input suppressed right now"** — the same two-sources-of-truth failure that killed the binding profile
(B2).

**But this is an integration requirement, not a deletion.** A Slate `IInputProcessor` fires
regardless of CommonUI filtering — that is precisely why the design reaches for a preprocessor in the
first place. So CkUI's existing suspension will **not** suppress CkIntent automatically.

→ **Proposed resolution (needs maintainer sign-off):** CkIntent does not own a suppression policy.
It *observes* CkUI's suspension state for the local player and treats "suspended" as its
`Suppressed` state, keeping only the focus dimension (`Always` vs `GameViewportFocusOnly`) as its
own. One reason-tagged, refcounted authority; CkIntent is a consumer. **INFERRED** that CkUI's
suspension state is observable from outside CkUI — the subsystem holds the map but I did not confirm
a public query or change signal. **Would confirm:** a `Get_`/`BindTo_` on `UCk_UI_Subsystem`; if
absent, adding one is a small CkUI change and should be scoped into Phase 1.

### N7 (new, blocking) — there are **four** "intent" tag namespaces, not two

**VERIFIED**, by census of the two `DefaultGameplayTags.ini` files:

| Namespace | Meaning | Where | Example |
|---|---|---|---|
| `ByteAttribute.Intent.*` | Player input intent | BusterBlock | `ByteAttribute.Intent.Attack` (`:750`) |
| `Bb.NpcIntent.*` | **NPC behavioural goal** (~17 tags) | BusterBlock | `Bb.NpcIntent.Shop.Attracted`, `Bb.NpcIntent.Employee.ClockIn` (`:304-320`) |
| `InteractionIntent.*` | Resolver channel intent | CkPlugins | `InteractionIntent.InteractionGym.Use` (`:755`) |
| `Intent.*` | Proposed for CkIntent | — | `Intent.Attack.Heavy` |

D13 was written believing consolidation meant reconciling **two** concepts. It is four, and
`Bb.NpcIntent.*` is genuinely a *different* concept — an AI goal ("this NPC wants to shop"), not an
input pattern and not a resolver channel. Those must not be merged.

→ **Proposed resolution (needs maintainer sign-off):** D13's "shared `Intent.*` namespace" narrows
to *player-driven input intents only*. `Bb.NpcIntent.*` is explicitly out of scope and should stay
distinct — arguably it deserves a rename to `Bb.NpcGoal.*` downstream, but that is BusterBlock's
call, not this campaign's. Two of the four namespaces are named for mechanisms rather than meanings
(`ByteAttribute.*` names a carrier D5 removes; `InteractionIntent.*` predates the resolver's current
shape). **Recommend:** propose the `ByteAttribute.Intent.* → Intent.*` rename to BusterBlock as a
follow-up, and leave `InteractionIntent.*` alone per D13's no-rename rule.

---

## 0B — Preprocessor coexistence

**VERIFIED.** Three registrations ship, and two collide at the same index:

| Preprocessor | Priority | Consumption |
|---|---|---|
| `CkLoadingScreen_Subsystem` | **0**, explicit (`:657`) | Eats **everything** while active — all nine handlers `return Get_CanEatInput()` (`:81-89`) |
| `CkDebuggerModel_ViewportPicker` | **0**, explicit (`:154`) | Selective; `HandleKeyDownEvent` returns `true` (`:140`) |
| `CkDebugOverlay_Subsystem` | **default** — no index argument passed (`:357`) | Not audited |

Two at index 0 means **registration order decides** between them, and registration order is
subsystem-init order — not something either module controls. This is a latent bug independent of
CkIntent.

→ **Proposed policy:** CkIntent registers at an index **below** the loading screen (so a loading
screen legitimately starves gameplay input) but **above** the debugger preprocessors (so the input
debugger is not starved by the entity-overlay picker while diagnosing input). Exact index to be set
in Phase 1 once the overlay's default is read. PIE default for the focus dimension:
`GameViewportFocusOnly` — a preprocessor otherwise sees the editor's own typing.

**Flagged, not chased:** the two-at-index-0 collision is a pre-existing defect worth a one-line
follow-up to the debugger owner.

## 0D — Local player model (O2/O3)

**VERIFIED: no local-player ↔ entity association exists.** The natural hook does exist and is
empty — `UCk_LocalPlayer_UE : public ULocalPlayer` (`CkCore/Engine/CkLocalPlayer.h:12`) is a bare
`UCLASS` containing only `CK_GENERATED_BODY`. `UCk_KeyBinding_Subsystem : ULocalPlayerSubsystem`
(`CkInput`) is the closest working precedent for per-local-player state.

→ **Proposed shape (fork — needs maintainer decision per non-negotiable #6):** a
`ULocalPlayerSubsystem` in CkIntent owning one input-source entity per local player, mirroring
`UCk_KeyBinding_Subsystem`'s lifetime. Possession change and pawn respawn then do **not** affect the
input source — it is bound to the *player*, not the pawn, and consumers resolve pawn→intent through
their own handle. **INFERRED** that this is the right cut; **would confirm:** a Phase 1 test where a
possession swap mid-hold leaves the `Holding` phase intact.

## 0G — The neighbours to mimic

**VERIFIED** these exist and are the exemplars named in `PROMPT.md`:

- **Feature quartet:** `CkTimer` — `CkTimer_{Fragment_Data,Fragment,Processor,Utils}.h/.cpp` under
  `Source/CkTimer/Public/CkTimer/`.
- **Processor tick shape:** `TProcessorBase` (`CkProcessor.h`) — see 0H.
- **Signal shape:** `CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE` (`CkSignal_Macros.h:37`).
- **Per-local-player subsystem:** `UCk_KeyBinding_Subsystem` (`CkInput`).
- **Debugger widget:** `SCkDebug_EventTimeline` (`CkDebuggerCommon/Widgets/`) — shared `SLeafWidget`
  with `LaneLabels`, `FCkDebug_TimelineEvent`, `FCkDebug_TimelineSpan`, `OnEventSelected`; consumed
  by `CkGoapDebugger`. `CkSmDebugger` has its own separate one — extend the **common** widget.

## 0H — The fixed-timestep gap, precisely

**VERIFIED** by reading `TProcessorBase::Tick` (`CkProcessor.h:243-294`). Three branches:

```cpp
// TickRate == Zero  -> DoTick(InDeltaT) once, every tick
// SampleLatestOnly  -> drains the accumulator, then ONE DoTick(FiredElapsed)
// ReplayMissedTicks -> while (Adjusted >= TickRate) { Adjusted -= TickRate; DoTick(TickRate); }
```

The replay branch is a **bare unbounded `while`**. At `Hz(60)`, a 1-second hitch runs **60**
`DoTick` calls in one frame — and under D6 each one can fire intent signals, so consumers receive a
60-step burst. `_RemainingDeltaTFromLastFrame = AdjustedTickRate` correctly carries the remainder
(`:293`).

`SampleLatestOnly` is confirmed **wrong for input**: it collapses the whole backlog into a single
`DoTick`, so every press inside the hitch is never sampled.

→ **N5 confirmed as a required deliverable.** Proposed shape: an optional
`static constexpr int32 MaxReplayedTicks` trait beside `TickCatchUpPolicy`, defaulting to unlimited
so no existing processor changes behaviour; when set, the loop drains the excess without ticking and
logs once. **This touches the shared CkEcs base** — it needs its own review note, and every existing
`ReplayMissedTicks` processor is unaffected only because the default preserves today's semantics.

## 0I — Namespace and boundary

Namespace: see **N7** above.

**Continuous look/move axes (O9): recommend OUT of scope.** **VERIFIED** they ride
`VectorAttribute_Intent_LookDirection` / `MoveDirection` downstream. They have no phases, no
ambiguity, no matching, and no deferral — every mechanism CkIntent exists to provide is inert for a
continuous axis. Forcing them through the intent grammar would add a second, degenerate code path.
CkIntent's frame record already carries a discrete SOCD-cleaned direction for *matching*; raw
continuous axes should keep their existing carrier.
→ **Needs maintainer sign-off**, since it leaves "intent" meaning two things downstream.

---

## Still blocked on a human

| Item | Why | What is needed |
|---|---|---|
| **0A — hardware spike** | Slate analog cadence, `UserIndex` fidelity, sub-frame arrival order are **UNKNOWN** and unverifiable from this repo | A throwaway preprocessor build + a scripted physical input sequence on a DualSense + keyboard. Phases 1–3 depend on the answers. |
| **0F — CkGameSettings boundary** | Cross-campaign agreement | Confirmation from that campaign's owner that CkIntent reading the resolved key map and re-deriving on `OnSettingsChanged` introduces no conflict. |
| **0C / 0E** | Write-ups, not research | Draftable once 0A's answers constrain the source interface. |

## ~~Open decisions this phase surfaced~~ — ALL SETTLED 2026-08-08

> ⚠ **This list is stale and is kept only to show how each was resolved.** `PROMPT.md`'s decision
> table is authoritative. Do not re-ask the maintainer any of these.

| Was | Resolved as |
|---|---|
| N6 — observe CkUI's suspension | **Rejected.** Layered consumption instead → D14, D24, D25b |
| N7 — narrow the shared namespace, exclude `Bb.NpcIntent.*` | **Accepted** → D13 as amended |
| O9 — continuous look/move axes out of scope | **Rejected as framed.** In scope for acquisition/record, out of the intent grammar → D18 |
| 0D — local-player subsystem owning an input-source entity | **Accepted, with constraints**: not a singleton, and any entity may bind raw inputs → D16, D22 |
| N5 — `MaxReplayedTicks` on shared `TProcessorBase` | **Accepted**, default unlimited, needs its own review note → D19 |
