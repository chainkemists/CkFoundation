# CkAudio

**Purpose:** Audio track system — `FCk_Handle_AudioDirector` is the root audio entity on a character/object; it holds a Record of `FCk_Handle_AudioTrack` entities, each managing one looping or one-shot sound. Built on `CkActorRelay` and `CkCue`.

**Depends on:** `CkActorRelay`, `CkCore`, `CkCue`, `CkEcs`, `CkEcsExt`, `CkLabel`, `CkLog`, `CkProvider`, `CkRecord`, `CkSettings`, `CkTimer`.
**Used by:** Character audio, ambient sounds, music.

---

## Key API

- `UCk_Utils_AudioDirector_UE::Add(InHandle, InParams)` — attach audio director to entity.
- `UCk_Utils_AudioTrack_UE::Request_Play(InTrackHandle, InRequest)` — play a track (see root CLAUDE.md section 9 for full signature).
- Signals: `OnTrackStarted`, `OnTrackFinished`.

---

## Pattern

```cpp
// Attach the director:
auto DirectorHandle = UCk_Utils_AudioDirector_UE::Add(InCharHandle, DirectorParams);

// Add a track:
auto TrackHandle = UCk_Utils_AudioTrack_UE::Add(DirectorHandle, TrackParams);
UCk_Utils_GameplayLabel_UE::Add(TrackHandle, Tag_Audio_BGM);

// Play:
UCk_Utils_AudioTrack_UE::Request_Play(TrackHandle, PlayRequest);
```

---

## Anti-patterns

1. Don't call UE's `UAudioComponent::Play` directly — route through audio track entities.
2. Don't skip labeling tracks when multiple tracks coexist on the same director.

---

## See also

- `CkCue/Claude.md` — cue system that audio tracks can be driven from.
- `CkActorRelay/Claude.md` — relay channel used to route audio events to actors.
- Root `CLAUDE.md` section 9 — signal bind/unbind boilerplate.

---

## Fragment shape (2026-08-06 — spec-fragment-granularity P4)

- `FCk_AudioTrack_Spec` (reflected) — authoring payload, unpacked at `Create`.
- `ck::FFragment_AudioTrack_Params` — retained residue: TrackName (RESOLVED at Create), Sound,
  Priority, OverrideBehavior, LoopBehavior, Volume, DefaultFadeIn/OutTime.
- `ck::FFragment_AudioTrack_PendingSetup` — construction-only fields (ScriptAsset + 3 library
  soft-ptrs) that survive to the DEFERRED async Setup; Setup consumes and REMOVES it on every
  completion path (success, unset-sound bail, failed-load bail).
