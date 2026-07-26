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
- **`OnPostLoadMapWithWorld` must filter on `InWorld == GetWorld()`.** The delegate is process-global,
  so under multi-PIE it fires for other PIE worlds; reacting to a foreign world wipes this world's
  player pools and spawns channels owned by the foreign world's PlayerStates.

---

## Anti-patterns

Don't use relay channels for high-frequency data (e.g., per-frame positions) — relay is for events. Use fragment state for continuous data.

---

## See also

- `CkAudio/Claude.md`, `CkVfx/Claude.md`, `CkCue/Claude.md`.
