# CkCue

**Purpose:** Cue system base — the framework that `CkAudio`, `CkVfx`, and `CkObjective` are built on. A Cue entity coordinates actors and ECS entities for a gameplay event (e.g., 'play this audio AND this VFX at this location together'). Uses `CkActorRelay` to bridge actor-side events.

**Depends on:** `CkActorRelay`, `CkCore`, `CkEcs`, `CkEcsExt`, `CkLabel`, `CkLog`, `CkProvider`, `CkRecord`, `CkSettings`, `CkTimer`.
**Used by:** `CkAudio`, `CkVfx`, `CkObjective`, `CkAudioEditor`, `CkVfxEditor`, `CkObjectiveEditor`.

---

## Key API

- No `_Utils.h` at top level. Each cue type (audio, VFX) extends the cue pattern via its own Utils.
- Base cue provides the Record + ActorRelay infrastructure that sub-systems build on.

---

## Pattern

A Cue entity coordinates timing, actor channels, and signal propagation for a compound gameplay event. Sub-modules (Audio, VFX) create their entity types as entries in the Cue's Record.

---

## Anti-patterns

Don't reach into cue internals from feature modules — use the Audio/VFX/Objective sub-module Utils.

---

## See also

- `CkAudio/Claude.md`, `CkVfx/Claude.md`, `CkObjective/Claude.md`, `CkActorRelay/Claude.md`.
