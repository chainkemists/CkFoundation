# CkIntent — input layering (ftxc-derived)

> **Written:** 2026-08-07. **Revised:** 2026-08-08 for D25b (declarative captures) and D15-revised
> (hold/charge transition policy). **Supersedes:** N6 (observe CkUI's suspension) — N6 is DEAD, do
> not implement it — and the original design's
> `ECk_Input_CapturePolicy { Always, GameViewportFocusOnly, Suppressed }`.
> **Does not change:** D1–D13, except D-list additions below.
>
> ⚠ **The "What ftxc does" section below is a factual record of the REFERENCE implementation.**
> CkIntent does **not** copy its callback model — see "The port", which is authoritative. In
> particular ftxc's `HandleKeyCapture(...) -> bool` and its return-false contract have **no CkIntent
> equivalent** (D25b). Read ftxc for the stack/priority/consumption idea, not for the API.
> **This doc dies when:** its decisions are folded into `PROMPT.md` and `Source/CkIntent/Claude.md`.

Maintainer direction: adopt the layered input model from `D:\Repos\FtxUiFramework\ftxc`, and design
the API so **CkUI can eventually be refactored onto the same model**. Suspension is rejected in
favour of layered consumption.

---

## What ftxc actually does (all VERIFIED by reading the source)

`ftxc/docs/Ftxc_InputArchitecture.md` + `include/ftx/core/{InputStack,InputRouter}.hpp` +
`src/InputStack.cpp`.

**One stack, top-first.** `FInputStack::RouteEvent` walks `_Stack.rbegin() → rend()` (last pushed =
highest priority), then falls through to **global actions** registered at the bottom
(`InputStack.cpp:394, :317`).

**Handlers declare interest; they do not see everything.** `IInputHandler` exposes
`Get_KeyCaptures() -> std::vector<FKeyCapture>`, `HandleKeyCapture(capture, event) -> bool`, and
`Get_InputHandlerName()` (`InputRouter.hpp:253-281`). `FKeyCapture::MakeCatchAll` exists for the
modal/router case that genuinely wants every key (`:221`).

**The propagation rule is three-state, from a two-state enum plus the bool return.** Verbatim
(`InputStack.cpp:556-563`):

```cpp
auto Handled = InHandler->HandleKeyCapture(Capture, InEvent);
if (Handled && Capture.Get_Behavior() == EKeyCaptureBehavior::Consume)
{ return true; }
// PassThrough, or Consume-but-not-handled: continue to the next handler
```

| Behavior | Returns | Outcome |
|---|---|---|
| `Consume` | `true` | stop — nothing below sees it |
| `Consume` | `false` | **fall through** — "I matched but declined" |
| `PassThrough` | either | act, then continue regardless |

That third row is exactly the maintainer's car requirement — *"pass input through or fully consume
it or both"* — and it is expressed **per capture**, not per layer. One layer can consume `Accelerate`
while passing `Look` through.

**Blocking is structural, not a flag.** From their doc, verbatim: *"When a modal is shown (and
pushed), it blocks global actions automatically. No manual `IsInputBlocked()` checks needed. The
stack structure itself provides the blocking behavior."* This is the argument against suspension,
stated by the reference implementation.

**RAII pairing.** `FInputStackGuard` pops on destruction; `~IInputHandler` notifies every stack that
still pins it, fires a hard ensure naming the offending widget type (captured at push, because by
destructor time the derived type is gone), and purges the entry so later routing can't dereference a
dangling pointer (`InputStack.hpp:180-189`, `:198-204`).

**A hard-won contract worth stealing outright** (`InputRouter.hpp:150-180`): a `Consume` handler
**must** return `false` when it took no meaningful action. Returning `true` unconditionally silently
eats the key and starves every layer below. They record it with the real bug it caused (`y`
didn't open the menu, `Ftxc_TableView`, April 2026). CkIntent should carry the same rule as doctrine
with the same worked example.

---

## The port to CkIntent (ECS)

| ftxc | CkIntent | Note |
|---|---|---|
| `FInputStack` (per app) | Layer stack on the **input-source entity**, in `CkInput` (D24) — per local player, not a singleton | |
| `IInputHandler` (a widget, with a callback) | **Any entity** carrying an input-layer fragment holding a `TArray` of captures | **No interface, no callback** (D25, D25b) |
| `FKeyCapture{key, mods, actionId, behavior}` | `FCk_Input_Capture` — matches a **ButtonId** (raw) or an **IntentTag** (matched), plus behavior | data only |
| `EKeyCaptureBehavior` × `handled` bool | `ECk_Input_CaptureBehavior { Consume, PassThrough }` — **two-state, purely declarative** | see below |
| Global actions at the bottom | The **debug-key surface**: fires only if no layer above consumed | D16 |
| `FInputStackGuard` (RAII pop) | **Entity lifetime** — destroying the layer entity pops it | |
| `MakeCatchAll` | Catch-all capture, for a modal/UI layer that eats everything | |

### Propagation is declarative — the single biggest divergence from ftxc

ftxc needs its `handled` bool because a *widget's internal state* decides during dispatch. CkIntent
models that as **which captures exist**: routing walks layers top-down and the first matching
`Consume` capture wins. A layer whose state decides whether it consumes **adds or removes captures**
rather than declining at dispatch.

This deletes ftxc's return-false footgun outright — it cannot exist without a return value — and it
makes the active capture set inspectable, which a callback never was (Phase 7 renders it).

Three consequences that must be written into the implementation, not discovered:

1. **Capture edits are deferred requests** (non-negotiable #5), so a capture change lands **next
   frame**. A modal pushed by frame N's Esc does not consume the rest of frame N's input. That is a
   one-frame transition contract — state it in the module doc and test it.
2. **Captures are rows in a `TArray` inside one stable layer fragment**, mutated by request. Never
   one fragment or tag per capture — fragment pools here are tombstone-mode (`in_place_delete`), so
   per-frame add/remove of fragment *types* is the expensive shape.
3. **The router owns press→release pairing.** A layer that consumed a press must receive the matching
   release even if its captures changed or the layer popped in between; otherwise the layer below
   gets an orphaned key-up for a press it never saw. Hold per-key "consumed-by" ownership in the
   router from press to release. Layers stay declarative; the router carries the state.

Mid-dispatch mutation — which ftxc guards against by snapshotting (`InputStack.cpp:265-295`) —
**cannot occur here**: deferred requests mean the routing processor sees an immutable capture set for
the whole pass. O13 is closed by this, not by porting the guard.

Layers the maintainer named: **game** (bottom), **vehicle** (pushed on entering a car — consumes
movement, passes look through), **UI** (top, usually catch-all Consume). A UI layer with a
catch-all Consume capture **is** suspension — structurally, with no flag to forget.

### The one thing ftxc does not have to solve: frame history

ftxc routes one event synchronously and it is gone. CkIntent records a frame history and matches
backward over it.

**The original D15 said "recording AND matching are unconditional; only delivery is layered." That
was wrong, and its justification was wrong.** The defect: a player charges Attack, a vehicle layer
pushes and consumes Attack, the hold crosses its threshold and *completes into the buffer* while
delivery is masked. Because D6 makes poll/consume the primary gameplay surface, a completed
undelivered intent is not a suppressed signal — it is **state waiting for the first poller**. The
player exits the vehicle five seconds later and the state machine drains a fully-charged attack that
was never performed. Sequences mostly self-limit (windows expire); **holds and charges are unbounded
and are the worst case.**

The stated justification — that delivery-aware matching would break replay determinism — does not
hold. Per-frame layer and consumption state are rows in the frame record anyway (Phase 7's layer view
requires them), so delivery-aware matching remains a pure function of the record.

**D15 revised — separate the fact from the policy:**

> **Physical hold duration per button accumulates unconditionally** — it is a fact about the
> hardware, and it is what the record stores.
> **An intent's *eligibility* to count that time is per-intent policy**, chosen by the game.

Neither "cancel the charge" nor "carry the charge over" may be hard-coded: both are legitimate game
designs. The framework expresses both.

**On *gaining* delivery while the button is already held:**

| Policy | Behaviour |
|---|---|
| `Inherit` | Adopt the in-flight hold at its current value — the vehicle's charge continues from where the character's was, firing at once if already past threshold |
| `RestartOnGain` | Begin accumulating from the transition; the new intent needs its own full duration |
| `RequireRePress` | Nothing until a physical release-and-press |

**On *losing* delivery while still held:**

| Policy | Behaviour |
|---|---|
| `Cancel` | Drop the accumulator |
| `Freeze` | Retain the accumulated value; resume if delivery returns |
| `Continue` | Keep accumulating unseen — the old broken behaviour, now an explicit opt-in with real uses (a channel that legitimately builds while a menu is up) |

**Default: `Cancel` on loss, `RequireRePress` on gain.** Not because it is more correct, but because
the failure modes are asymmetric — the conservative default under-fires and is noticed immediately,
while the permissive default silently detonates a five-second-old charge and reads as a bug in the
character controller. Scenario "charge carries into the vehicle" is `Inherit` on the vehicle's
intent, opted into knowingly.

**Recording remains unconditional**, and per-frame layer/consumption state joins the record. That is
what keeps the policy replayable: a replay reproduces an inherited charge exactly.

### Layered delivery and the poll surface — UNRESOLVED, must be designed before Phase 6

D6 makes poll/consume the **primary** gameplay surface; D25b makes effect delivery a signal
broadcast. Signals can be gated at fire time — **a poll reads state**. If layering gates only
signals, every gameplay consumer on the poll surface bypasses the layer stack entirely and the
vehicle layer stops nothing that matters.

Candidate shapes (undecided): the poll API takes the consumer's layer identity and answers
accordingly; or per-frame delivery records are written alongside the frame record and the poll reads
through them. **This is a hole, not a detail** — it is the difference between layering working and
layering being decorative.

### The ordering problem (the real adaptation cost)

ftxc gets ordering free — `_Stack` is a `std::vector`, push order is priority. **ECS entities have no
inherent order.** Two options:

- **(a) Ordered child record** on the stack entity — append = push, remove = pop. Preserves ftxc's
  LIFO semantics exactly and keeps the mental model identical. **Requires `CkRecord` to guarantee
  iteration order — UNVERIFIED, must be checked in Phase 1.**
- **(b) Explicit `_Priority` int** per layer. Order-independent and debuggable, but invites priority
  collisions — the exact defect already sitting in the codebase, where two Slate preprocessors both
  register at index 0 (0B).

**Recommend (a)**, falling back to (b) only if `CkRecord` does not preserve order. Given 0B, if (b)
is used it needs collision detection at registration, not silent tie-breaking.

---

## Decisions this adds (proposed — need sign-off)

- **D14** — Input delivery is governed by a **per-local-player layer stack** ported from ftxc:
  entities are layers, captures declare interest, and propagation follows the three-state
  `Consume`/`PassThrough` × handled rule. Replaces `ECk_Input_CapturePolicy` entirely.
- **D15 (revised 2026-08-08)** — **Recording is unconditional; physical hold duration is a fact.
  Whether an intent may count that time across a delivery transition is per-intent policy**
  (`Inherit` / `RestartOnGain` / `RequireRePress` on gain; `Cancel` / `Freeze` / `Continue` on loss),
  defaulting to `Cancel` + `RequireRePress`. Per-frame layer/consumption state joins the record so the
  policy stays replayable. The original "matching is unconditional" clause is **withdrawn** — it
  detonated stale charges on layer pop.
- **D16** — The bottom-of-stack **global action** surface is the supported way for any entity to bind
  raw inputs with no binding-profile entry, no intent definition, and no bake — the prototyping /
  debug-key requirement.
- ~~**D17** — carry ftxc's `Consume`-must-return-`false`-when-unhandled contract.~~ **DEAD (D25b).**
  There is no return value, so the footgun cannot exist. Do not implement it; do not reintroduce a
  callback to make it expressible.
- **N6 is superseded.** CkIntent does not observe CkUI's suspension. CkUI keeps `SuspendInput` for
  its CommonUI filtering; the stated direction is that **CkUI eventually becomes a layer** on this
  same stack. The layer API must therefore be designed so a UI system can adopt it — this campaign
  does not refactor CkUI, but must not preclude it.

## Phase impact

| Phase | Change |
|---|---|
| **1** | Now delivers the **layer stack primitive** (it replaces capture policy, which was already Phase 1) + the `CkRecord` ordering spike. Raw captures land here. |
| **2** | ButtonId captures wire to the binding map. |
| **6** | IntentTag captures — layered delivery of matched intents. The two-surface contract (D6) now reads *through* the layer stack. |
| **7** | Debugger gains a **layer-stack view**: which layers exist, their order, which capture consumed a given input on a given frame. This is the "why did nothing happen when I pressed X" tool, and it is the highest-value addition the layering brings to the debugger. |

## Open, needs answering in Phase 1

1. Does `CkRecord` guarantee iteration order? Decides ordering option (a) vs (b). **UNVERIFIED.**
2. Do axes participate in captures, or only buttons/intents? A vehicle layer consuming "steer" is an
   axis consumption. **Recommend yes** — otherwise continuous input has no layering story, which
   re-creates the split the axis decision just closed.
3. ~~Layer entity destroyed mid-dispatch.~~ **CLOSED** — deferred requests make the capture set
   immutable for the whole routing pass; ftxc's snapshot guard solves a problem this substrate does
   not have. (Handle destruction still needs the usual `CK_IGNORE_PENDING_KILL` treatment.)
4. **Cross-module processor ordering.** CkInput's sampling and routing groups must run before
   CkIntent's matching in the same frame. Same-group order is registration order and is **not** safe
   for this — it needs explicit earlier groups. Name them in Phase 1.
5. **Where routing results are recorded.** Consumption is decided in `CkInput`; the frame record and
   Phase 7's layer view live in `CkIntent`. Decide which module owns the per-frame
   "which layer consumed what" rows, and how they cross the boundary.
6. **The poll-surface hole** (above). Must be designed before Phase 6.
