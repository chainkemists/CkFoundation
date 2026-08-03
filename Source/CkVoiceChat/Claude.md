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

**Depends on (as of P3):** `CkActorRelay`, `CkCore`, `CkEcs`, `CkEcsExt`, `CkLabel`, `CkLog`,
`CkRecord`, `CkSettings`. UE: `AudioMixer`, `DeveloperSettings`, `GameplayTags`, `NetCore`,
`Voice`. Still to be earned: `CkSpatialQuery` (P3 item 5, the routing probe), `CkResourceLoader`
(P4 asset resolution). Never `CkRelationship`.

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

## Routing-policy matrix (review N7) — who receives a talker's bundles

The server routing pipeline is two processors: `FProcessor_VoiceChat_Route` (per talker:
authorization + staging) then `FProcessor_VoiceChat_FlushForwards` (per recipient: fairness cap →
byte budget → send). A bundle reaches a recipient only if EVERY row below passes:

| Gate (in order) | Positional3D | Global2D | HybridRadio (P4) |
|---|---|---|---|
| Wire `ChannelIdx` resolves in the registry | required (drop + count, never stash — N1) | same | same |
| Stamped sender owns the talker (spoof guard) | required where both resolve; player-less (NPC) talkers skip it — **known v1 gap**, an owning player can transmit on any NPC talker | same | same |
| Talker is a member with `CanTalk`, not server-muted | required | required | required (membership IS the entitlement) |
| Recipient set | `CanHear` members — **range filter via the routing probe lands with gate item 5**; until then identical to Global2D | `CanHear` members regardless of range | `CanHear` members; one wire copy, 3D-vs-radio branch chosen client-side at render |
| Not the talker entity, not the talker's own connection | required (no echo back) | same | same |
| Recipient has not listener-muted the talker | required (the privacy exclusion — muted audio is never SENT) | same | same |
| Audible-speaker cap (`MaxAudibleSpeakers`, default 8) | envelope-bucket priority, least-recently-served rotation at saturation | same | same |
| Per-connection byte budget this tick | required | same | same |

Listen-server host: the host's capture injects straight into its own routing inbox (no
self-RPC); the host receives other talkers like any client (a Client RPC on a host-owned relay
executes locally).

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
