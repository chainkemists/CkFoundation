# CkIntent — mission brief (PROMPT.md)

> **Written:** 2026-08-07. STABLE content only — current state lives in [PROGRESS.md](PROGRESS.md).
> **This doc dies when:** the campaign ships and `Source/CkIntent/Claude.md` carries the permanent contract.
> On death: delete it, or replace the body with one tombstone line ("Superseded by X — kept for history").
>
> **Supersedes:** the original `CkInput — Input & Intent System` PROMPT (2026-08-07, uncommitted).
> **Does not change:** the requirement itself. MK / Sekiro / Souls-grade intent remains in full scope.
> What changed is *where the system sits in this repo* — the original was written without the repo
> open and collided with three shipping systems. See "Things ruled out" for the full list.

---

## Goal

A frame-deterministic intent system in CkFoundation that turns raw device input into named,
frame-indexed **intents** — expressive enough for fighting-game motion inputs, soulslike tap/hold
differentiation, and timing-sensitive deflect/charge windows — from one authoring grammar, with
zero added latency on inputs that are not genuinely ambiguous.

CkIntent owns **acquisition, sampling, matching, and arbitration**. It does *not* own key binding:
Enhanced Input's `UEnhancedInputUserSettings` remains the binding and rebinding store, and CkIntent
derives its `FKey → ButtonId` map from the player's resolved mappings. A developer authors intents
and consumes them; they never hand-author an `UInputMappingContext`, never write a
`SetupPlayerInputComponent`, and never touch a `PlayerController`.

## Success criteria

Each is an observation, not an activity.

1. In `Gym_Input_Fighting`, a quarter-circle-forward + Punch on a gamepad drives its intent to
   `Completed` **on the same logic frame the Punch is pressed** — the on-screen counter reads 0
   frames of deferral. `[EDITOR-VERIFY]`
2. An AutoTest asserts a button with no hold sibling and no chord membership emits its terminal
   phase on the press frame, and **fails loudly** if a later intent addition regresses it.
3. Tap-vs-hold on one shared button resolves correctly for release at threshold−1, threshold, and
   threshold+1 frames (AutoTest, all three cases).
4. Rebinding a key through the CkGameSettings keybinding page changes which physical key drives an
   intent on the next sample, with **no edit to any intent definition**. `[EDITOR-VERIFY]`
5. The debugger timeline scrubs back to a failed quarter-circle and shows which step timed out and
   by how many frames. `[EDITOR-VERIFY]`
6. A ~40-move set authored in AngelScript compact notation bakes with **no hand-written per-move
   struct construction** in the `.as` source.
7. Every public API is exercised from C++, Blueprint, and AngelScript (non-negotiable #4).
8. Full CkTests suite is delta-zero against the baseline captured at Phase 0 entry.

## Constraints & locked decisions

| # | Decision | Why |
|---|---|---|
| D1 | **Two modules, not one** (revised 2026-08-08). **`CkInput`** — the existing module, extended — owns the *raw* layer: Slate acquisition, per-game **input biasing**, device→local-player ownership, and the EI binding store / rebinding / glyphs it already ships. **`CkIntent`** — new — owns sampling, frame records, grammar, matching, arbitration, and intent output. Data flows `Unreal → CkInput → CkIntent`. | Maintainer 2026-08-08: *"I wanted our own CkInput which takes the raw input from Unreal and then gives the developers control, per game, on how to bias the raw input before it goes to CkIntent."* This also resolves the D1 name collision properly — CkInput keeps its name and gains the raw layer, rather than being worked around. |
| D20 | **Input biasing is a first-class, per-game stage in CkInput**, between raw acquisition and CkIntent: deadzone, response curve, sensitivity, inversion, filtering. CkIntent consumes *conditioned* input and does not re-condition it. | The game, not the framework, decides how a stick feels. It also means CkIntent's octant/hysteresis mapping has one well-defined input contract instead of guessing what the platform already applied. |
| D21 | **REVISED for D25a.** Input capabilities — sub-frame press ordering among them — are declared **per device class**, carried as event provenance, never as one flag on the input source. Order-sensitive grammar steps consult the provenance of the *specific events in the chord* and degrade to `Simultaneous` with a diagnostic when unsupported. | Maintainer: ordering "should be an option… it's possible that later we encounter a gamepad-like-controller that _has_ sub-frame ordering." **A single field would lie**: D22 puts keyboard and gamepad on the *same* local player, and they differ (message queue vs polled bitmask). |
| D25 | **ECS Feature architecture (data / fragment / processor / utils) is the default shape for everything in both modules.** UObjects, UInterfaces, and subsystems are permitted **only** where the engine gives no other reasonable seam — not avoided dogmatically, but never used where a fragment would do. **VERIFIED precedent:** only 7 `UINTERFACE` declarations exist across 101 CkFoundation modules, all at genuine UObject-world bridges. | Maintainer 2026-08-08. The test is not "is this pure ECS" but "would forcing this into ECS be a contortion". Two places in this design failed that test and are revised below. |
| D25a | **`ICk_Input_Source` is deleted.** There is no source interface. Producers write raw events into a fragment on the input-source entity; the sampler reads that fragment and never knows who wrote it. The Slate `IInputProcessor` stays a Slate interface (engine contract — genuine "no other choice") but becomes a thin writer. Capability declaration (D21) becomes a fragment field, not an interface method. | Swappability and testability fall out of the fragment for free; the synthetic source is just "something else writes the same fragment". An interface bought nothing ECS did not already provide. |
| D25b | **Layer captures are declarative — no handler callback, no `handled` return value.** Propagation walks layers and the first matching `Consume` capture wins. A layer whose *state* decides whether it consumes does so by **adding/removing captures**, not by declining at dispatch time. | ftxc needs its bool because a widget's internal state decides during dispatch; ECS models that as which captures exist. This also **supersedes D17 entirely** — the "`Consume` must return `false` when unhandled" footgun ftxc documents cannot exist when there is no return value. Effect delivery becomes a signal broadcast, which is the house pattern and needs no return. |
| D23 | **`Gym_Input_KeyBinding` is a first-class deliverable and lands FIRST — before any new CkInput code.** Exercises `UCk_Utils_KeyBinding_UE` (remap, conflicts, swap, reset, persist) and `UCk_Utils_KeyIcon_UE` (glyph resolution, device hot-swap) interactively. | Maintainer 2026-08-08. **VERIFIED: that surface has zero test and zero gym coverage today**, and CkGameSettings' keybinding page is being built on it. Building the gym first turns it into a regression net for extending CkInput, not just a campaign deliverable. Supersedes the earlier review's cut of a rebinding gym. |
| D24 | The **layer stack primitive lives in `CkInput`** (the lower module); `CkIntent` registers intent-flavoured captures into the same stack. | Confirmed by maintainer 2026-08-08 (O14). One stack must arbitrate raw input and matched intents together — a UI layer has to stop both. |
| D22 | **Device → local-player ownership is explicit**, never inferred from `UserIndex` alone. Both networked co-op and split-screen are supported, including assigning keyboard/mouse to a specific local player. | Maintainer 2026-08-08. The keyboard reports `UserIndex 0` regardless of which split-screen player is using it, so inference cannot work. |
| D2 | Enhanced Input + `UEnhancedInputUserSettings` stays the **binding/rebinding store** | It ships, it is AS-bound, BusterBlock consumes it, and CkGameSettings decision #3 locked it two days earlier. A move list changes *matching*, not *binding*. |
| D3 | Acquisition is a Slate `IInputProcessor`; EI is polled only for the resolved key map | Sub-frame arrival order and every-`FKey` visibility are unavailable through polled EI. |
| D4 | Intents are **client-local**. Frame records and intent entities never replicate | 4-player co-op PvE. No client→server input transport exists in the framework, and building one imports fighters' hardest problem for no gain. Server-visible effects route through OwningClientAuth SMs / existing authority channels. |
| D5 | Intent phase is a **fragment + dedicated signals**, never `CkByteAttribute` | Attribute requests coalesce within a tick → one `OnValueChanged` carrying the last value. Transient phases would be unobservable. |
| D6 | **Poll/consume is the primary gameplay surface**; signals are the presentation surface | Late binders under `FireIfPayloadInFlight*` receive only the LAST payload, so signal replay can never reconstruct a sequence. State machines drain at their own decision points. |
| D7 | Deferral derives from **forward ambiguity only** — hold thresholds and chord windows. Never from a token's membership as a sequence *suffix* | A motion's prior steps are already in the buffer; the backward scan separates `236P`/`2P`/`P` on the press frame. This is what makes zero-latency specials possible. |
| D8 | The compiled artifact is `FCk_Intent_CompiledSet` — intents + resolution table + arbitration baked together; SM activates a pre-baked set as an O(1) swap | State/character-dependent move sets are the genre norm. No mid-match rebake. |
| D9 | Compact notation (`"236+LP w=200 lenient"`) is the **authoring** surface, sharing one parser with the test fixture | AS asset blocks are flat field assignments and cannot brace-init `TArray` defaults; nested `_Steps`/`_Tokens` would mean per-move imperative construction. |
| D10 | Fixed timestep uses `TProcessorBase`'s existing `TickRate` trait + a new max-catch-up clamp | The accumulator already exists. The clamp does not, and is required. |
| D11 | Charge accumulators stay separate from the backward-scan matcher | Unifying produces a worse implementation of both. Standing instruction. |
| D12 | MK / Sekiro / Souls-grade intent is **in scope as framework capability** | CkFoundation serves many downstream projects; breadth is the requirement, not speculation. |
| D14 | Input delivery is governed by a **per-local-player layer stack** (in `CkInput`, D24) — entities are layers, captures are declarative data, propagation walks top-down and the **first matching `Consume` capture wins**. **Replaces `ECk_Input_CapturePolicy` entirely; supersedes N6 (dead — do not implement).** See [DESIGN_InputLayering.md](DESIGN_InputLayering.md). | Blocking becomes structural instead of a flag someone forgets to set. A UI layer with a catch-all Consume *is* suspension. |
| D15 | **REVISED 2026-08-08.** Recording is unconditional and **physical hold duration is a fact**. Whether an intent may *count* that time across a delivery transition is **per-intent policy**: `Inherit` / `RestartOnGain` / `RequireRePress` on gain; `Cancel` / `Freeze` / `Continue` on loss. Default `Cancel` + `RequireRePress`. Per-frame layer/consumption state joins the record. | The original "matching is unconditional" clause detonated stale charges on layer pop. Both "cancel the charge" and "carry it into the vehicle" are legitimate game designs — the framework must not legislate either. The conservative default fails loudly; the permissive one fails silently. |
| D16 | The bottom-of-stack **global action** surface lets any entity bind raw inputs with no profile entry, no intent definition, no bake. | The prototyping / debug-key requirement, per maintainer. |
| ~~D17~~ | ~~`Consume` must return `false` when unhandled.~~ **DEAD — superseded by D25b.** | Captures are declarative; there is no return value, so the footgun cannot exist. Do not reintroduce a callback to make it expressible. |
| D18 | Continuous look/move axes are **in scope** for acquisition, sampling, the frame record, and the debugger — and **out of the intent grammar** (no phases, matching, or deferral). | They have no edge to match or ambiguity to resolve, but excluding them entirely would force a second consumer of the same Slate event stream and break the "developer never touches a PlayerController" promise. |
| D19 | Add a `MaxReplayedTicks`-style clamp trait to the shared `TProcessorBase`, defaulting to unlimited. | The `ReplayMissedTicks` branch is a bare unbounded `while`; a 1 s hitch at `Hz(60)` runs 60 `DoTick`s in one frame. Default preserves every existing processor's behaviour. |
| D13 | `CkIntent` and `CkInteraction` stay **fully independent** — no dependency either direction, and no rename of the resolver's intent surface. Consolidation = a documented shared `Intent.*` gameplay-tag namespace, plus a **post-campaign bridge module** depending on both. | The resolver's intent is a *sustained, producer-agnostic* assertion — AI asserts it with no input at all (`BB_NpcAI_Combat_Feature.as:537-543`). Same domain concept at a different lifecycle, not a homonym. A downstream project already bridges input→resolver 1:1 by tag. |

## Non-goals

| Out of scope | Why |
|---|---|
| Rollback netcode | D4 makes it unnecessary. The frame record's POD-ness preserves the option for free; realizing it is a separate campaign. |
| Replicated frame records / server-authoritative input | D4. Delete this framing wherever it appears — as written it misleads toward a transport that must not exist. |
| A new rebinding model, persistence, or conflict resolution | D2. `UCk_Utils_KeyBinding_UE` covers query/remap/conflicts/swap/reset/persist today. |
| A rebinding settings page | CkGameSettings owns it (`UCk_GameSettingsUI_KeyBindingPageWidget`). |
| `UInputMappingContext` → profile importer | Moot once EI remains the binding store. |
| Keyboard / gamepad layout renderer | Highest-cost, lowest-yield Slate item. Unbound-key diagnosis works in a flat key-state list. |
| Replacing CommonUI device detection, glyph DB, UI navigation | Used unmodified. `UCk_Utils_KeyIcon_UE` already wraps the glyph lookup. |
| Intent-driven UI navigation | CommonUI owns UI input on a separate axis. CkIntent gates itself via capture policy. |
| Renaming `CkInteraction`'s intent surface | D13. Measured ~15 files across three repos (resolver quartet + inspector + generated AS, CkTests gym ×3 + autotest, downstream task/AI/assets) for a cosmetic gain; module-qualified names (`FCk_Request_InteractionResolver_StartIntent`) already disambiguate at every call site. |
| The CkIntent → InteractionResolver bridge module | D13. Needs Phases 1–6 shipped first, and must resolve an authority conflict this campaign does not own: the resolver's `Add` defaults to `ECk_Replication::Replicates` (`CkInteractionResolver_Utils.h:42`) while CkIntent is client-local (D4). Separate campaign. |

## Reading list

Read before authoring anything (non-negotiable #1 — name the neighbor you mimic).

| Read | Why |
|---|---|
| `Source/CkTimer/Public/CkTimer/CkTimer_{Fragment_Data,Fragment,Processor,Utils}.h/.cpp` | The canonical small quartet. Every new feature shape comes from here. |
| `Source/CkEcs/Public/CkEcs/Processor/CkProcessor.h` (`TickRate`, `ECk_ProcessorTickCatchUp`) | The fixed-timestep infrastructure this campaign consumes rather than rebuilds. |
| `Source/CkInput/Public/CkInput/CkKeyBinding_Utils.h` + `Subsystem/CkKeyBinding_Subsystem.h` | The binding store CkIntent reads from, and the `OnSettingsChanged` hook it re-derives on. |
| `Source/CkEcs/Public/CkEcs/Signal/CkSignal_{Macros,Fragment_Data}.h` | Signal definition + the binding-policy semantics that force the D6 two-surface split. |
| `Source/CkAttribute/CLAUDE.md` (anti-pattern #3) | Why D5 exists. Read it so the decision is not relitigated. |
| `CkGameplayDebugger/Source/CkDebuggerCommon/.../SCkDebug_EventTimeline.h` | **Shared, parameterized timeline widget** (`SLeafWidget`, `LaneLabels`, `FCkDebug_TimelineEvent`/`FCkDebug_TimelineSpan`, `OnEventSelected`), already consumed by `CkGoapDebugger`. Phase 7 extends this, it does not build a timeline. |
| `docs/campaigns/2026-08-05-CkGameSettings/PROMPT.md` + `docs/specs/2026-08-05-CkGameSettings-design.md` | The sibling boundary. Decision #3 there is binding on this campaign. |
| `Source/CkInteraction/Public/CkInteraction/InteractionResolver/` quartet | The sibling "intent" concept (D13). A sustained tag assertion selecting channel mappings and a best-target cache — the thing CkIntent's `Holding` phase must eventually be able to drive. Note `Add`'s `ECk_Replication::Replicates` default. |
| A downstream consumer's hand-rolled bridge — in the BusterBlock repo, `Script/Hfsm/Tasks/BB_Hfsm_Task_IntentToResolver.as` (+ `Script/Npc/AI/BB_NpcAI_Combat_Feature.as`, `Script/Player/BB_PlayerCharacter_Assets.as`) | Read **if that checkout is available** — it is a downstream game repo, not part of CkFoundation, and this reference goes stale by design. It is the de-facto producer CkIntent supersedes, and its header comment states the contract CkIntent must honor. Its unbind path also documents a real trap: an intent left asserted makes the resolver swallow every later `StartIntent` on that channel. |
| **ftxc** — `D:\Repos\FtxUiFramework\ftxc\docs\Ftxc_InputArchitecture.md` + `include/ftx/core/{InputStack,InputRouter}.hpp` + `src/InputStack.cpp` | **The reference architecture for D14–D17.** Read the propagation rule in `TryRouteToHandler` and the `Consume`-returns-`false` contract comment in `InputRouter.hpp:150-180` before designing the layer API. External repo — this reference goes stale by design. |
| `Script/CLAUDE.md` | AS authoring rules — required before any `.as` and before shaping any public API. |

## Things ruled out — do not re-investigate

| Ruled out | Why | Evidence |
|---|---|---|
| `CkByteAttribute` as the intent-phase carrier | Same-tick request coalescing yields one `OnValueChanged` with only the last value; the decay processor guarantees same-frame set+clear. Clamping is *not* the issue. | `Source/CkAttribute/CLAUDE.md:54`; `CkByteAttribute_Fragment_Data.h:55` (`ECk_MinMax::None` default) |
| Enhanced Input triggers / the 5.3 Combo trigger as the matcher | Event-driven not frame-indexed; no motion inputs, lenience, negative edge, charge, SOCD, or priority arbitration; state cannot be snapshotted. | Original design Part I §1 — reasoning stands |
| Divorcing EI for **binding** (as opposed to matching) | An EI-backed rebinding stack ships here with AS bindings; a sibling campaign locked it two days earlier and BusterBlock already migrated to it. Two stores = two answers to "which key is Heavy Attack". | `docs/specs/2026-08-05-CkGameSettings-design.md:205`, `:139`, `:17`; `Source/CkInput/Public/CkInput/CkKeyBinding_Utils.h` |
| A parallel fixed-timestep accumulator | `TProcessorBase` already carries `TickRate` + `ReplayMissedTicks` + remainder carry. Only the max-catch-up clamp is missing. | `Source/CkEcs/Public/CkEcs/Processor/CkProcessor.h` |
| `CkSubstep` for the logic step | Physics-substep integration; its own doc says not to use it for non-physics systems. | `Source/CkSubstep/Claude.md` |
| Speculative emission with retraction (fire `Tapped`, retract on hold) | Pushes ambiguity onto every consumer; audio/VFX either flicker or reimplement deferral. `Pending` is the compromise — and consumers that act on it must handle `Failed`. | Original design Part I §4 — reasoning stands |
| Deferral computed from "the longest window of any other intent sharing that token" | Read naively this sweeps in sequence-suffix membership and defers every button, destroying the latency guarantee. Superseded by D7. | See D7 |
| "The fighting-game surface is speculative generality" | Raised in review and **withdrawn**. CkFoundation is a framework serving many projects; breadth is the stated requirement. | D12 |
| `SampleLatestOnly` as the catch-up policy for input | Drops presses. `ReplayMissedTicks` + a clamp is the correct shape. | D10 |

## Phase index

Gate contracts live beside this file as `PHASE_N.md`. Status is tracked **only** in
[PROGRESS.md](PROGRESS.md) — do not duplicate a status column here (Exhibit-A drift).

| Phase | Scope |
|---|---|
| 0 | Research + hardware spike + netcode stance + CkGameSettings boundary + AS exposure plan. **Review gate.** |
| **1a** | **CkInput** — `Gym_Input_KeyBinding` (D23) against the **existing** keybinding + key-icon surface. No new production code. Lands first as a regression net. |
| 1 | **CkInput** — raw-event fragment + per-device capability provenance (D25a, D21), Slate preprocessor as a thin writer + priority policy + PIE focus gating, synthetic writer, device→local-player ownership (D22), **the layer stack primitive + declarative raw captures** (D14, D16, D24, D25b), press→release ownership in the router, `CkRecord` ordering spike. Rewrite `CkInput/Claude.md` — it currently describes none of this. |
| 1b | **CkInput** — the biasing stage (D20): deadzone, response curve, sensitivity, inversion; per-game configurable. This is the contract CkIntent consumes. |
| 2 | **CkInput** — ButtonId map derived from EI resolved mappings; stability, uniqueness, re-derive on `OnSettingsChanged`; two-tier button space. |
| 3 | Sampler on `TickRate = Hz(60)` + clamp trait; frame record; ring buffer; octant mapping + hysteresis; SOCD policies. |
| 4 | Intent grammar + notation parser; compiled sets + per-set resolution tables under D7; cycle validation. |
| 5 | Matcher, charge accumulators, arbiter, poll/consume API. |
| 6 | Intent fragment + signals, decay lifecycle, two-surface contract, AS + BP verification. |
| 7 | Debugger: extend `SCkDebug_EventTimeline`, near-miss display, direction rosette, key-state list, resolution-table view, **layer-stack view** (which layer consumed what, on which frame — the "why did nothing happen when I pressed X" tool). |
| 8 | Gyms (`Fighting`, `Souls`, `Debugger`) + autotest gap-closing. |

**Stop at the end of each phase for review. Do not chain phases without a checkpoint.**
Write tests alongside each phase — Phase 8 is for gaps and gym wiring, not for all testing.
