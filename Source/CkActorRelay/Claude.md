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

## Anti-patterns

Don't use relay channels for high-frequency data (e.g., per-frame positions) — relay is for events. Use fragment state for continuous data.

---

## See also

- `CkAudio/Claude.md`, `CkVfx/Claude.md`, `CkCue/Claude.md`.
