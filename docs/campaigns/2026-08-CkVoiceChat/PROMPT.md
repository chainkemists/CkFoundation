# CkVoiceChat — mission brief (PROMPT.md)

> **Written:** 2026-08-02. STABLE content only — current state lives in [PROGRESS.md](PROGRESS.md).
> **This doc dies when:** the campaign ships and `Source/CkVoiceChat/Claude.md` becomes the module
> doc of record (P5). On death: replace the body with one tombstone line.

## Goal

CkFoundation ships `CkVoiceChat`, a Tier-4 runtime module providing proximity voice chat —
microphone capture, Opus encoding, server-routed transport with server-side interest management,
and spatialized playback — as three composable ECS features (VoiceTalker / VoiceChannel /
VoiceListener), with zero OnlineSubsystem/session/service coupling, working over any UNetDriver.

## Design of record

- **Spec:** [docs/specs/2026-08-02-CkVoiceChat-technical-review.md](../../specs/2026-08-02-CkVoiceChat-technical-review.md)
  (7 ADRs, module design, wire format, threading model, P0–P5 plan).
- **CTO review (BINDING):** [docs/reviews/2026-08-02-CkVoiceChat-CTO-review.md](../../reviews/2026-08-02-CkVoiceChat-CTO-review.md)
  — verdict **GREEN-LIGHT WITH NON-BLOCKING NOTES**; ADR-4 blessed with a constraint clause;
  N1 is a **standing condition** (must land before the P3 gate).
- Execution: phases P0–P5 per spec §10; every phase gate gets a top-tier audit before the next
  phase starts. Nothing is pushed without the maintainer's say-so.

## Success criteria

1. P0: module skeleton compiles (C++ + AS bindings clean); ADR-4 doctrine amendment landed in
   `Source/CkActorRelay/CLAUDE.md`; spike memo committed answering the three Iris questions.
2. P1: `Codec/` pure layer unit tests green headless; encode/decode micro-benchmark recorded.
3. P2: AutoTests green with fake capture; `[EDITOR-VERIFY]` real-mic loopback confirmed by a
   human; **packaged Opus-init smoke (review N5) passes**.
4. P3: net tests green incl. late-join, voice-arrives-before-registry (N1), and listen-server
   host asymmetry (host talks / host hears / client↔client).
5. P4: HybridRadio + per-channel attenuation/effects audition; moderation matrix tests green.
6. P5: full suite delta-zero vs recorded baseline; module docs merged; three-environment surface
   verified (C++/BP/AS).

## Constraints & locked decisions

| Decision | Choice | Source |
|---|---|---|
| Stack | Own stack on engine primitives; no managed service | ADR-1 (settled) |
| Capture/codec | Engine `Voice` module factories (48 kHz mono Opus); no direct libOpus link | ADR-2 (settled) |
| Playback | Custom `USynthComponent` subclass + own adaptive jitter buffer; NOT `UVoipListenerSynthComponent` | ADR-3 (settled) |
| Transport | **Relay-actor RPCs, Unreliable, paced — BLESSED as doctrine** under the clause below; custom UChannel is the named fallback, revisited only if the P0 spike shows pathological drop/starvation or the P3 profile shows relay overhead matters | ADR-4 (CTO adjudicated) |
| Channels | Entities with per-channel audio config, **from day one** | ADR-5 + ruling Q3 |
| Routing set | Persistent probe, event-driven, ±hysteresis; probe filter direction is `ProbeName.MatchesAny(Filter)` | ADR-6 (settled) |
| `IVoiceChat` facade | Deferred, recorded | ADR-7 (settled) |
| Module name | `CkVoiceChat` | Ruling Q2 |
| Roger-beep | **Consumer recipe in docs; NO CkCue dep in v1**; P4 ships the documented recipe (bind `OnTransmitStarted/Stopped` → cue) + gym audition | Ruling Q4 |
| VoiceListener | Separate feature (local ears state stays off the replicated talker) | Ruling Q5 |
| MaxAudibleSpeakers | Default 8; **server-only cap** in v1 | Ruling Q6 |
| Wire recipe | 48 kHz mono Opus, 20 ms frames, 2–3 frames/RPC, header `{Seq u16, ChannelIdx u8, AmplitudeQ8 u8, NumFrames u8}`, adaptive jitter ≈60 ms start, Opus PLC | Spec §7.3 (settled) |
| Amplitude fairness | Accept self-report v1 + two server-side hardenings at P3: (a) per-talker envelope clamp with decay; (b) top-N saturation ties broken least-recently-served round-robin. Documented as known v1 limit in module Claude.md | CTO observations |
| HybridRadio routing | Membership IS the entitlement (no probe); relevancy-culled speaker transform ⇒ client renders the 2D radio branch (correct degradation) | CTO observations |
| Dep budget | **Every dep must be earned by code that consumes it, at each phase's Build.cs review.** NO `CkRelationship` ever (N4). `CkTimer` only if a real timer-entity use materializes — hysteresis/VAD bookkeeping rides `FCk_Chrono` (CkCore) | Review N4 |

### ADR-4 constraint clause (doctrine, amended into `CkActorRelay/CLAUDE.md` at P0)

A stream over relay actors is permitted iff ALL of:
(a) reliability class chosen from payload semantics — Reliable for stateful must-apply payloads,
Unreliable for disposable time-sensitive payloads that tolerate silent loss end-to-end;
(b) RPC bodies enqueue-only, never apply inline;
(c) sender identity stamped server-side from the channel owner;
(d) a per-connection byte budget drained by a pacing processor;
(e) payload bounded well under the bunch ceiling (needing chunking ⇒ needing reliability ⇒ re-justify);
(f) stats counters from day one;
(g) an explicit, stated drop policy.
Adopters: CkRenderTarget, CkVoiceChat. The Broadcast/Bind event-channel "events only" rule is
unchanged.

### Review notes ledger (N1–N8) — phase obligations

| Note | Obligation | Binding when |
|---|---|---|
| **N1** | Replace §7.4's ordering over-claim with: **drop unresolvable-`ChannelIdx` packets, count them in a stat/overlay counter, NEVER stash**; add a voice-arrives-before-registry net test | **Mandatory before the P3 gate** (standing condition of the green-light) |
| N2 | Packet-path validation per non-negotiable #3: hoisted validity local, ensure with empty body (fires once per site), separate always-on ordinary drop branch, per-talker attribution via throttled `ck::voice_chat` Warning + stat counter | P3 (routing processor) |
| N3 | `ChannelIdx` u8 reuse policy stated in code: registry indices session-append-only, ensure on wrap past 255 (or idx+epoch byte) | P3 (registry) |
| N4 | Dep list trim: no Relationship; Timer only if earned; every dep earned at P0 Build.cs review | P0, re-audited every phase |
| N5 | Packaged Opus-init smoke pulled forward to the **P2 gate** (capture+codec+playback local), full packaged smoke stays at P5 | P2 |
| N6 | Synth pins: init 48 kHz mono, audio mixer SRC owns device-rate conversion (never resample in `OnGenerateAudio`); Start/Stop game-thread-only; SPSC ring survives Stop→Start without tearing. Verify against 5.7 `SynthComponent.h` at P2 | P2 |
| N7 | Routing-policy matrix table (per-policy send-set formula) in module Claude.md | P3 |
| N8 | Citation-drift fixes in the review brief (cosmetic; paths corrected in the review doc itself if kept as record) | Any |

### P0 spike amendments (added 2026-08-03 — binding on P3, from [SpikeMemo_P0_UnreliableUnicastClientRPC.md](SpikeMemo_P0_UnreliableUnicastClientRPC.md))

Iris flags every unicast RPC `Ordered` even when Unreliable (fork `NetRPCHandler.cpp:24-33`), so
voice rides the reliable-queue machinery (deep silent queue; sent-once/never-resent). Therefore:

| # | Amendment |
|---|---|
| S1 | **No reliable RPCs on the voice relay actor, ever** — reliable attachments on the same object head-of-line-block the unreliable section |
| S2 | **The pacing processor bounds FRESHNESS, not just bytes** — the transport queues ~4096 bundles silently; drop stale frames server-side before enqueue or latency replaces loss |
| S3 | **Bundle ≤ 3 × 20 ms frames; serialized RPC < 256 B** — the server→client unreliable split threshold makes bigger payloads all-or-nothing multi-part |
| S4 | **Teardown/travel windows stop sends before destroying channel actors** — a client receiving RPCs for an unresolvable object emits unthrottled `LogIrisRpc` Errors |
| S5 | **P3 gate measures the per-connection drain budget under production net config** and sets the voice byte budget below it with headroom — the spike's PIE config drained ~850 B/tick while 8 talkers × 25 Hz × ~214 B ≈ 43 kB/s (added 2026-08-03 with the spike verdict) |

## Non-goals (v1) — spec §2

Echo cancellation/noise-suppression DSP (hook reserved); trace-based occlusion (v1 passes through
`USoundAttenuation` occlusion settings); engine `IVoiceChat` facade; console/mobile permission
plumbing (Windows first); voice recording/replay.

## Reading list

Spec + CTO review (above), then: root `CLAUDE.md` + `Source/CLAUDE.md` (doctrine);
`Source/CkActorRelay/CLAUDE.md` (amended doctrine); skills `ck-macros-and-codegen`,
`ck-change-control`, `ck-methodology`. Mimicry exemplars: **CkTimer** (quartet, request
completion), **CkRenderTarget** (module topology, Settings, relay actor + subsystem, pacing),
**CkCue** (unreliable relay RPCs), **CkVfx** (component lifetime), **CkAudio** (Director→Track
record topology, spatial update), **CkSpatialQuery** (probes; filter direction).

## Things ruled out — do not re-investigate

| Ruled out | Why | Evidence |
|---|---|---|
| Custom UChannel transport (v1) | Buys nothing measured; costs fork-coupled netcode, Iris unknowns, `ChannelDefinitions`/`StaticChannelIndex` registration fragility. Named fallback only | CTO adjudication pt 3 |
| Pre-emptive UChannel spike | Days de-risking a path we may never take | CTO adjudication pt 3 |
| EOS RTC / Vivox / ODIN | No server-side positional culling; backend connectivity breaks LAN/offline/deterministic tests; framework cannot couple to a service contract | ADR-1 |
| Direct libOpus linking | Re-implements what the engine factories give (bitrate/VBR/complexity + stats hooks); Opus impls are private, factory surface is the supported route | ADR-2 |
| `UVoipListenerSynthComponent` | Drags in OnlineSubsystemUtils; UE-146893 seamless-travel GC bug; opaque buffering | ADR-3 |
| Integer channel ids w/ global audio config | Per-channel attenuation/effects is the ecosystem's #1 unfulfilled request | ADR-5 |
| Per-frame `Get_CurrentOverlaps` polling / world scans in packet path | Explicit CkSpatialQuery anti-pattern; confirmed root cause of a reference plugin's large-level failures | ADR-6 |
| Proximity-only v1 (channels retrofitted later) | Retrofit reshapes wire format + routing processor — the two most expensive things to churn | Ruling Q3 |
| Client-side speaker cap (v1) | Client only receives what the server forwards; decode cost bounded by same N | Ruling Q6 |
