# CkActorRelay

**Purpose:** Actor relay channels — a named communication channel on an `AActor` that entities can broadcast to, and other actors/entities can listen on. Used by `CkAudio`, `CkVfx`, `CkCue` to route events to the actor-side without a direct actor reference.

**Depends on:** `CkCore`, `CkEcs`, `CkEcsExt`, `CkLabel`, `CkLog`, `CkSettings`.
**Used by:** `CkAudio`, `CkCue`, `CkObjective`, `CkVfx`.

---

## Key API

- `UCk_Utils_ActorRelay_UE::Acquire_Channel(InActorHandle, ChannelTag)` — get/create a channel.
- `Broadcast(ChannelHandle, Payload)` — fire to all listeners.
- `Bind / Unbind` — listen to channel events.

---

## Pattern

Audio/VFX modules acquire a relay channel on the owning actor; the cue system broadcasts events through the channel so actor-side Blueprint logic can react.

---

## Channel pools (`UCk_ActorRelay_Group_Subsystem_Base_UE`)

- **Spawning is deferred to `OnWorldBeginPlay`.** Spawning replicated actors before
  `UWorld::HasBegunPlay()` makes UE classify them as level-startup actors (`bNetStartup=true`), so Iris
  serializes them to clients by *path* reference instead of by class; the client cannot resolve a path
  for a dynamically spawned actor and fires "Could not find static actor" / "Failed to instantiate
  Handle" in `UObjectReplicationBridge`. A subsystem created mid-game (seamless travel) never receives
  `OnWorldBeginPlay`, so `Initialize` spawns immediately when the world has already begun play.
- **Growth is capacity-driven and one-at-a-time.** `DoMaybeGrowPool` adds a channel only when every
  existing channel is both ECS-ready and at `Get_MaxEntitiesPerChannel()`, and the pool is under
  `Get_ChannelCount()`. Waiting on the not-yet-ready channel is deliberate: bursting several channels
  at once re-creates the Iris first-packet pressure the lazy-spawn design exists to avoid. The new
  channel replicates, becomes ready, and broadcasts `ChannelReadyChanged`, which re-drives pending
  acquires through the normal retry path.
- **`Try_ResolvePending` is the sync-or-null escape hatch.** The Promise/Pending async subscription is
  the wrong fit for consumers that already drive a retry loop (an ECS processor re-evaluating every
  tick); CkStateMachine's owning-client push path uses it. Internal callers use `DoTryResolve` directly.
- **Every channel entity carries a `FFragment_SaveKey` derived from its group tag.** A save's channel row then
  classifies `EngineOwned` (capture rule 2) and rendezvouses onto the fresh world's channel for the same group, so
  the entities the channel lifetime-owns keep a resolvable owner across a load. The key is per GROUP, not per
  channel — N pooled channels share it and N saved rows consolidate onto one fresh channel, which is sound because
  channels within a group are interchangeable and nothing per-instance is stable across runs. Relay stamping uses
  the explicit shared-rendezvous policy; ordinary SaveKeys remain unique and collision-diagnosed.
- **`OnPostLoadMapWithWorld` must filter on `InWorld == GetWorld()`.** The delegate is process-global,
  so under multi-PIE it fires for other PIE worlds; reacting to a foreign world wipes this world's
  player pools and spawns channels owned by the foreign world's PlayerStates.

---

## Anti-patterns

**Broadcast/Bind event channels are for events only.** Don't use the Broadcast/Bind channel
mechanism for high-frequency data (e.g., per-frame positions) — relay events are for events. Use
fragment state for continuous data.

**Relay ACTORS as per-player RPC endpoints are a separate mechanism**, and paced, budgeted data
streams over them ARE permitted doctrine (adjudicated 2026-08-02,
`docs/reviews/2026-08-02-CkVoiceChat-CTO-review.md` § ADR-4). Replicated fragment state is the
wrong prescription for a lossy time-sensitive stream — it rides reliable, delta-serialized
container replication, and retransmitting stale data (e.g. audio) is strictly worse than dropping
it. A stream over relay actors is permitted **iff ALL of**:

- (a) the reliability class is chosen from payload semantics — Reliable for stateful must-apply
  payloads (CkRenderTarget's pixels: reliable ⇒ ordered ⇒ no resend/reorder logic), Unreliable for
  disposable time-sensitive payloads that tolerate silent loss end-to-end (voice: Opus PLC);
- (b) RPC bodies enqueue-only — never apply inline;
- (c) sender identity is stamped server-side from the channel owner — clients are not trusted to
  self-identify;
- (d) a per-connection byte budget is drained by a pacing processor;
- (e) the payload is bounded well under the bunch ceiling (needing chunking ⇒ needing reliability
  ⇒ re-justify the reliability class);
- (f) stats counters exist from day one;
- (g) the drop policy is explicit and stated.

Adopters: `CkRenderTarget` (reliable pixel streams), `CkVoiceChat` (unreliable voice streams).

---

## See also

- `CkAudio/Claude.md`, `CkVfx/Claude.md`, `CkCue/Claude.md`.
