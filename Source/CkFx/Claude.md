# CkFx

**Purpose:** Sound effects (SFX) attachment to entities. Thin module for spawning one-shot audio events anchored to an entity's position, without the full AudioTrack Record lifetime management of `CkAudio`.

**Depends on:** `CkCore`, `CkEcs`, `CkEcsExt`, `CkLabel`, `CkLog`, `CkRecord`, `CkSettings`.
**Used by:** Hit effects, footstep sounds, UI clicks — anything that is fire-and-forget audio.

---

## Key API

- `UCk_Utils_Sfx_UE` — compose Sfx on an entity (`Add`), then `Request_PlayAttached` /
  `Request_PlayAtLocation` (deferred — drained by `FProcessor_Sfx_HandleRequests` on the audio
  group's tick; both carry the request-completion delegate).
- Params hold `TSoftObjectPtr` sound/settings refs; `FProcessor_Sfx_Setup` resolves them through
  CkResourceLoader (`"Sfx.Setup"` consumer id) and roots the resolved batch on the Sfx `Current`.
  Play requests queue while setup/loading is in flight (`FTag_Sfx_NeedsSetup` gates the drain).

---

## Pattern

Use for one-shot sounds. For looping / managed audio tracks, use `CkAudio`. Add early (the asset
load is a non-blocking preload at composition); play later.

---

## Anti-patterns

Don't use `CkFx` for music, ambient loops, or sounds that need explicit Stop control — use `CkAudio` for those.

---

## See also

- `CkAudio/Claude.md` — full audio track lifecycle.
