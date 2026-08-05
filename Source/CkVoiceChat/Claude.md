# CkVoiceChat

> **Status: CAMPAIGN IN PROGRESS (P0 skeleton, 2026-08-03).** Design of record:
> [docs/specs/2026-08-02-CkVoiceChat-technical-review.md](../../docs/specs/2026-08-02-CkVoiceChat-technical-review.md);
> binding CTO review: [docs/reviews/2026-08-02-CkVoiceChat-CTO-review.md](../../docs/reviews/2026-08-02-CkVoiceChat-CTO-review.md);
> living state: [docs/campaigns/2026-08-CkVoiceChat/PROGRESS.md](../../docs/campaigns/2026-08-CkVoiceChat/PROGRESS.md).
> This doc is a stub until P5 replaces it with the full module doc.

**Purpose:** Proximity voice chat — microphone capture, Opus encoding, server-routed transport
with server-side interest management, and spatialized playback, as three composable ECS features:
**VoiceTalker** (an entity that can speak), **VoiceChannel** (a context speech happens in — child
entity under a host, per-channel spatialization policy/attenuation/effect chain), **VoiceListener**
(the local ears — per-talker client mute + receive volume). Zero coupling to OnlineSubsystem,
sessions, or external services.

**Depends on (P0):** `CkCore`, `CkEcs`, `CkEcsExt`, `CkLabel`, `CkLog`, `CkRecord`, `CkSettings`.
UE: `DeveloperSettings`, `GameplayTags`. Later phases add deps only as their code earns them
(review N4): P2 `AudioMixer`/`AudioCaptureCore`/`Voice`, P3 `NetCore`/`CkActorRelay`/`CkSpatialQuery`,
P3/P4 `CkResourceLoader`. Never `CkRelationship`.

---

## Key API (P0 surface — grows per phase)

- `UCk_Utils_VoiceTalker_UE::Add(Handle, Params)` — compose talker fragments directly on a
  transform-bearing entity. Params: transmit mode (PushToTalk default), input gain (1.2), VAD
  threshold (0.07), auto-join channel tags, playback attach socket.
- `UCk_Utils_VoiceChannel_UE::Add(HostHandle, Params)` — create a channel child entity under a
  long-lived host, labeled by its `VoiceChat.Channel.*` tag, connected via a Record on the host.
  `TryGet_VoiceChannel(Host, Tag)` resolves by label.
- `UCk_Utils_VoiceListener_UE::Add(Handle)` — compose local ears state (mute set + volume map).
- `UCk_Utils_VoiceChat_Settings_UE` — project settings getters with constexpr fallbacks
  (`ck::voice_chat::defaults`): codec (48 kHz/24 kbps/20 ms), transport (frames per RPC, byte
  budget), routing (MaxAudibleSpeakers 8 server-only, hysteresis), playback (jitter depths,
  default attenuation).

## Anti-patterns

- Don't add a `CkRelationship` dep for team channels — team semantics are membership-flag
  configurations on channel entities (ADR-5; CTO review N4).
- Don't poll overlaps for the routing set — the P3 routing processor is event-driven off a
  persistent probe (ADR-6); world scans in the packet path are the confirmed failure mode of a
  commercial reference.
- Don't stash undecodable/unroutable voice packets — voice is disposable; drop + count (review N1).

## See also

- `CkRenderTarget/CLAUDE.md` — the paced relay-actor stream precedent (ADR-4 co-adopter).
- `CkActorRelay/CLAUDE.md` — relay doctrine incl. the ADR-4 stream clause.
