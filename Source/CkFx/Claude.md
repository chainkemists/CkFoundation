# CkFx

**Purpose:** Fire-and-forget effects attachment to entities — one-shot audio events (Sfx) and
one-shot Niagara spawns (Vfx) anchored to an entity's position, without the full AudioTrack Record
lifetime management of `CkAudio` or the replicated cue routing of `CkVfx`.

**Depends on:** `CkCore`, `CkEcs`, `CkEcsExt`, `CkLabel`, `CkLog`, `CkRecord`, `CkResourceLoader`
(soft sound/system params resolve through rooted batches — consumer ids `Sfx.Setup` / `Vfx.Setup`),
`CkSettings`.
**Used by:** Hit effects, footstep sounds, UI clicks, impact particles — anything fire-and-forget.

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
