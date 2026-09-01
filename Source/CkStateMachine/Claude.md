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

**Which setter.** The resting `Fail` goes through `Request_SetInitialResult` — a direct fragment write with no parent wake-up — not `Request_UpdateConditionResult`. At construction the parent transition is activated lazily by the state evaluator's normal cascade, so eagerly bumping every parent's `Evaluating` tag across a freshly-built graph is pure redundant pump work; polled conditions write no resting value and pay nothing, and event-driven is kept to match. The later value-changed paths (`MarkSatisfied` / `MarkUnsatisfied`) do use `Request_UpdateConditionResult`, which wakes the parent — those are the genuine "value just changed, re-evaluate now" cases.

**Trade-offs and constraints:**

- **Custom subclasses calling `MarkSatisfied()` synchronously in `BeginPlay`** must call `Super::BeginPlay()` *before* `MarkSatisfied()`. Reversing the order leaves the condition stuck at Fail. `UCk_SmCondition_AlwaysTrue` is the canonical example of this ordering.
- **Negation (`_NegateResult = true`) of an event-driven condition** keeps its prior "never fires" semantic. The Fail resting state is set via `Request_SetInitialResult` directly, not via `MarkUnsatisfied` — so the resting value isn't inverted. After the event fires, `MarkSatisfied` with negate still maps to Fail, so the transition still doesn't fire. Negating an event-driven condition has no meaningful gameplay shape; the choice here preserves the prior behavior.
- **Pump cost.** A state with N racing event-driven transitions costs ~2N pump iterations on first state entry (each transition activates → resolves Fail → state evaluator walks past it). The `_MaxPumpIterations` warning fires at 8. For typical 2–4 transition states this is invisible; for larger N consider whether all of those gates need to be on the same state.
- **`Request_ResetTransition` for fully-event-driven transitions** still preserves condition results (event-driven conditions never go back to Undetermined automatically), so a condition that flipped to Pass stays Pass across transition resets. This is unchanged by the resting-state fix.

---

## Transition composition and evaluation

### Why a freshly-created transition is NOT FullyEventDriven

`UCk_Utils_SmTransition_UE::Create` calls `Request_MarkTransition_AsNotFullyEventDriven` unconditionally, AFTER establishing the parent-state link (the mark cascades to the parent state, so ordering is load-bearing). Reason: a zero-condition transition is *vacuous* and Passes on first evaluation, which requires the parent state to tick (`State_Update` adds `FTag_SmState_NeedsEvaluation`) — impossible if the state is FullyEventDriven. The earlier default of adding `FTag_SmTransition_FullyEventDriven` at Create assumed every transition would eventually receive an event-driven condition; vacuous transitions broke that and starved the state of evaluation entirely.

Later composition self-corrects: adding a Polled condition leaves the state not-FullyEventDriven (`Request_MarkTransition_AsNotFullyEventDriven` runs again — idempotent); adding an EventDriven condition with no Polled condition present makes `CkSmCondition_Utils::Create` re-mark transition + state back to FullyEventDriven, restoring the perf optimization. `Request_RecomputeFullyEventDrivenStatus` encodes the rule: FullyEventDriven iff at least one condition exists AND none is Polled.

### Single-pass eager AND

`FProcessor_SmTransition_Evaluate` arms EVERY `Undetermined` condition in one sweep rather than one per evaluate cycle. All armed conditions are evaluated in the same polled main-pass sweep, whose unconditional parent wake re-runs the transition evaluate within the same pump — the whole AND resolves in one frame instead of one condition per frame. `Fail` still wins immediately (early return). Safe because polled `Evaluate` is const (a pure predicate), so eagerly evaluating a condition that a failing sibling would previously have short-circuited has no side effects.

### The condition→transition wake is unconditional

After `FProcessor_SmCondition_Polled` computes a result it always does `ParentTransition.AddOrGet<FTag_SmTransition_Evaluating>()` (AddOrGet bumps the dirty-marker version even when the tag already exists). Gating that wake on the transition still *holding* `FTag_SmTransition_Evaluating` made it dead code — the transition evaluator removes that tag at the top of its own run earlier in the same frame, so a freshly-computed condition result sat unconsumed until the next frame's evaluate cycle. The event-driven path (`Request_UpdateConditionResult`) already worked this way.

---

## Cascade convergence (Gameplay_Script settle barrier)

A transition cascade (task result → state evaluate → transition → commit → new state/task
EntityScript spawn → `BeginPlay` → `EnterTask`) is a chain of consumed-marker processors, each
needing its own dispatch. Since 2026-08-24 the chain's processors declare
`LocalSettleAfter = FGroup_Gameplay_Script` + `LocalSettleTrigger` (see `CkEcs/Claude.md`
§ Group-local settle barriers), so the **event-driven** cascade converges at the end of
`FGroup_Gameplay_Script` — before Chaos/Physics/Transform/PostTransform/Replication — instead of
in the global tail pump after the push slot. A transform request enqueued in `DoEnterTask`
therefore reaches the component the same frame the new state becomes observable
(pinned by BB `Bb_AutoTest_Sm_CascadeWriteReachesComponentSameFrame`).

Deliberately fenced OUT of the barrier — do not add them:

- `SmTask_Tick` (real DeltaT into user code; ignition stays main-pass — the barrier only drains
  what the main pass ignited).
- `SmCondition_Polled` + `SmState_Update` (marker-less ⇒ replayed every pass; Polled's unconditional
  parent wake + `Request_ResetCondition`'s re-arm form a guaranteed 2-pass livelock — pinned by
  `Bb_AutoTest_Sm_PolledTransitionDoesNotTripBarrier`). Polled/vacuous transitions of a state
  entered inside the barrier arm in the NEXT main pass, exactly as before the barrier.
- `SmCondition_ResetEveryFrame` (frame-scoped reset; replaying it mid-cascade wipes armed state).
- `Sm_ApplyReplicatedHistory` / `Sm_FlushPendingReplication_Drain` (`ReplayQueue` is presence-sticky
  and deliberately one-event-per-tick; clients still converge same-frame because the main-pass drain
  feeds `FFragment_Sm_PendingTransition`, which IS a trigger).

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

### Transition authority (who decides)

`ck::statemachine::Get_IsTransitionAuthority` mirrors `FProcessor_Sm_HandleRequests`' request-authority: Standalone, OR Server+ServerAuth, OR OwningClient+OwningClientAuth, OR the listen-server host owning its own pawn, OR the `DoesNotReplicate` self-authoritative shortcut (a non-replicated local sub-SM self-derives on every machine from its replicated inputs — the framework's original sub-SM design, which keeps non-owning clients deriving sub-SM state).

ONE exception: a **relayed** sub-SM (`DoesNotReplicate` but carrying an `OwningClientAuthoritative` NetIdentity inherited from its replicated root) returns false on every machine EXCEPT the owning client — i.e. on a server the host does not own, and on non-owning observer clients. Both follow the relay (the server follows the owning client's relayed transitions; observers follow the server's leg-2 republish onto the root's WithHistory container) rather than self-evaluate, which would revert relayed state on a locally-true release condition (the "sprint self-revert"). The START lifecycle in `HandleRequests` keeps the plain `DoesNotReplicate` shortcut, so the sub-SM still starts locally on those machines in order to exist for the relay; only transition DECISIONS are suppressed.

Every decision site gates on this shared predicate, never an inline net-context check:

- `FProcessor_SmState_Evaluate` — gates before queueing, so nothing is queued and nothing is dropped later.
- `FProcessor_SmTransition_Evaluate` — `MarkedDirtyBy` is removed BEFORE the gate so a non-authority machine does not re-loop; the rep-driven replay path commits the transition on its own schedule.
- `FProcessor_SmCondition_Polled` — polled conditions are evaluated only by the transition authority.
- `FProcessor_SmTask_Tick`.

The earlier inline gates skipped only `NonOwningClient` and non-OwningClientAuth `OwningClient`, which missed the Server-of-OwningClientAuth case: a relayed sub-SM is OwningClientAuth on the server, so the server kept polling its own predicates and ticking tasks into duplicate side effects instead of following the relay.

### DefineState determinism (spec §9)

Every machine's `DefineState` output is hashed into `FFragment_SmState_Fingerprint._Hash` immediately after `DefineState` returns. The hash is structural: declaration-order task/transition/condition classes plus the compose-from list. The fingerprint flows on the wire inside each `FCk_Sm_TransitionEvent._NewStateFingerprint`, and the WithHistory RepData also carries `_InitialStateFingerprint`. Non-authority machines verify their locally-computed fingerprint against the replicated value at construction time on every replayed transition. On mismatch: ensure + `FTag_Sm_DeterminismFault` quarantines the SM (no further transitions land).

**This is enforced. Identical `DefineState` bodies are a hard rule.** Authoring the same state class differently across builds is a determinism fault, not a graceful degradation.

**The hash keys on the class NAME STRING.** `ck::statemachine::fingerprint_detail::MixClass` mixes the case-normalized name *string*, never the FName ComparisonIndex. Comparison indices are process-local (assigned in name-table population order; the accessor is literally `ToUnstableInt`) and an FName's stored case is first-registration-wins, so neither survives a packaged-client vs dedicated-server comparison. A false mismatch trips `FTag_Sm_DeterminismFault` and permanently quiesces a healthy SM — and PIE, with one shared name table, can never reproduce it. Section salts and per-name terminators keep sections and consecutive names from smearing. The result is reinterpreted uint32 → int32 (well-defined two's-complement in C++20) because UHT rejects uint32 in BlueprintRead\* UPROPERTYs.

**Per-class fingerprint cache, and the first-instantiation zero.** A state class's fingerprint comes from its DefineState output — an instance operation gated on the EntityScript Construct pipeline — but publication needs the value BEFORE the new state's instance exists, or published transition events ship with fingerprint 0 and clients cannot verify determinism on commit. `UCk_SmState_EntityScript::Get_/Set_CachedFingerprint` caches per class on first compute (every Construct's `DoComputeFingerprint` populates it; `CommitPendingTransition`'s publication path reads it). The cache is keyed by CLASS PATH, not the short FName — two same-short-named classes in different packages sharing a slot would publish the wrong fingerprint and false-fault a healthy SM; path keys also survive hot reload + PIE restarts. Single-threaded (all CkFoundation processor work is main-thread).

Residual gap: a class's FIRST-EVER instantiation misses the cache and publishes 0 (clients treat 0 as "skip verify" for that one transition). `DoBackfillFingerprintToRepData` patches the already-published payload after the fact — useful for inspection, the debugger, and late joiners who receive the patched ring — but clients that already applied the zero-fingerprint event do NOT re-verify (seq dedup). Closing it fully would need a CDO graph-walk cache pre-warm at SM setup. The backfill is authority-only, skips local-only SMs, and patches `History.Last()` (WithHistory) or `_CurrentStateFingerprint` (WithoutHistory) only when the event's/payload's class matches — guarding against a fast follow-up transition racing past the Construct callback.

### Stash-and-flush precedence (spec §5.4)

Rep payloads can arrive before the client's `FProcessor_Sm_Setup` has populated `FFragment_Sm_Current`. The OnChange/OnAdd handlers route to `FFragment_Sm_PendingReplicationEntries` when either:

1. `FFragment_Sm_Current` is absent (Setup hasn't run), OR
2. `FFragment_Sm_PendingReplicationEntries` is already non-empty (preserves arrival order under back-to-back deliveries — without this, a later OnChange could land ahead of earlier stashed entries on the next pump).

`FProcessor_Sm_FlushPendingReplication_Drain` releases stash → `FFragment_Sm_ReplayQueue` in arrival order once Setup completes, then mirrors any stashed `_RunStatus`.

### Lagged-out gap recovery (spec §5.7)

If a client's `_ClientLastAppliedSeq` falls below the earliest seq the WithHistory ring carries (the client missed events that have aged out), the OnChange handler logs a warning and synthesizes a single snap event from local current → `History.Last().NewStateClass`. **Snap-no-op**: if local current already equals the target, just advance the watermark — no synthetic event, no spurious Enter/Exit.

### Echo suppression (spec §5.5)

On an `OwningClientAuthoritative` SM, the owning client commits transitions locally for zero latency *and* publishes them through the relay. The server applies them and re-publishes through the rep payload — which makes a round-trip back to the same owning client. OnChange/OnAdd detect this case (`OwningClientAuth` + `IsEntityLocallyControlled_ByPlayer`) and no-op so the same transition isn't applied twice.

### Commit publication — two disjoint paths

`FProcessor_Sm_CommitPendingTransition`:

1. **IsRepPublisher** (`NetContext == Server`) — the server is the canonical publisher for `Replicates` SMs regardless of authority model; for ServerAuth it is both originator and publisher, for OwningClientAuth it commits via RPC and republishes through the same write path. Non-owning clients receive RepData deltas from this write and replay through `ApplyReplicatedHistory`.
2. **IsOwningClientOriginator** (`OwningClient` + `OwningClientAuthoritative`) — commits locally for zero latency and buffers into `FFragment_Sm_PendingClientBatch` for `FProcessor_Sm_PushOwningClientBatch` to RPC; the server's `Server_PushTransitionBatch` handler replays into the server pipeline and eventually lands on path 1.

Non-owning clients hit neither branch — they only consume rep deltas from OnChange/OnAdd.

### Relay handler routing (server side)

The three `ACk_StateMachineRelay_UE` `Server_*` handlers route owning-client intent into the same pipeline the server's own commits use:

- `PushTransitionBatch` → ReplayQueue → `ApplyReplicatedHistory` → `CommitPendingTransition` → the publication path that broadcasts to non-owning clients.
- `PushCurrentState` → synthesizes one event from the server's local current state to the incoming target, same pipeline.
- `PushRunStatus` → `MirrorRunStatus_OrDeferWhileReplaying` locally plus a RepData republish, deliberately *not* through `HandleRequests` because the single-authority gate would drop it (on an OwningClientAuth SM the server is not the request originator).

Server-assigned `NextSeq` values are the server's own monotonic sequence; the owning client never interprets them because echo suppression drops the round-trip, and its `_ClientLastAppliedSeq` is separate bookkeeping.

**Run-status must be pushed BEFORE the transition batch.** Incident: before `FFragment_Sm_PendingClientBatch` carried run-status, owning-client run-status changes only wrote the server→client rep container — a no-op on the client, which does not own it. `Server_PushRunStatus` was therefore dead code, the server's OwningClientAuth SM never started, and `ApplyReplicatedHistory` dropped every relayed transition (it requires `_RunStatus == Running`).

**Channel acquisition.** Incident: the earlier `Request_AcquireAnyChannel` returned "the first pooled channel", which in a listen-server session can be ANOTHER player's (e.g. the host's). On this client that is a SimulatedProxy with no NetConnection, so UE silently DROPPED the client→server RPC and the owning-client push (run-status / transitions) was lost. `Acquire_RelayChannel` now resolves the owning PlayerState and calls `Request_AcquireChannel_ForPlayer`, so the owning AutonomousProxy channel is always used. The result is sync-or-null (`Try_ResolvePending`) and the push processor retries per pump.

### Leg-2 receive dispatch (relayed sub-SM events)

The root's WithHistory container can carry a MIX of root-level transition events (empty `_SubSmIdentity`, the root's own seq space) and sub-SM events relayed through the root on behalf of a non-replicated sub-SM (non-empty `_SubSmIdentity`, each sub-SM's OWN server-assigned seq space — see `FProcessor_Sm_CommitPendingTransition`'s IsServerRepublisher branch). `Sm_DispatchWithHistory` partitions by identity and runs dedup + lagged-out recovery PER TARGET, each against its own `FFragment_Sm_ClientReplayState` / `FFragment_Sm_ReplayQueue`. The partitioning is load-bearing: feeding a sub-SM's events through the ROOT's seq space corrupts the dedup watermark AND lets lagged-out recovery snap the root to a sub-SM's state class (its snap-to-`History.Last()` must use the last event FOR THAT TARGET). Mirrors `Server_PushTransitionBatch`'s identity routing on the leg-1 path.

**Known gap — parent-then-child arrival ordering.** The local sub-SM is resolved via `TryFind_ActiveSubSm_ByParentHierarchy`; on both legs, if the hosting parent state is not active yet the event is logged and DROPPED. There is no stash-and-defer. Dropping is deliberate: a mis-timed event must never land on the wrong SM.

### Replay dedup against the queue, not just the watermark

`Sm_AppendNewerEvents` dedups against the target container's contents as well as `_ClientLastAppliedSeq`. The watermark alone is not enough: the replay queue drains one event per tick, so a ring re-delivery during a long drain (a late joiner working through a 64-event ring) re-carries every still-queued event; without the container check each re-delivery duplicated them and replayed double Enter/Exit lifecycles. Events within one delivery are seq-ascending per target, so one threshold computed up front covers the whole batch.

### Run-status mirror ordering

`MirrorRunStatus` applies immediately; `MirrorRunStatus_OrDeferWhileReplaying` is the ordering-safe form for receive paths (rep OnChange, stash drain, relay RPC). A Paused/Stopped mirror landing while replayed transitions are still queued (or one is mid-commit) is PARKED on `FFragment_Sm_DeferredRunStatusMirror` and applied by the commit tail once the queue drains — otherwise the commit's not-Running branch discards the queued transition and destroys the live state entity, letting status jump the on-the-wire ordering. Running mirrors always apply immediately (transitions require Running) and clear any parked non-Running status (latest-wins collapse). The raw `MirrorRunStatus` is for the commit-tail unpark and other queue-safe callers.

### First-sync initial state

`FTag_Sm_NeedsInitialStateEntry` + `FProcessor_Sm_FirstSyncInitialState`: the authority enters its initial state via `DoStart` (`Request_Start`). Non-authority machines never run Start (single-authority rule) and the initial entry is not a replayed transition, so without this tier they sit at `<none>` until the first transition replays — forever for a sink-state SM. WithHistory only; NoHistory SMs snap via their own OnChange handler and a first-sync here would race it.

### Sub-SM net identity

`FFragment_Sm_NetIdentity` exists because a sub-SM entity is created detached from the pawn (`Request_CreateEntity` under the task entity) and so never carries `FFragment_OwningActor_Current`. Live `ComputeNetContext` / `Get_EffectiveAuthorityModel` need the owning pawn via a non-recursive lookup and therefore misresolve on a sub-SM — it would see itself as `NonOwningClient` on the owning client and AutoDetect to ServerAuthoritative. `UCk_SmTask_SubStateMachine` snapshots the PARENT SM's live-resolved identity at `EnterTask`; EnterTask runs on every machine with the parent already resolved, so each machine captures its own correct per-machine role, and nesting chains automatically. Caveat: the NetContext half is frozen at EnterTask, so a mid-life re-possession would leave it stale (sub-SMs are expected not to outlive a possession change); the EffectiveAuthority half is machine-independent and never stale.

### SM graph entities never replicate independently

`UCk_SmState_EntityScript::Get_EffectiveReplication` forces `DoesNotReplicate` regardless of whether the owning SM replicates. The SM's transition-history container fragment is the sole server→client transport; non-authority machines rebuild the entire state sub-graph locally via the replay path (`FProcessor_Sm_ApplyReplicatedHistory` → `DoEnterState` → `Create`). Letting these children inherit the CkEntityScript default of `Replicates` made the server push each state out as an Iris net object too, so non-owning clients reconstructed a second, malformed copy via SpawnProcessor: no `FFragment_RecordOfSmTransitions` (Create never ran) and an owner ref resolving to a tombstone — the orphaned initial-state husks. Forcing DoesNotReplicate removes that second path.

Related: a snapshot-restored SM-graph entity keeps its EntityScript but not its SmState feature fragment (the hydration redrive rebuilds the real graph fresh), so both `BeginPlay` and `EndPlay` Has-guard instead of CastChecking — otherwise CastChecked ensures and NetContext resolves through a stale `_OwnerStateMachine` captured from the saved world. SM graph entities are also stamped `FTag_Snapshot_SaveTransient` at Create so the save capture never persists them as respawnable rows.

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

## Save / load (v3 rebuild + hydrate)

### Capture shape (Produce)

Both handlers emit a CANONICAL payload rather than the live one: WithHistory persists a single event `{null → CurrentStateClass, Seq 0, Fp 0}` (empty history when there is no current state); WithoutHistory persists the latest state with Seq 0 / Fp 0. Live server seqs restart in the rebuilt world, so persisting the live ring would diverge, and HydrationApply only reads the target state + status. Both are gated on the replication MODEL (not `_Replication`) so `DoesNotReplicate` SMs — default model WithHistory — persist too. Runtime state-overrides ride inside the RepData as save-only fields, never on the wire. Both NetApply shapes always return `Applied`: pre-Setup arrivals stash via `Sm_ShouldStash`, never via dispatcher NotReady retries, preserving the arrival-order contract `FlushPendingReplication_Drain` relies on.

### Params and state save opt-in

Every persisted field on `FCk_Fragment_StateMachine_ParamsData` carries the SaveGame **specifier** — the flag that sets `CPF_SaveGame`, which the `ArIsSaveGame` tagged-property gate checks. `meta=(SaveGame)` is inert metadata and round-trips NOTHING (empirically: every field restored to its default). Fields serialize through the reflected `SerializeItem` path; the proxy archive writes `_InitialStateClass` by path string.

`_ShouldPersistCurrentState` defaults to true, preserving the existing save behavior. Set it false on the live reconstructed Params when an SM's current runtime state is derived from authored/rebuilt behavior: Produce emits no state-machine payload, and HydrationApply answers `Applied` without touching an older payload if one is present. This gates save transport only; `NetApply` and live replication retain their existing behavior.

### `FCk_Sm_SavedStateOverride` must be BlueprintType

Nothing scripts it directly, but the parent RepData shapes' generated `Set_`/`Get_SavedStateOverrides` accessors are AS-registered and reference the type — leaving it a bare USTRUCT left it unregistered and broke the whole AngelScript compile at PIE start (`asINVALID_DECLARATION`). It mirrors `ck::FFragment_Sm_StateOverrides::FEntry` exactly (`_OverrideStateClass` = the class swapped in, `_CachedStatesToOverride` = the state tags it overrides, cached from the override class CDO's `Get_StatesToOverride()` at add time). Save-only: filled by Produce, consumed by HydrationApply, never filled by the wire publish paths (the `TryUpdateContainerFragment` sites in `DoPublishRunStatus` / `FProcessor_Sm_CommitPendingTransition`) — an empty array on every replicated delta.

### Why the deferred override re-add is safe

`Sm_ReinstallSavedOverrides` re-installs saved runtime state-overrides through the SAME public deferred request the live path uses (`Request_AddOverrideState` → `FProcessor_Sm_HandleRequests` → `ck::FFragment_Sm_StateOverrides`). Construct rebuilds the SM but never re-adds runtime overrides (they aren't part of the recipe), so this is the only thing that puts them back. Ordering proof:

1. The sole consumer of the override map is `UCk_Utils_SmState_UE::Get_ResolvedStateClass`, reached from `FProcessor_Sm_HandleRequests::DoEnterState` when it drains a `Request_Transition`.
2. `FProcessor_Sm_HydrationResume` enqueues its restore `Request_Transition` only in its Transition phase, reached only after observing `RunStatus == Running` on a pump LATER than its Start phase (`Request_Start` is itself deferred).
3. The `Request_AddOverrideState` calls are enqueued during HydrationApply, strictly before the `FFragment_Sm_HydrationResume` record the ladder consumes even exists, so they sit ahead of the restore transition in the SM's FIFO.

Even in the degenerate single-batch case, FIFO order within one drain applies the earlier-enqueued AddOverrideState first. No synchronous mirrored fragment write is needed. Only the class is needed for the re-add (apply re-derives `_CachedStatesToOverride` from the override-class CDO); the saved tags ride along for save-file fidelity only.

`Sm_StashHydrationResume` observes NotReady-before-any-mutation: the `FFragment_Sm_Current` guard MUST precede the override re-add, because both the override re-install and the resume stash are single-shot and enqueuing override requests on a NotReady retry would stack duplicates. Gate first, then overrides, then resume.

### HydrationResume Option-A cost (accepted, v1)

InitialState's Enter/Exit side effects re-run on every load before the restore transition lands. Because the redrive re-fires Initial-state AND saved-state entry effects, a spawn decision on a re-entered state can duplicate a subordinate — the never-double contract keeps spawn decisions off the InitialState / saved state, or idempotent behind hydrated guard flags. Mechanically: the SM is already normal-boot composed, so there is NO virgin reset and NO WaitDriver phase; `MarkedDirtyBy` pumps the ladder inside the settle drain and each settle frame also main-passes it, so convergence does not depend on pump re-run subtleties.

`FTag_Sm_IsSubMachine` is derived graph state: under rebuild+hydrate its parent's task recreates the sub-SM fresh when the parent redrives, so it is never image-restored as an orphan.

---

## Implementation notes

- **Header back-compat.** `CkStateMachine_Fragment.h` and `CkStateMachine_Utils.h` re-include the per-feature State/Condition/Transition/Task Fragment/Utils headers purely for consumers written before the split. Do not "clean them up".
- **Deferred EntityScript attach.** `FFragment_SmScript_PendingAttach` / `FTag_SmScript_PendingAttach` avoid a same-frame race with `FProcessor_EntityScript_ContinueConstruction` when an SM child (Task/Condition) is removed before BeginPlay runs — see the `CkEntityLifetime_Fragment.cpp` destruction pipeline, where `CK_IGNORE_PENDING_KILL` does NOT exclude `FTag_DestroyEntity_Initiate`.
- **SmTask lifetime invariant.** A task never outlives its owning SM (destroy cascades mark owner and dependents together). A destroyed owner under a live, ticking task means someone created or kept the task outside its owner's lifetime cascade; the ensure in `FProcessor_SmTask_Tick` names both task and owner so the creator can be found, instead of the former symptom — a tombstone-ensure storm from inside `ComputeNetContext`.
- **Debug graph walk reads the task script class directly.** `FProcessor_Sm_Debug_GraphWalk_Iterate` gets a task's script class via `UCk_Utils_SmTask_UE::Get_ScriptClass` (the same path the live cache uses), not through the task's `FFragment_EntityScript_RequestSpawnEntity` child — that child fragment leaves `ClassName` empty once the spawn request has been consumed. The archetype is still read from the child, but only for sub-SM detection (`Get_InitialStateClass`).
- **C++-only replication queries.** `UCk_Utils_StateMachine_UE::Get_Replication` / `Get_AuthorityModel` / `Get_EffectiveAuthorityModel` / `Get_ReplicationModel` are deliberately not BPFL UFUNCTIONs, to avoid the AS-binding refresh quirk that bites newly-added BPFL UFUNCTIONs within the same toolbox run.
- **Test-support fingerprint injection.** `CkStateMachine_TestSupport.h` exists because the fingerprint-mismatch AutoTests cannot synthesize a mismatch by authoring a divergent `DefineState` — the determinism Warning fires before tests can ack it, and the AutoTest harness escalates Warnings into failures even after FinishSuccess. Instead tests inject a fake fingerprint on the publisher side: the wire value arriving at the non-authority client is corrupted, the receive-side verify path runs unchanged, and the test asserts the genuine quarantine response while `_ExpectedLogErrors` suppresses the escalation. Everything in the file is `WITH_DEV_AUTOMATION_TESTS`-gated (shipping builds have no fragment, no utility, no production-side check); the UFUNCTION declarations cannot be `#if`-gated (UHT allows only `WITH_EDITORONLY_DATA` there), so the .cpp implementations stub out in non-test builds.

---

## See also

- `CkDynamic/Claude.md` — state behaviors.
- `CkTimer/Claude.md` — timeouts within states.
