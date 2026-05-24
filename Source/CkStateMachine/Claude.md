# CkStateMachine

**Purpose:** ECS-driven state machine — entities with state machine fragments transition between states based on conditions. States are `UCk_SmCondition_EntityScript`-driven with data-asset conditions. Uses `CkDynamic` for state behaviors.

**Depends on:** `CkActorRelay`, `CkCore`, `CkDynamic`, `CkEcs`, `CkLabel`, `CkLog`, `CkProvider`, `CkRecord`, `CkSettings`, `CkTimer`. UE: `NetCore` (for the replicated-fragment container push-model dirty signaling).
**Used by:** AI behavior, ability states, game mode state.

---

## Key API

- `UCk_Utils_SmCondition_UE` — add condition entities to state machine.
- `UCk_SmCondition_EntityScript` — override to define state transition conditions in C++ or Blueprint.

---

## Pattern

State machine entity has a Record of state entities; each state has a Record of condition entities; processors evaluate conditions each tick and drive transitions.

---

## Signals

- **`OnSmStarted`** — fires once when the SM enters Running state (auto-start or explicit `Request_Start`). Payload is empty.
- **`OnSmStateChanged`** — fires once on initial state entry with `PreviousStateClass == nullptr`, and again on every subsequent transition with the OLD class in `PreviousStateClass` and the NEW class in `NewStateClass`. Order: `OnSmStarted` fires *before* the initial `OnSmStateChanged`. A sink state (zero outgoing transitions) gets its initial entry fire and then no further fires — pinned by `CkAutoTest_StateMachine_NoTransitionAvailable_StaysInState`.
- **`OnSmStopped`** — fires once when the SM transitions to Stopped (via `Request_Stop` or `EndPlay`).

Consumers that need to react per-state-entry (e.g., apply per-state visuals, swap input modes) should bind `OnSmStateChanged` and treat the initial entry the same as any other transition — they get a clean per-entry pulse regardless of whether the SM ever transitions onward.

---

## Anti-patterns

Don't encode state transition logic in raw if-else chains in a Processor — define states and conditions as data assets consumed by the state machine processor.

---

## Event-driven condition resting state

`UCk_SmCondition_EventDriven::EnterCondition` writes the condition result to **`Fail`** (not `Undetermined`) immediately when the condition becomes active. This is the "not yet satisfied" resting state. Subclasses' `DoEnterCondition` runs after, so a subclass that synchronously calls `MarkSatisfied()` still ends up at `Pass` — the Fail write is the default that holds when no synchronous override happens.

**Why:** the state evaluator (`FProcessor_SmState_Evaluate`) walks transitions in declaration order and `Break`s on the first `Undetermined` it finds. With Undetermined-resting event-driven conditions, a state with two racing event-driven transitions could leave the second one's Pass forever-ignored if the first one's condition was still waiting for its event. Fail-resting lets the evaluator move past the first transition (`Fail` → `Reset` → `Continue`) and inspect the second within the same pump cycle.

**Trade-offs and constraints:**

- **Custom subclasses calling `MarkSatisfied()` synchronously in `BeginPlay`** must call `Super::BeginPlay()` *before* `MarkSatisfied()`. Reversing the order leaves the condition stuck at Fail. `UCk_SmCondition_AlwaysTrue` is the canonical example of this ordering.
- **Negation (`_NegateResult = true`) of an event-driven condition** keeps its prior "never fires" semantic. The Fail resting state is set via `Request_UpdateConditionResult` directly, not via `MarkUnsatisfied` — so the resting value isn't inverted. After the event fires, `MarkSatisfied` with negate still maps to Fail, so the transition still doesn't fire. Negating an event-driven condition has no meaningful gameplay shape; the choice here preserves the prior behavior.
- **Pump cost.** A state with N racing event-driven transitions costs ~2N pump iterations on first state entry (each transition activates → resolves Fail → state evaluator walks past it). The `_MaxPumpIterations` warning fires at 8. For typical 2–4 transition states this is invisible; for larger N consider whether all of those gates need to be on the same state.
- **`Request_ResetTransition` for fully-event-driven transitions** still preserves condition results (event-driven conditions never go back to Undetermined automatically), so a condition that flipped to Pass stays Pass across transition resets. This is unchanged by the resting-state fix.

---

## Replication

State machines are replicatable. Opt in by setting `FCk_Fragment_StateMachine_ParamsData::_Replication = Replicates`; the default is `DoesNotReplicate` and local-only SMs cost nothing extra.

### Two-channel transport

- **Server→client** is a *container* replicated fragment attached to the SM entity in Setup (`FProcessor_Sm_Setup`). Two payload shapes:
  - `FCk_RepData_StateMachine_WithHistory` — ring of `FCk_Sm_TransitionEvent` (size `RingSize = 64`) plus `_RunStatus` and `_InitialStateFingerprint`. Default; preserves intermediate state transitions for replay.
  - `FCk_RepData_StateMachine_NoHistory` — latest `_CurrentStateClass` + `_Seq` + `_CurrentStateFingerprint` + `_RunStatus`. Snap-to-current; for high-frequency or cosmetic SMs.
- **Client→server** is `ACk_StateMachineRelay_UE`, a `ACk_ActorRelay_UE` subclass acquired per owning actor via `UCk_Utils_StateMachine_UE::Acquire_RelayChannel(SM)`. Three RPCs: `Server_PushTransitionBatch`, `Server_PushCurrentState`, `Server_PushRunStatus`. Used by the owning-client authoritative path.

### Per-SM authority + replication choice

Both knobs live on params, immutable for the SM's lifetime:

| Knob | Values | Default |
|---|---|---|
| `_AuthorityModel` | `ServerAuthoritative` / `OwningClientAuthoritative` | `ServerAuthoritative` |
| `_ReplicationModel` | `WithHistory` / `WithoutHistory` | `WithHistory` |

### NetContext lifecycle parameter

`EnterState` / `ExitState` / `EnterTask` / `ExitTask` / `Tick` / `EnterCondition` / `ExitCondition` all take an `ECk_Sm_NetContext`: `Standalone` / `Server` / `OwningClient` / `NonOwningClient`. Branch behavior by role inside the lifecycle method — e.g. play SFX only on `NonOwningClient` so non-owning clients see the same effect as the owning client without authority's authoritative path running it twice. Resolved fresh from authority + ownership queries on every call (no caching).

### Single-authority rule (spec §5.6)

Mutating requests (`Request_Start`/`Stop`/`Pause`/`Resume`/`Transition`/`AddOverrideState`) may only be enqueued on the SM's authority. `FProcessor_Sm_HandleRequests` ensures + drops requests originating off-authority:

- `ServerAuthoritative` SMs: server only.
- `OwningClientAuthoritative` SMs: owning client only.
- `DoesNotReplicate` SMs: every machine is self-authoritative.

Non-authority machines drive Enter/Exit exclusively via the replay path (`FProcessor_Sm_ApplyReplicatedHistory`).

### DefineState determinism (spec §9)

Every machine's `DefineState` output is hashed into `FFragment_SmState_Fingerprint._Hash` immediately after `DefineState` returns. The hash is structural: declaration-order task/transition/condition classes plus the compose-from list. The fingerprint flows on the wire inside each `FCk_Sm_TransitionEvent._NewStateFingerprint`, and the WithHistory RepData also carries `_InitialStateFingerprint`. Non-authority machines verify their locally-computed fingerprint against the replicated value at construction time on every replayed transition. On mismatch: ensure + `FTag_Sm_DeterminismFault` quarantines the SM (no further transitions land).

**This is enforced. Identical `DefineState` bodies are a hard rule.** Authoring the same state class differently across builds is a determinism fault, not a graceful degradation.

### Stash-and-flush precedence (spec §5.4)

Rep payloads can arrive before the client's `FProcessor_Sm_Setup` has populated `FFragment_Sm_Current`. The OnChange/OnAdd handlers route to `FFragment_Sm_PendingReplicationEntries` when either:

1. `FFragment_Sm_Current` is absent (Setup hasn't run), OR
2. `FFragment_Sm_PendingReplicationEntries` is already non-empty (preserves arrival order under back-to-back deliveries — without this, a later OnChange could land ahead of earlier stashed entries on the next pump).

`FProcessor_Sm_FlushPendingReplication_Drain` releases stash → `FFragment_Sm_ReplayQueue` in arrival order once Setup completes, then mirrors any stashed `_RunStatus`.

### Lagged-out gap recovery (spec §5.7)

If a client's `_ClientLastAppliedSeq` falls below the earliest seq the WithHistory ring carries (the client missed events that have aged out), the OnChange handler logs a warning and synthesizes a single snap event from local current → `History.Last().NewStateClass`. **Snap-no-op**: if local current already equals the target, just advance the watermark — no synthetic event, no spurious Enter/Exit.

### Echo suppression (spec §5.5)

On an `OwningClientAuthoritative` SM, the owning client commits transitions locally for zero latency *and* publishes them through the relay. The server applies them and re-publishes through the rep payload — which makes a round-trip back to the same owning client. OnChange/OnAdd detect this case (`OwningClientAuth` + `IsEntityLocallyControlled_ByPlayer`) and no-op so the same transition isn't applied twice.

### Anti-patterns

- **Don't replicate high-frequency SMs with `WithHistory`.** The ring fills fast and bandwidth dominates. Use `WithoutHistory` for animation state, gesture, anything that snap-to-current would handle correctly.
- **Don't transfer authority mid-flight.** v1 doesn't support possession transfer (`ServerAuth` → `OwningClientAuth` or vice versa) at runtime. Pick a model at SM creation; if you need both, instantiate two SMs.
- **Don't author divergent `DefineState` per machine.** The determinism fingerprint is a runtime ensure, not a soft warning — diverging structurally faults the SM and no transitions land.
- **Don't call mutating `Request_*` from non-authority.** They're ensured + dropped (see Single-authority rule above). Drive non-authority state changes via the replay path.

### See also

- `docs/superpowers/specs/2026-05-20-statemachine-replication-design.md` — full design spec.
- `docs/superpowers/plans/2026-05-20-statemachine-replication.md` — phased implementation plan + meta-constraints.
- `CkStateMachine/Net/CkStateMachine_NetContext.h` — enums and `FCk_Sm_TransitionEvent`.
- `CkStateMachine/Net/CkStateMachine_NetContextUtils.h` — `ComputeNetContext`, `MirrorRunStatus`.
- `CkStateMachine/Net/CkStateMachineRelay_Actor.h` — relay actor + `Server_*` RPCs.
- `CkStateMachine/State/CkSmState_Fingerprint.h` — structural fingerprint algorithm.

---

## See also

- `CkDynamic/Claude.md` — state behaviors.
- `CkTimer/Claude.md` — timeouts within states.
