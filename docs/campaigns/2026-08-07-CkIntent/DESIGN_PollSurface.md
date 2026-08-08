# CkIntent — the poll surface and the layer stack

> **Status: PROPOSED — decides nothing.** Requires orchestrator/maintainer ruling. Written 2026-08-08
> by a sub-agent; the ruling, when made, lands in PROGRESS.md's decision log.

**Scope.** Develops the design space for the hole named in
[DESIGN_InputLayering.md](DESIGN_InputLayering.md) § "Layered delivery and the poll surface —
UNRESOLVED" (its open item 6). It develops the two shapes that doc names, adds a third, and works all
three through six scenarios. It proposes; it rules on nothing, renames nothing, and writes no code.

**Not in scope.** The layer-ordering question (O11, `CkRecord` iteration order), the axes-in-captures
question (O12), and the cross-module ownership of routing results (DESIGN_InputLayering open item 5)
are touched only where they constrain this one. Where a shape depends on one of them, it says so.

---

## 1. The hole, restated more sharply than the design doc states it

DESIGN_InputLayering frames the hole through charges: a charge completes while masked and detonates
when a state machine drains it five seconds later. That framing understates it. **The hole does not
need a charge.**

D6 (`PROMPT.md`) makes poll/consume the primary gameplay surface. D15-revised keeps recording
unconditional. For a *non-accumulating* construct — a tap, a chord, a quarter-circle-Punch — there is
no accumulator for a transition policy to cancel, and the backward scan resolves it on the press
frame (D7). So:

> A UI modal is up with a catch-all `Consume`. The player inputs 236+Punch. The frame record
> receives it (unconditional). The matcher resolves it on the press frame (D7's whole latency
> guarantee is that it does). An intent reaches `Completed` with **an empty delivery set**. The
> modal closes. The character's state machine reaches its decision point and polls.

Under D15-revised's *default* (`Cancel` on loss, `RequireRePress` on gain) the charge case is already
defended — the accumulator is dropped at mask time. The tap/motion case has no such defence, because
no policy in D15 applies to it. **The hole is therefore wider than the doc's own worst case and is
not closed by D15's conservative default.** Any ruling that leans on "the default is conservative"
should be checked against this paragraph first.

### 1a. A gap in D15-revised that sits directly on this hole

D15-revised withdraws the clause "matching is unconditional" and replaces it with per-intent policy —
but every policy it names (`Inherit`/`RestartOnGain`/`RequireRePress`, `Cancel`/`Freeze`/`Continue`)
is about **hold duration**. Nothing in the revision says what the matcher does with a
*non-accumulating* construct whose delivery set is empty. Three readings, all consistent with the
text as written:

| | Reading | Consequence |
|---|---|---|
| **α** | The matcher runs unconditionally and emits an intent whose delivery set may be empty; the poll surface filters. | Replay stays a pure function of the record. The debugger can honestly show "this matched and reached nobody" — a real diagnostic. Puts the entire burden on the poll surface, i.e. on this document. |
| **β** | The matcher runs but suppresses emission when the delivery set is empty. | Cheapest poll surface. But Phase 7's stated value ("why did nothing happen when I pressed X") requires the suppression itself to be recorded — which is α's data with the intent entity withheld. |
| **γ** | Matching runs **per delivering layer**, over that layer's own compiled set. | Most expensive per frame, and the only reading that composes cleanly with D8 (see §3). |

This is not a rhetorical trichotomy: the three produce different frame records and different APIs. It
would be worth ruling on α/β/γ **in the same sitting** as the poll shape, because a shape chosen
against one reading is under-specified against the others.

---

## 2. Terminology hazard: "Consume" already means two things

The campaign uses one word for two mechanisms in adjacent decisions:

| Word | Mechanism | Decided by | When |
|---|---|---|---|
| `ECk_Input_CaptureBehavior::Consume` | **Routing-time masking.** Declarative, per capture; stops propagation down the layer stack. | The capture set (D25b) | Frame N, in the router |
| "poll/**consume**" | **Gameplay-time claiming.** A consumer takes an intent so a sibling consumer does not act on it. | The consumer, at its decision point | Frame N..N+k, anywhere |

They are not the same operation and they do not compose the same way. Everything in §7 (who marks
consumed) is about the second; everything about layer masking is about the first. This doc writes
**mask** for the first and **claim** for the second wherever ambiguity would otherwise arise, and
flags the naming choice itself as something the orchestrator may want to settle:

- **Option N1** — keep `Consume` on the capture behavior (ftxc parity, matches DESIGN_InputLayering's
  table verbatim) and name the gameplay verb `Request_Claim`.
- **Option N2** — keep `Consume` on the gameplay verb (matches D6's wording, which downstream will
  read first) and rename the capture behavior (`Block` / `Absorb`).
- **Option N3** — accept the collision and rely on the type system (`ECk_Input_CaptureBehavior::Consume`
  vs `UCk_Utils_Intent_UE::Request_Consume`) to disambiguate.

The sketches below use `Request_Consume` (N3) so they read as the campaign currently writes it. That
is presentation, not a preference.

---

## 3. What is already fixed, and what it costs each shape

Constraints inherited from the decision table, each of which narrows the space:

1. **D6** — poll is primary. A shape that only fixes signals is not a candidate.
2. **D25b** — captures are declarative, no callback, no return value. A shape that needs a consumer to
   "decline" at dispatch reintroduces the footgun D25b deleted, and is out.
3. **D15-revised** — *"Per-frame layer/consumption state joins the record so the policy stays
   replayable."* **This already commits the campaign to per-frame delivery/consumption rows.**
4. **Phase 7** — the layer-stack view needs "which capture consumed a given input on a given frame",
   which is the same rows again.
5. **D14/D24** — the stack lives in `CkInput`; `CkIntent` registers intent-flavoured captures into it.
   Rows therefore cross a module boundary (DESIGN_InputLayering open item 5, unresolved).
6. **D16** — the bottom-of-stack global surface fires only if nothing above consumed.
7. **D4** — client-local. No wire format, no authority question. The rows never replicate.

**Consequence worth stating plainly: constraint 3 means Shape B's storage is not an incremental cost
of choosing B.** The rows are already mandated by D15 and Phase 7. DESIGN_InputLayering presents A and
B as two alternatives; on inspection **B is a data-layer decision that is already taken, and A is an
API decision layered over it.** The live fork is which *read API* is primary. This reframing is the
single most load-bearing observation in this document and every recommendation below depends on it —
if the orchestrator disagrees that D15's clause commits to the rows, the rest of this analysis
changes shape.

### 3a. The pivotal sub-question: is a layer the same entity as a compiled-set owner?

D8: *"the SM activates a pre-baked set as an O(1) swap"*, motivated by "state/character-dependent move
sets". The vehicle scenario is exactly that — entering a car both **pushes a layer** and **wants a
different move set**. If those are the same entity (or 1:1), then matching is naturally per-layer
(reading γ), per-layer intent state falls out for free, and Shape C below is nearly free. If a local
player has exactly one active compiled set and layers only mask it, then Shape C duplicates work that
A/B do once.

Nothing in the doc set answers this. It is cheap to answer (it is a design choice, not a code fact)
and it changes the cost ranking of the three shapes. **Recommend answering it before ruling on the
poll shape.**

---

## 4. House precedent for the API shapes (read, not assumed)

Verified by reading, 2026-08-08:

| Precedent | File | Bears on |
|---|---|---|
| `TryGet_PoiDisplayDefinition_ByConsumer(const FCk_Handle_Poi&, FGameplayTag InConsumer)` — a read parameterized by *consumer identity*, returning an invalid handle when none matches | `Source/CkPoiDisplayDefinition/Public/CkPoiDisplayDefinition/CkPoiDisplayDefinition_Utils.h:155-165` | Shape A's `_ByLayer` suffix is house-idiomatic, not an invention |
| `TryGet_CurrentInteractions_ByTarget(...)` | `Source/CkInteraction/Public/CkInteraction/InteractSource/CkInteractSource_Utils.h:134` | Same — `_By<Identity>` is the established disambiguator (no UFUNCTION overloads) |
| `Get_IsEffectivelyHidden` — a read that resolves a *cascade* into one answer, so the caller cannot forget a step | `CkPoiDisplayDefinition_Utils.h:147-153` | The shape of a poll that cannot be asked wrong |
| `Get_EvaluationResult(const FCk_Handle_SmCondition&)` — the SM's poll at its decision point; a plain `BlueprintPure` read of a fragment | `Source/CkStateMachine/Public/CkStateMachine/Condition/CkSmCondition_Utils.h:76-82` | The consumer-side shape CkIntent's poll will be mimicked against |
| `Get_CurrentStateClass` / `Get_CurrentStateHandle` | `Source/CkStateMachine/Public/CkStateMachine/StateMachine/CkStateMachine_Utils.h:162,169` | Same |
| `NotifyDebugDataConsumed()` / `Get_IsDebugDataDesired()` — pull-based consumers *stamp* their reads so a producer can skip work when nobody is polling | `Source/CkStateMachine/Public/CkStateMachine/Debug/CkStateMachine_Debug_Utils.h:29-35` | Directly reusable idea for the per-frame-cost shapes (§ perf, Shape B/C) |
| `UCk_Utils_Timer_UE` overall — `UPARAM(ref)` handle first, `Get_`/`TryGet_`/`Request_`, concrete return type on its own line for UFUNCTIONs | `Source/CkTimer/Public/CkTimer/CkTimer_Utils.h` | The idiom every sketch below follows |
| Root doctrine's **"Immediate mutators"** clause — a `Request_*` that mutates inline and enqueues nothing fires its completion delegate synchronously | `Plugins/CkFoundation/CLAUDE.md` § Request completion | Makes an immediate claim house-legal (§7) despite non-negotiable #5 |
| `ECk_Signal_BindingPolicy` comments — late binders under `FireIfPayloadInFlight*` receive only the LAST payload | `Source/CkEcs/Public/CkEcs/Signal/CkSignal_Fragment_Data.h:12-18` | Why D6 exists; also why no shape may push layering onto the signal path alone |

---

## 5. Shape A — the poll takes the consumer's layer identity

**Idea.** Every read and every claim on the intent surface is parameterized by the asking layer. The
unqualified accessor does not exist.

### API sketch

```cpp
UFUNCTION(BlueprintPure,
          Category = "Ck|Utils|Intent",
          DisplayName="[Ck][Intent] Get Phase By Layer")
static ECk_Intent_Phase
Get_Phase_ByLayer(
    const FCk_Handle_Intent& InIntent,
    const FCk_Handle_InputLayer& InLayer);

UFUNCTION(BlueprintPure,
          Category = "Ck|Utils|Intent",
          DisplayName="[Ck][Intent] Try Get Intent By Layer")
static FCk_Handle_Intent
TryGet_Intent_ByLayer(
    const FCk_Handle_InputSource& InInputSource,
    const FCk_Handle_InputLayer& InLayer,
    UPARAM(meta = (Categories = "Intent")) FGameplayTag InIntentName);

UFUNCTION(BlueprintCallable,
          Category = "Ck|Utils|Intent",
          DisplayName="[Ck][Intent] Request Consume By Layer",
          meta = (AutoCreateRefTerm = "InDelegate"))
static FCk_Handle_Intent
Request_Consume_ByLayer(
    UPARAM(ref) FCk_Handle_Intent& InIntent,
    const FCk_Request_Intent_Consume& InRequest,   // carries the claiming layer + claimant entity
    const FCk_Delegate_Request_OnCompleted& InDelegate);
```

`_ByLayer` follows the `_ByConsumer`/`_ByTarget` precedent. There is deliberately **no**
`Get_Phase(const FCk_Handle_Intent&)`; per the repo's no-back-compat-shims norm, the unqualified form
is never introduced rather than introduced-and-deprecated.

### Frame-record rows this shape needs

Minimum, if it is to answer scenario 3 correctly: the same rows Shape B carries. See §6 — A's
correctness collapses into B's data as soon as a poll is allowed to happen after the press frame.

### Scenario walk

| # | Scenario | Shape A behaviour |
|---|---|---|
| 1 | Vehicle masks Attack mid-charge | Character SM polls `_ByLayer(CharacterLayer)`. **Correct only if delivery is evaluated at the intent's completion frame, not at poll time.** If evaluated live, the vehicle pops, delivery is restored, and the stale `Completed` reappears — the detonation returns through a different door. So A must read recorded delivery ⇒ A needs B. D15's `Inherit` on the vehicle's own intent works fine (the physical hold is layer-independent by D15's fact/policy split). |
| 2 | UI modal catch-all | Every gameplay poller passes its layer, every answer is "not delivered". Works — **conditional on there being no way to ask without a layer** (see failure mode A-F1). |
| 3 | SM polls k frames after the press | Ambiguous as stated: "as layer L" does not say "as of when". Two sub-shapes — **A1** evaluate the live mask (wrong: retroactively hides intents that *were* delivered, and un-hides ones that were not); **A2** evaluate the recorded mask at the completion frame (right). Only A2 is a candidate. |
| 4 | Two layers poll, one claims | Needs a per-(intent, layer) claim table; a single global flag makes `PassThrough` a lie (see §7). Claiming through a mask must be rejected — `Request_Consume_ByLayer` with a layer that was not delivered is a `CK_ENSURE_IF_NOT` + `Failed_NotEnqueued`, never a silent no-op. |
| 5 | Bottom-of-stack global/debug (D16) | Needs a well-known bottom-layer handle. **Any consumer can pass it** — that is the hole in miniature: layer identity is a *passed value*, so it is forgeable. Mitigation would be resolving the layer from the caller's entity instead of accepting it — at which point A has become Shape C's identity model with A's API. |
| 6 | Who marks claimed, where, and at what granularity | The claimant, via an immediate mutator (§7); recorded as a row keyed (intent, layer, frame); per-layer granularity recommended. |

### Failure modes

- **A-F1 — silently decorative layering (the named failure).** Reachable three ways: (a) an
  unqualified accessor is added later "for convenience"; (b) an invalid/`{}` layer argument is treated
  as "unfiltered" rather than "no delivery"; (c) a consumer passes a layer it is not actually on. (a)
  and (b) are preventable by construction and by a test; **(c) is not preventable at all**, because
  the parameter is data the caller supplies. This is A's structural weakness.
- **A-F2 — stale-charge detonation** returns under A1 (live evaluation). Only A2 is safe.
- **A-F3 — layer-identity drift.** A consumer whose layer legitimately changed between press and poll
  must decide whether to ask as its press-time or poll-time layer. A makes this a per-call-site
  decision, i.e. it will be made inconsistently.
- **A-F4 — AngelScript ergonomics.** AS permits defaults only on trailing parameters and has no
  overloads; a mandatory layer parameter in the middle of every signature is the most verbose of the
  three shapes at the AS call site, which is where the ergonomic pressure toward a convenience
  bypass (A-F1a) originates.

### Perf shape

Per-poll: evaluating the mask is O(layers × captures) unless cached; a per-frame cached
"delivery set per intent" collapses it to O(1) after the first poll. Zero per-frame cost when nobody
polls — the only shape with that property, and it composes with the `Get_IsDebugDataDesired()`
demand-tracking idiom (`CkStateMachine_Debug_Utils.h:29-35`). Under A2 the per-frame record cost is
B's anyway, so the advantage largely evaporates.

### Phase-7 debugger

The debugger must *recompute* masks to display them. If the poll's mask evaluation is wrong, the
debugger — sharing that code — displays the same wrong answer. **A diagnostic blind spot exactly
where the tool is supposed to be authoritative.** Under A2 the debugger can read rows instead, and
the blind spot closes.

---

## 6. Shape B — per-frame delivery records beside the frame record; the poll reads through them

**Idea.** The router writes, per frame, what each layer saw and what blocked it. The poll is a lookup
into those rows rather than a live evaluation.

### API sketch

```cpp
// ── Data (CkIntent side; see DESIGN_InputLayering open item 5 for who owns the write) ──
USTRUCT(BlueprintType)
struct CKINTENT_API FCk_Intent_DeliveryRow
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Intent_DeliveryRow);

private:
    UPROPERTY(...) FGameplayTag                   _IntentName;
    UPROPERTY(...) int32                          _FrameIndex = 0;
    UPROPERTY(...) int32                          _LayerStackGeneration = 0;   // indexes the stack table
    UPROPERTY(...) TArray<FCk_Handle_InputLayer>  _DeliveredTo;
    UPROPERTY(...) FCk_Handle_InputLayer          _BlockedBy;                  // invalid when nothing blocked
    UPROPERTY(...) int32                          _BlockingCaptureIndex = INDEX_NONE;

public:
    CK_PROPERTY_GET(_IntentName);
    CK_PROPERTY_GET(_FrameIndex);
    CK_PROPERTY_GET(_LayerStackGeneration);
    CK_PROPERTY_GET(_DeliveredTo);
    CK_PROPERTY_GET(_BlockedBy);
    CK_PROPERTY_GET(_BlockingCaptureIndex);
};

// ── Read ──
UFUNCTION(BlueprintPure,
          Category = "Ck|Utils|Intent",
          DisplayName="[Ck][Intent] Get Phase As Seen By Layer")
static ECk_Intent_Phase
Get_Phase_AsSeenByLayer(
    const FCk_Handle_Intent& InIntent,
    const FCk_Handle_InputLayer& InLayer);

UFUNCTION(BlueprintPure,
          Category = "Ck|Utils|Intent",
          DisplayName="[Ck][Intent] Try Get Delivery Row")
static FCk_Intent_DeliveryRow
TryGet_DeliveryRow(
    const FCk_Handle_Intent& InIntent,
    int32 InFrameIndex);
```

### Rows the frame record must carry (the union, usable by every shape)

| Row | Why | Notes |
|---|---|---|
| Layer-stack snapshot, keyed by a **generation id** | Phase 7's layer view; replay | Store the stack once per generation, not per frame — the stack changes rarely and a per-frame copy is the wasteful shape |
| Per (frame, input-or-intent): delivery set + `_BlockedBy` + capture index | The answer to "why did nothing happen when I pressed X" | The capture index is what makes the debugger name the *specific row*, not just the layer |
| Per (frame, intent, layer): phase transition | Scenario 3 | Transitions, not per-frame snapshots |
| Per (intent, layer): claimed-by (claimant entity + frame) | Scenario 4 & 6 | Per-layer, per §7 |
| Per button: physical hold duration | D15's *fact* | Layer-independent by construction |
| Per delivery transition: **which D15 policy fired** (`Inherit`/`RestartOnGain`/`RequireRePress`, `Cancel`/`Freeze`/`Continue`) | The only way the debugger can explain a *missing* charge | Cheap; one enum per transition, and transitions are rare |

That last row is worth calling out: without it, "my charge vanished" is undiagnosable, and it is the
single most likely support question this system will generate.

### Scenario walk

| # | Scenario | Shape B behaviour |
|---|---|---|
| 1 | Vehicle masks Attack mid-charge | Prevented by construction — the completion frame's row shows an empty (or vehicle-only) delivery set; the character's read is `NotDelivered` forever after, regardless of what the stack does later. `Inherit` on the vehicle's intent reads the physical-hold row, which is layer-independent. |
| 2 | UI modal catch-all | Rows show `_BlockedBy = UILayer`, `_DeliveredTo = {}`. Every layer-qualified read answers "nothing". Same passed-value hazard as A for *who* asks. |
| 3 | SM polls k frames later | **The scenario B is built for.** It gets the delivering-frame view, unambiguously, because the row is stamped with the frame. This is the clean answer, and it is the same answer under any poll-time layer drift. |
| 4 | Two layers poll, one claims | Claim rows are per (intent, layer). The `PassThrough` layer's claim does not mutate the lower layer's row. Claiming through a mask is rejected: no delivery row ⇒ no claim row can be written. |
| 5 | Bottom-of-stack global/debug | The bottom layer is a row participant like any other; a debug consumer reads its own layer's row. Same forgeable-identity hazard as A — B fixes *when*, not *who*. |
| 6 | Who marks claimed | Same as A. B's advantage is that the claim is recorded in the same structure the debugger and replay already read. |

### Failure modes

- **B-F1 — decorative layering** is still reachable through *who asks*: B answers "as of which frame"
  authoritatively but still takes the layer as a parameter. **B fixes A-F2/A-F3 and does not fix
  A-F1c.** This is the crux of why §8 proposes a third shape.
- **B-F2 — the row is not written for something.** If any path can produce an intent without a
  delivery row (a synthetic input writer, a test fixture, a replay seek), the poll must decide what an
  absent row means. It must mean **not delivered**, never "unfiltered". A test should pin that.
- **B-F3 — ring-buffer expiry vs late polls.** The ring is finite (Phase 0 sized it ~120 frames). A
  poll older than the ring has no row. Same rule: absent ⇒ not delivered. A `Freeze`d charge held
  across a menu for 10 s is *longer than the ring* — so the charge's own state, not the ring, must
  carry its delivery eligibility. Naming this now avoids discovering it in Phase 6.
- **B-F4 — cross-module write.** Consumption is decided in `CkInput`, the rows live in `CkIntent`
  (DESIGN_InputLayering open item 5). If that boundary is resolved by having `CkIntent` re-derive
  delivery from `CkInput`'s state rather than receiving written rows, B silently degenerates into A1.

### Perf shape

Per-frame: writing rows for intents whose phase changed (not all intents, every frame). Per-poll: one
lookup, O(1) with a per-frame index, O(delivered layers) without. Memory: bounded by
ring depth × (intents that changed phase) — the generation-id trick keeps the stack out of the
per-frame cost. At realistic scale (≤4 local players, ≤8 layers, ≤40 intents) this is not a perf
question; it is a memory-layout tidiness question.

### Phase-7 debugger

**B is the debugger's data model, effectively for free.** `SCkDebug_EventTimeline` gets one lane per
layer (spans = layer active) plus one lane per intent, with spans coloured by
delivered-and-claimed / delivered-not-claimed / matched-but-delivered-to-nobody / suppressed-by-policy.
`_BlockedBy` + `_BlockingCaptureIndex` produce the literal sentence "frame 412: Attack blocked by
layer `UI_Modal`, capture row 0 (`CatchAll`, `Consume`)". Because the debugger reads *recorded facts*
rather than recomputing, a bug in the poll shows up as a **divergence between the record and observed
behaviour** — the opposite of A's blind spot.

---

## 7. Cross-cutting: who marks an intent consumed, and how it interacts with the mask

Independent of shape, and each of these needs a ruling.

**(i) The claim must be immediate, not deferred.** Non-negotiable #5 says mutations are deferred
requests. A deferred claim breaks scenario 4 outright: two consumers polling on the same frame would
both observe "unclaimed", both act, and the deferred claims would collide next frame. The house
already has the escape hatch — root `CLAUDE.md`'s **"Immediate mutators"** clause: a `Request_*` that
mutates inline and enqueues nothing fires its completion delegate synchronously after the mutation.
Consumption is precisely that. **This exception should be declared explicitly in the campaign docs
with its reason**, or someone will "fix" it into a deferred request later and reintroduce the race.

**(ii) Per-layer claim, not global.** If one flag marks an intent claimed for everyone, then a layer
whose capture said `PassThrough` can still starve a layer below it by claiming first. That makes
`PassThrough` a lie and creates a *second*, undeclared blocking mechanism competing with the capture
set — which is the exact hazard D25b removed when it deleted the `handled` return value. Blocking
should stay declared in the capture set, decided a frame earlier, inspectable. Recommend the claim
flag be keyed (intent, layer).

**(iii) Claim-through-a-mask should not be expressible.** A layer that was not delivered an intent
cannot claim it. Under §5/§6 that is an ensure + `Failed_NotEnqueued`; under §8 it is unrepresentable.

**(iv) A "claim down-stack" escape hatch should be resisted.** The obvious extension of (ii) is
`Request_Consume_Downstack` for "I really did take this globally". That is `handled == true` wearing a
different hat, and it resurrects the ftxc footgun (`Consume`-returns-`true`-unconditionally starves
everything below) that D25b's declarative model deleted. If a genuine use case appears, it should be
expressed as a *capture edit* (the layer adds a `Consume` capture, landing next frame) rather than a
runtime claim.

**(v) Same-layer claim order is nondeterministic.** Two consumers on the *same* layer polling the same
intent resolve first-come-first-served, i.e. by processor order. That is a real determinism hazard for
replay and for tests. Options: accept it and document (intents needing single-consumer semantics live
on a layer with one consumer); or route through the Phase-5 arbiter; or make the claim carry a
priority. Not this document's to choose, but it should not be discovered in Phase 6.

**(vi) Delivery-time vs poll-time layer identity.** If a consumer's layer changed between press and
poll, whose view does it get? Shapes B and C answer "delivery-time" by construction; A must choose.
Recommend delivery-time, because the alternative means an intent's visibility changes retroactively
when a layer is pushed — which is the stale-charge bug generalised.

---

## 8. Shape C (proposed by this sub-agent) — delivery materializes per-layer intent handles

**Idea.** Do not gate the poll. Gate what *exists*. At delivery, an intent materializes as per-layer
state, and the only way to obtain an intent handle is through a layer. The handle **is** the layer's
view, so no read takes a layer parameter, no read can be asked wrong, and there is no mask evaluation
at poll time at all.

This is the ECS restatement of the idea DESIGN_InputLayering already accepted for blocking: *"Blocking
is structural, not a flag."* Shape C applies that same move one level up — **visibility is structural,
not a parameter.**

### API sketch

```cpp
// Obtaining a handle REQUIRES a layer. There is no InputSource-wide enumerator.
UFUNCTION(BlueprintPure,
          Category = "Ck|Utils|InputLayer",
          DisplayName="[Ck][InputLayer] Try Get Intent By Name")
static FCk_Handle_Intent
TryGet_Intent_ByName(
    const FCk_Handle_InputLayer& InLayer,
    UPARAM(meta = (Categories = "Intent")) FGameplayTag InIntentName);

UFUNCTION(BlueprintPure,
          Category = "Ck|Utils|InputLayer",
          DisplayName="[Ck][InputLayer] Get Delivered Intents")
static TArray<FCk_Handle_Intent>
Get_DeliveredIntents(
    const FCk_Handle_InputLayer& InLayer);

// The layer a consumer entity belongs to — RESOLVED, never passed (closes A-F1c).
UFUNCTION(BlueprintPure,
          Category = "Ck|Utils|InputLayer",
          DisplayName="[Ck][InputLayer] Try Get Layer For Entity")
static FCk_Handle_InputLayer
TryGet_LayerForEntity(
    const FCk_Handle& InConsumerEntity);

// Reads carry no layer parameter — the handle already encodes the view.
UFUNCTION(BlueprintPure,
          Category = "Ck|Utils|Intent",
          DisplayName="[Ck][Intent] Get Phase")
static ECk_Intent_Phase
Get_Phase(
    const FCk_Handle_Intent& InIntent);

UFUNCTION(BlueprintCallable,
          Category = "Ck|Utils|Intent",
          DisplayName="[Ck][Intent] Request Consume",
          meta = (AutoCreateRefTerm = "InDelegate"))
static FCk_Handle_Intent
Request_Consume(
    UPARAM(ref) FCk_Handle_Intent& InIntent,
    const FCk_Request_Intent_Consume& InRequest,      // claimant entity; NO layer field — implied
    const FCk_Delegate_Request_OnCompleted& InDelegate);
```

`Get_Phase` reads like the SM-condition precedent (`Get_EvaluationResult(const FCk_Handle_SmCondition&)`)
— a plain fragment read on a typesafe handle, which is what every consumer in this codebase already
expects a poll to look like. That is not cosmetic: it is why the ergonomic pressure toward a bypass
never builds.

### Rows the frame record must carry

The same union as §6 — C does **not** replace B's rows. C is a read API; B is the record. C's per-layer
intent entity is the *live* view; B's rows are the *historical* view that Phase 7 and replay read.
Choosing C is choosing "C over B's rows as the primary read", not "C instead of B".

### Scenario walk

| # | Scenario | Shape C behaviour |
|---|---|---|
| 1 | Vehicle masks Attack mid-charge | The character layer's Attack intent simply never receives the completion, so its poll returns the pre-charge phase. Detonation is unrepresentable. Cost: per-layer charge projection — the physical hold stays the D15 fact on the input source; each delivering layer's intent carries the *policy-applied* accumulator. `Inherit` on the vehicle's intent = seed from the physical hold at materialization. This is arguably the cleanest expression of D15's fact/policy split of the three shapes: fact on the source entity, policy on the per-layer intent. |
| 2 | UI modal catch-all | Nothing materializes below the modal. A consumer holding a handle from before the modal: needs a rule — see C-F2. There is nothing to bypass, because there is no unfiltered enumerator. |
| 3 | SM polls k frames later | It polls the handle it already holds, which is its layer's view as delivered. Time-of-poll is irrelevant, layer drift is irrelevant. The cleanest of the three. |
| 4 | Two layers poll, one claims | Two distinct intent entities exist (one per delivered layer). One claim mutates one entity. `PassThrough` genuinely means both act; §7(ii) falls out for free rather than needing a keyed table. Claim-through-a-mask is unrepresentable (there is no entity to claim). |
| 5 | Bottom-of-stack global/debug (D16) | The global surface is a layer entity like any other; a debug key is a capture on it and its intents materialize there. **No special case, and no well-known forgeable handle**, because `TryGet_LayerForEntity` resolves rather than accepts. This is the scenario where C separates most cleanly from A and B. |
| 6 | Who marks claimed | The claimant, immediately (§7(i)), on the per-layer intent entity — one fragment write. The delivery row in the frame record mirrors it for the debugger/replay. |

### Failure modes

- **C-F1 — the convenience-helper regression.** The hole returns the moment anyone adds
  `Get_AllIntents(InputSource)` or `TryGet_Intent_ByName(InputSource, Tag)`. The doctrine rule that
  must be written into `Source/CkIntent/Claude.md` is one line: **no API returns an intent handle
  without a layer in the lookup path.** That is greppable and testable, unlike A's "pass the right
  layer".
- **C-F2 — orphaned handles on layer pop.** D14 ties layer lifetime to entity lifetime; popping the
  layer destroys its intent entities and any held handle goes invalid. That is *good* (it fails loudly
  through `ck::IsValid` rather than silently answering stale) but it must be documented, and consumers
  must branch on validity rather than assume — the same discipline `TryGet_*` already imposes
  everywhere else in this codebase.
- **C-F3 — entity churn.** Load-bearing assumption: intent entities are **persistent per (layer,
  intent-definition)** with a mutating phase fragment, not spawned per activation. If Phase 6's decay
  lifecycle spawns an entity per activation, C multiplies that by delivering-layer count — and fragment
  pools here are `in_place_delete` (tombstone-mode), where per-frame add/remove of fragment *types* is
  the expensive shape. **This assumption should be confirmed against Phase 6's intent-entity lifetime
  before C is adopted.** DESIGN_InputLayering already names the same trap for captures ("never one
  fragment or tag per capture").
- **C-F4 — depends on §3a.** If layers and compiled-set owners are unrelated, C duplicates matching
  or duplicates state across layers with no compensating structure.
- **C-F5 — `TryGet_LayerForEntity` needs a resolution rule.** Direct fragment on the consumer entity?
  Context-owner chain walk (`UCk_Utils_ContextOwner_UE::Get_ContextOwner`)? Lifetime-owner walk? Each
  has different behaviour for a child entity spawned by an SM task. Unresolved, and it is the one place
  C hides complexity that A makes explicit — an honest cost, not a rounding error.

### Perf shape

Per-poll: O(1) fragment read, **no mask logic at all** — the cheapest poll of the three, which matters
because D6 makes polling the *primary* surface and polls outnumber frames. Per-frame: materializing/
updating per-layer intent state at delivery, i.e. work proportional to (delivering layers × intents
that changed), which is the same order as B's row writes. Memory: layers × intents of live state, plus
B's ring. Nothing here is a perf concern at the stated scale; the concern is entity/fragment churn
shape (C-F3), not cycles.

### Phase-7 debugger

Two complementary views, both cheap: a **live tree** (input source → layers in stack order → intents
delivered to each → phase + claimed-by), which is a direct render of live state with no reconstruction;
and the **historical timeline** from B's rows. The live tree is something A cannot offer without
recomputation and B can only offer by replaying its own rows. The pairing — "here is what is true now,
here is how it got that way" — is close to the ideal shape for "why did nothing happen when I pressed
X".

---

## 9. Shapes considered and not developed

| Shape | Why not developed |
|---|---|
| **Global suppression flag consulted by the poll** | This is N6, explicitly dead. It also fails the vehicle case outright — a vehicle layer masks *Attack*, not *everything*, and a boolean cannot express per-intent masking. |
| **Poll returns everything; consumers filter themselves** | Decorative layering with extra steps, and it makes every call site a correctness surface. Fails the same test that killed ftxc's `Consume`-returns-`true` footgun. |
| **Gate at the matcher only (never emit a masked intent)** | Reading β in §1a. Viable, but it must still record the suppression for Phase 7, at which point it carries B's data with the intent withheld — so it is best evaluated *as* the α/β/γ question, not as a poll-surface shape. |
| **Global claim flag (one consumed bit per intent)** | §7(ii): makes `PassThrough` a lie and creates a second undeclared blocking mechanism. |

---

## 10. Comparison

| | A — layer-parameterized poll | B — delivery records | C — per-layer materialization |
|---|---|---|---|
| Fixes "as of when" (scenario 3) | Only as A2, i.e. by adopting B's data | ✅ by construction | ✅ by construction |
| Fixes "who is asking" (A-F1c / scenario 5) | ❌ layer is a passed value | ❌ layer is a passed value | ✅ layer is resolved, and no unfiltered enumerator exists |
| Stale-charge detonation (S1) | Only as A2 | ✅ prevented | ✅ unrepresentable |
| `PassThrough` stays honest (S4) | Needs a keyed claim table | Needs a keyed claim table | ✅ falls out of separate entities |
| Claim-through-mask | Ensure + `Failed_NotEnqueued` | Ensure + `Failed_NotEnqueued` | ✅ unrepresentable |
| Per-poll cost | O(1) cached / O(layers×captures) cold | O(1) lookup | O(1) fragment read |
| Per-frame cost | ~zero standalone; B's once A2 | Row writes (already mandated) | Row writes + per-layer state update |
| Debugger | Recomputes → shares poll bugs | ✅ reads recorded facts | ✅ live tree + B's timeline |
| AS/BP ergonomics | Worst (mandatory mid-signature param) | Same as A | Best (reads look like every other Ck poll) |
| New unknowns introduced | none | module-boundary write ownership (open item 5) | layer resolution rule (C-F5); depends on §3a and Phase 6 entity lifetime |

**The rows in bold-italic terms:** B and C are not competitors. B is the record; C is a read API over
a substrate B provides. A is a read API that, once made correct for scenario 3, *becomes* a read API
over B — and one that leaves the "who is asking" hole permanently open.

---

## 11. Recommendation, counterargument, and what would settle it

### Recommendation

**Treat B's rows as the substrate (they are already mandated by D15-revised and Phase 7, so this is
recognition rather than a new cost), and propose C as the primary gameplay read API — with A not
adopted as the primary poll shape.** The reason is narrow and structural: A and B both leave layer
identity as a *value the caller passes*, so the decorative-layering failure remains permanently
reachable by a caller that passes the wrong layer or by a convenience overload added in month six;
C makes it unreachable, because there is no way to obtain an intent handle that does not go through a
layer, and the resulting call sites look like every other poll in this codebase
(`Get_EvaluationResult(SmCondition)`) rather than carrying a mandatory correctness parameter. This
recommendation is **conditional on two answers**: §3a (is the layer the compiled-set owner?) and
C-F3 (are intent entities persistent per definition, or spawned per activation?). If the layer is not
the compiled-set owner, or intent entities are per-activation, C's cost profile inverts and A2-over-B
becomes the better shape — in which case the unqualified accessor must never be introduced, and
`Get_Phase_ByLayer` must read the recorded row rather than a live mask.

### Strongest counterargument

C taxes the 90% case: a character state machine asking "was Attack pressed?" must first obtain a layer
handle it does not naturally hold, and that friction is precisely the pressure that produces the
convenience helper (C-F1) which reopens the hole — so C may buy structural safety with an ergonomic
debt that gets paid back as the very bug it prevents. And C bakes delivery-time layer identity into
the handle, which is the wrong answer for a consumer whose layer legitimately changed between press
and poll (a pawn that changes possession mid-hold reads its *old* layer's intents until it re-resolves)
— a case A handles naturally by simply asking as whoever it currently is.

### What would settle it — cheapest first, none requiring a build

1. **Write the call sites before the implementation (≈30 minutes, paper only).** Author three
   AngelScript call sites under each shape: (a) a character SM polling Attack at its decision point;
   (b) a vehicle SM polling Accelerate while the character layer is masked; (c) a debug-key consumer at
   the bottom layer (D16). For each shape, count how many of the three can be written **wrong in a way
   that compiles and silently bypasses layering**. That number is the direct measurement of the
   failure mode this whole hole exists to prevent, and it is the discriminating experiment: the shapes
   differ on it by construction, so the count cannot come out tied by accident. Doing it in AS also
   surfaces the AS constraints early (no overloads, defaults only on trailing parameters, handle-first)
   per non-negotiable #4.
2. **Answer §3a by decision, not by search** — is a layer the compiled-set owner? It is a design choice
   nobody has made yet, it costs one sitting, and it flips C's cost ranking.
3. **Pin down Phase 6's intent-entity lifetime** (persistent per definition vs per activation) — C-F3
   is load-bearing and the answer is currently unwritten. Note that DESIGN_InputLayering already made
   the analogous ruling for captures ("captures are rows in a `TArray` inside one stable layer
   fragment... never one fragment or tag per capture") for exactly the tombstone-pool reason, so there
   is a precedent to mimic rather than a question to research.
4. **Rule on α/β/γ (§1a) in the same sitting.** A poll shape chosen against one reading of "matching is
   no longer unconditional" is under-specified against the other two.
5. **Only then, a code read:** `CkRecord`'s iteration-order guarantee (O11), which the layer stack
   needs regardless and which C's per-layer intent record would also lean on.

---

## Appendix — contradictions and stale claims found in the doc set while researching this

Reported as observations; none is this document's to fix.

1. **O13 is simultaneously open and closed.** `PROGRESS.md`'s Open-items table lists
   *"O13 — layer entity destroyed mid-dispatch | ⏳ Open"*, while `DESIGN_InputLayering.md`'s
   "Open, needs answering in Phase 1" item 3 strikes it through as **CLOSED**, and PROGRESS's own
   2026-08-08 dated entry says *"O13 closed by deferred requests rather than an ftxc-style snapshot
   guard."* The Open-items table row is stale.
2. **O9 is likewise stale in the same table** (⏳ Open) despite D18 settling it in PROGRESS's own
   decision log and `PHASE_0_RESEARCH.md`'s settled list. PROGRESS's "Current state" warns that
   PHASE_0_RESEARCH's list is stale but does **not** warn that the Open-items table is — so a reader
   following the doc's own guidance lands on the stale table.
3. **D15-revised withdraws "matching is unconditional" without replacing it for non-accumulating
   constructs** (§1a). Every policy it names is about hold duration; taps, chords and motion inputs
   fall through the revision. This is the gap that makes the poll-surface hole wider than the design
   doc's own worst case (§1).
4. **The design doc frames the hole through charges**, and separately establishes that D15's default
   (`Cancel` + `RequireRePress`) defuses charges. Read together, that could suggest the hole is
   lower-priority than it is; the tap/motion case is undefended by any default.
5. **"Consume" carries two unrelated meanings in adjacent decisions** (§2) — `ECk_Input_CaptureBehavior::Consume`
   (routing-time masking, D14/D25b) and D6's "poll/consume" (gameplay-time claiming).
6. **The immediate-vs-deferred nature of consumption is unstated anywhere**, and non-negotiable #5
   ("requests are deferred") points the wrong way for it (§7(i)). Left unstated, a later reader will
   "correct" it into a deferred request and reintroduce a same-frame double-claim race.
