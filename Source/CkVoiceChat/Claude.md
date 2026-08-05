# CkVoiceChat

> **Status: P4 feature-complete (2026-08-04); ship (P5) gated on the human audition items in
> the campaign's Gate_4.md `[EDITOR-VERIFY]` block.** Design of record:
> [docs/specs/2026-08-02-CkVoiceChat-technical-review.md](../../docs/specs/2026-08-02-CkVoiceChat-technical-review.md);
> binding CTO review: [docs/reviews/2026-08-02-CkVoiceChat-CTO-review.md](../../docs/reviews/2026-08-02-CkVoiceChat-CTO-review.md);
> living state: [docs/campaigns/2026-08-CkVoiceChat/PROGRESS.md](../../docs/campaigns/2026-08-CkVoiceChat/PROGRESS.md).

**Purpose:** Proximity voice chat — microphone capture, Opus encoding, server-routed transport
with server-side interest management, and spatialized playback, as three composable ECS features:
**VoiceTalker** (an entity that can speak), **VoiceChannel** (a context speech happens in — child
entity under a host, per-channel spatialization policy/attenuation/effect chain), **VoiceListener**
(the local ears — per-talker client mute + receive volume). Zero coupling to OnlineSubsystem,
sessions, or external services.

**Depends on (as of P4):** `CkActorRelay`, `CkCore`, `CkEcs`, `CkEcsExt`, `CkLabel`, `CkLog`,
`CkRecord`, `CkResourceLoader` (per-channel audio asset resolution), `CkSettings`, `CkShapes`,
`CkSpatialQuery` (the routing probes). UE: `AudioMixer`, `DeveloperSettings`, `GameplayTags`,
`NetCore`, `Voice`. Never `CkRelationship`.

**Playback config (P4).** A channel's authored `_Attenuation`/`_SourceEffectChain` soft refs
resolve through CkResourceLoader at channel Setup on every machine (spatializing channels with
none authored resolve the module default); `Get_ResolvedAttenuation`/`Get_ResolvedSourceEffectChain`
expose them. Each receiving machine's synth adopts the config of the **highest-`_Priority`
channel currently delivering that talker's stream** (one synth per talker per machine — the
single-playback dedupe), attaches at the owning actor's `PlaybackAttachSocketName`, and applies
spatialization per policy. **HybridRadio** renders per recipient: spatialized proximity speech
inside the channel's `AudibleRange`, flat radio through the channel's effect chain outside it,
with the same hysteresis asymmetry the routing probes use (near inside range, held to
range + margin). Remote `Get_CurrentAmplitude` mirrors the wire header while bundles flow and
releases to zero once the stream idles past the sender's stale-drop age.

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
| Recipient set | `CanHear` members ∩ the sender's probe-maintained proximity set, with hysteresis: a recipient becomes audible inside the channel's range and stays audible until range + margin (`Get_ProximityHysteresisMarginCm`). A sender or recipient without a Transform has no probes and Positional3D **fails closed** for it — no position, no proximity, never a fallback to membership-wide sends | `CanHear` members regardless of range | `CanHear` members; one wire copy, 3D-vs-radio branch chosen client-side at render |
| Not the talker entity, not the talker's own connection | required (no echo back) | same | same |
| Recipient has not listener-muted the talker | required (the privacy exclusion — muted audio is never SENT) | same | same |
| Audible-speaker cap (`MaxAudibleSpeakers`, default 8) | envelope-bucket priority, least-recently-served rotation at saturation | same | same |
| Per-connection byte budget this tick | required | same | same |

Listen-server host: the host's capture injects straight into its own routing inbox (no
self-RPC); the host receives other talkers like any client (a Client RPC on a host-owned relay
executes locally).

## Consumer recipes

### Roger-beep / walkie-talkie bip (Ruling Q4: consumer recipe, NO CkCue dep in this module)

The module fires `OnVoiceTalker_TransmitStarted` / `OnVoiceTalker_TransmitStopped` on every
transmit edge; the beep is the consumer's cue, bound in game code (any of C++/BP/AS). AngelScript
shape:

```angelscript
// In the consumer's EntityScript BeginPlay - Talker is the FCk_Handle_VoiceTalker:
utils_voice_talker::BindTo_OnTransmitStarted(Talker, ECk_Signal_BindingPolicy::IgnorePayloadInFlight,
    ECk_Signal_PostFireBehavior::DoNothing, FCk_Delegate_VoiceTalker(this, n"OnTransmitStarted"));
utils_voice_talker::BindTo_OnTransmitStopped(Talker, ECk_Signal_BindingPolicy::IgnorePayloadInFlight,
    ECk_Signal_PostFireBehavior::DoNothing, FCk_Delegate_VoiceTalker(this, n"OnTransmitStopped"));

UFUNCTION()
private void OnTransmitStarted(FCk_Handle_VoiceTalker InTalker)
{
    // The consumer's own cue tag + CkCue executor - e.g. a squelch-open bip:
    // Request_ExecuteCue_Transient(n"Cue.Voice.RogerBeep.Open", ...) on the cue subsystem.
}
```

`OnTransmitStopped` is the classic roger-beep edge (squelch-close "over"). For a per-CHANNEL beep
(different radio nets sounding different), key the cue tag off the channel the talker transmits
on. Audition: gym station (P4 `[EDITOR-VERIFY]`).

## Anti-patterns

- Don't add a `CkRelationship` dep for team channels — team semantics are membership-flag
  configurations on channel entities (ADR-5; CTO review N4).
- Don't poll overlaps for the routing set — the routing set is a persistent probe's
  event-maintained overlap set (ADR-6); world scans in the packet path are the confirmed failure
  mode of a commercial reference. The routing processor READS `FFragment_Probe_Current`'s
  maintained set (the CkCrowd Neighbors adopter pattern) only when bundles actually flow — it
  never issues a spatial query.
- Don't stash undecodable/unroutable voice packets — voice is disposable; drop + count (review N1).

## See also

- `CkRenderTarget/CLAUDE.md` — the paced relay-actor stream precedent (ADR-4 co-adopter).
- `CkActorRelay/CLAUDE.md` — relay doctrine incl. the ADR-4 stream clause.
