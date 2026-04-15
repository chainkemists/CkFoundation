# CkFx

**Purpose:** Sound effects (SFX) attachment to entities. Thin module for spawning one-shot audio events anchored to an entity's position, without the full AudioTrack Record lifetime management of `CkAudio`.

**Depends on:** `CkCore`, `CkEcs`, `CkEcsExt`, `CkLabel`, `CkLog`, `CkRecord`, `CkSettings`.
**Used by:** Hit effects, footstep sounds, UI clicks — anything that is fire-and-forget audio.

---

## Key API

- `UCk_Utils_Sfx_UE` — spawn SFX at entity position with params.

---

## Pattern

Use for one-shot sounds. For looping / managed audio tracks, use `CkAudio`.

---

## Anti-patterns

Don't use `CkFx` for music, ambient loops, or sounds that need explicit Stop control — use `CkAudio` for those.

---

## See also

- `CkAudio/Claude.md` — full audio track lifecycle.
