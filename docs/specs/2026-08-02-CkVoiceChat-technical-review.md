# CkVoiceChat — Proximity Voice Chat Module — Technical Review

**Date:** 2026-08-02
**Status:** DESIGN — awaiting CTO review; no code written
**Scope:** CkFoundation only. New Tier-4 runtime module providing microphone capture, Opus
encoding, server-routed transport, and spatialized playback as composable ECS features.
**Evidence discipline:** every load-bearing claim is marked **[CONFIRMED]** (verified against
engine/plugin source or primary documentation on 2026-08-02, citation given) or **[INFERRED]**
(reasoned, with what would confirm it).

---

## 1. Executive summary

CkVoiceChat is a self-contained proximity voice chat stack built entirely on engine primitives the
fork already ships (mic capture, Opus codec, audio-mixer synth playback) and on transport patterns
CkFoundation already proved (CkRenderTarget's paced relay-actor streaming, CkCue's unreliable relay
RPCs). It deliberately does **not** depend on any online service, online subsystem, or session
system.

Three composable features form the public surface:

- **VoiceTalker** — "this entity can speak": capture (local owner), routing state (authority),
  spatialized playback (every remote client).
- **VoiceChannel** — "a context speech happens in": per-channel spatialization policy
  (Positional3D / Global2D / HybridRadio), per-channel attenuation + source-effect chain,
  server-authoritative membership with per-member talk/listen flags.
- **VoiceListener** — "this entity is the local ears": per-talker client mute and per-talker
  receive volume.

The headline differentiators over every commercial reference analyzed (§3): proximity culls
**bandwidth server-side**, not just volume client-side; channels are runtime entities with
per-channel audio config (the ecosystem's #1 unfulfilled request); real voice-activity signals
drive UI/lipsync (not the push-to-talk key state); and the community's recurring failure modes
(seamless travel, late join, listen-server host asymmetries, packaged-build codec init) are the
test matrix from day one, not post-launch patches.

---

## 2. Goals and non-goals

### Goals

| # | Goal |
|---|---|
| G1 | Positional (proximity) voice with **server-side interest management** — packets are only forwarded to listeners who can hear them |
| G2 | Channel model expressive enough for: world proximity, global, team, radio/walkie-talkie, dead-chat, spectator (listen-only) — without per-mode special cases |
| G3 | Zero coupling to OnlineSubsystem, sessions, or external services; works over any UNetDriver (IP, P2P sockets, LAN), listen or dedicated server |
| G4 | Full three-environment API (C++ / Blueprint / AngelScript) per house non-negotiable #4 |
| G5 | PIE-first development loop: loopback mode + injectable fake capture; no "test packaged only" |
| G6 | Real speaking-state + amplitude outputs (signals + getters) for indicators and lipsync, available on server and all clients without decoding on the server |
| G7 | Deterministic lifecycle: teardown mid-transmit, seamless travel, late join, host/peer asymmetry all covered by automated tests |
| G8 | Opinionated, documented defaults; tuning exposed via ProjectSettings + CVars, never hand-edited ini surgery |

### Non-goals (v1)

- Echo cancellation / noise suppression DSP (mitigated by push-to-talk defaults + noise gate; the
  hook for a future DSP stage is reserved in the capture pipeline).
- Trace-based occlusion (v1 passes through `USoundAttenuation` occlusion settings; a dedicated
  occlusion processor is a named follow-up).
- Engine `IVoiceChat`/`IVoiceChatUser` modular-feature facade (deliberately deferred — §6 ADR-7).
- Console/mobile permission plumbing (the capture seam isolates it; Windows first).
- Recording/replay of voice.

---

## 3. Competitive research summary

Four commercial references were analyzed in depth (store listings, full manuals — two via
26/28-page PDFs, one via complete gitbook, one via marketplace archive — plus review/Q&A corpora),
alongside the engine's native stacks and managed services (EOS RTC, Vivox, ODIN, Steam, Mumble,
Discord). Full digests are retained in the research session; the decision-relevant extract:

| Product | Architecture | Verdict for us |
|---|---|---|
| **Antize VoIP System** ($79.99, Blueprint-only, EOL at UE 5.4) | Wrapper over stock OSS VoIP | Its support corpus is the lesson: voice riding the game NetDriver's default budget untuned was its #1 ticket source; session-system coupling its #1 Q&A category; "test packaged, not PIE" killed its dev loop. Steal: walkie roger-beep polish as a one-bool toggle; user-facing mic-calibration panel; speaking-indicator widget with its own cull distance. |
| **CasualCoder Simple Voice Chat** ($5.99) | Thin wrapper over engine `UVOIPTalker`; verified to ~5 players | Ergonomics only: one-call init; opinionated *documented* numeric defaults (mic threshold 0.07, gain 1.2, attenuation 500/4000/Natural/Binaural) beat exposed-but-undocumented knobs. Proximity is client-side attenuation only — everyone still receives every stream. |
| **MeoPlay Cross-Platform Voice Chat Pro** ($56, 4.1★/43) | Own capture+Opus+RPC stack; server routes | Best feature model: server relevance routing; server authorization gates (allow-global/allow-proximity/max-range clamp); raw PCM tap as public API; explicit quality knobs. Worst engineering (confirmed by publisher + reviews): `GetAllActorsOfClass` in the packet path; zero delegates (poll-until-non-null readiness); string-path attenuation assets breaking cooks; no per-channel attenuation/effects (top request); voice competing inside the game replication budget. |
| **Blue Mountains Onset VoIP** ($59.99, 4.97★/33) | Own stack over a **custom UNetDriver channel** | Best architecture: PlayerState-anchored talkers auto-created both sides; integer channels with world=0 proximity; single-send + single-playback dedupe across shared channels; "prefer 3D if near" hybrid radio with one wire copy; replicated amplitude; decoded-PCM interception with cancel-default-playback. Its changelog is a catalog of exactly the lifecycle bugs our tests must pin (seamless-travel disconnects, join-with-active-speakers crash, deaf listen-server host, spectators). Its worst setup pain: hand-edited single-line `ChannelDefinitions` ini + `StaticChannelIndex` collisions. |

Convergent industry facts **[CONFIRMED** via engine/Opus/vendor docs]:

- The spatialization pattern everyone converged on (engine VOIP, ODIN, EOS integration kits):
  **a `USynthComponent` on the remote speaker's body + a `USoundAttenuation` asset**.
- The canonical wire recipe: **48 kHz mono Opus, 20 ms frames** (960 samples — the smallest frame
  for which packet-loss concealment works well), 16–32 kbps; ~60 bytes per frame at 24 kbps;
  adaptive jitter buffer starting ≈3 frames (60 ms); Opus PLC for single losses, optional in-band
  FEC for bursts.
- The community failure-mode list: `UVoipListenerSynthComponent` GC'd on seamless travel
  (**UE-146893**); dedicated servers have no audio device (capture/playback must never initialize
  there); Opus init failures appearing only in packaged builds; listen-server host echo and
  host↔client routing asymmetries; mic-permission termination on Apple platforms when the
  Info.plist key is missing.

---

## 4. Requirements derived from research

**Functional:** proximity + global + team/radio channels, simultaneous membership, per-member
talk/listen flags; push-to-talk and open-mic (VAD-gated) transmit modes, runtime-switchable
per-talker; self-mute, client per-talker mute, server-authoritative mute; per-talker receive
volume; speaking-state + amplitude signals for local and remote talkers; per-channel attenuation +
source-effect chain + spatialization policy; loopback/self-monitor for mic testing; debug
overlay/CVars.

**Non-functional:** no per-frame world scans anywhere in the packet path; server never decodes
audio (pass-through routing; amplitude travels in the frame header); bounded per-connection voice
bandwidth with graceful top-N speaker culling; unreliable delivery (a dropped voice frame must
never head-of-line-block anything); mouth-to-ear latency target ≈100–150 ms; all mutations via
deferred requests; all validation via `CK_ENSURE_IF_NOT` + separate early-out per non-negotiable
#3; typesafe handles; signals local-only; net state via the persistence-handler registry
(`Register_NetOnly` — nothing is saved).

---

## 5. Architecture overview

```
LOCAL OWNER (talker's machine)
  IVoiceCapture (engine Voice module, 48kHz mono)         [capture thread]
    └─> FProcessor_VoiceTalker_Capture   drain PCM, gain, noise gate, VAD,
        RMS amplitude, Opus encode (IVoiceEncoder)         [game thread]
    └─> FProcessor_VoiceTalker_Transmit  bundle 2-3 frames -> header
        {Seq u16, ChannelIdx u8, AmplitudeQ8 u8, N, sizes} [game thread]
    └─> ACk_VoiceChatRelay_UE::Server_PushVoiceFrames(...)  UNRELIABLE RPC
                                 |
SERVER (authority; listen host injects its own frames locally, no RPC)
  RPC body: stamp sender from channel owner, enqueue only
    └─> FProcessor_VoiceChat_Route                          [game thread]
        validate: talker composed, channel member, CanTalk, not server-muted
        listener set = probe-overlap set (Positional3D)     (incremental, event-driven)
                       ∩ channel membership ∩ not-client-muted
                       minus the sender's own connection
        cap: top-N talkers per listener by AmplitudeQ8 (no decode)
        pace: per-connection byte budget per tick
    └─> per listener: Client_ReceiveVoiceFrames(...)        UNRELIABLE RPC
                                 |
EVERY REMOTE CLIENT
  RPC body: enqueue only
    └─> FProcessor_VoiceTalker_Receive  seq-order into jitter buffer,
        update speaking state + amplitude, fire signals     [game thread]
    └─> UCk_VoiceChatSynthComponent::OnGenerateAudio        [audio render thread]
        SPSC ring consumer: decode (IVoiceDecoder, persistent per-talker
        state), Opus PLC on gaps, adaptive playout depth
    └─> spatialization: per-channel USoundAttenuation + source-effect chain;
        position driven by FTag_Transform_Updated (CkAudio SpatialUpdate pattern)
```

Control plane (channel registry, memberships, mute matrix) replicates via the standard
replicated-container fragment path; the audio data plane rides relay-actor RPCs and is never
stored.

---

## 6. Decision records

Each ADR: decision, rejected alternative(s), and the reason. All are open to review; ADR-4
explicitly needs a doctrine adjudication.

### ADR-1 — Own the stack on engine primitives; no managed service

**Decision:** build capture→codec→transport→playback in-module from engine primitives.
**Rejected:** EOS RTC / Vivox / ODIN (or wrapping them).
**Why:** managed services provide no server-side positional culling (positional audio, where it
exists at all, is client-side rendering of streams everyone receives — [CONFIRMED] for EOS, which
documents no positional support; Vivox/ODIN do positional but are hosted/paid); they require
backend connectivity (breaks LAN/offline and deterministic tests); and a framework module cannot
couple CkFoundation to a third-party service contract. Consuming projects that already ship an
EOS-based kit can still adopt one independently — this module competes on composability and
offline-safety, not on hosted infrastructure.

### ADR-2 — Capture + codec via the engine `Voice` module factories

**Decision:** `FVoiceModule::Get().CreateVoiceCapture(DeviceName, 48000, 1)` /
`CreateVoiceEncoder(48000, 1, VoiceEncode_Voice)` / `CreateVoiceDecoder(48000, 1)`.
**[CONFIRMED]** in the fork (UnrealEngine-Angelscript, UE 5.7.4): factories at
`Engine/Source/Runtime/Online/Voice/Public/VoiceModule.h:70/81/91`; `IVoiceEncoder`/`IVoiceDecoder`
at `Public/Interfaces/VoiceCodec.h:29/109`; Opus implementations are **private**
(`Private/VoiceCodecOpus.h`), so the factory surface is the supported route; libOpus vendored at
`Engine/Source/ThirdParty/libOpus` (1.3.1-12); `Voice.Build.cs:12-21` compiles capture out on
dedicated-server targets (`VOICE_MODULE_WITH_CAPTURE=0`) — the exact behavior we want, for free.
**Rejected:** linking libOpus directly (re-implements what the factories give: bitrate/VBR/
complexity control + `STATGROUP_Voice` hooks); `UAudioCaptureComponent` as primary capture
(heavier, audio-mixer-coupled; remains a candidate backend behind the capture seam, §7.6).
**Note:** gain and gating are applied by **our** codec layer — the engine's `voice.MicInputGain`
path is broken upstream in some versions ([CONFIRMED] via reference-plugin changelog that ships a
replacement CVar).

### ADR-3 — Custom `USynthComponent` subclass + own jitter buffer for playback

**Decision:** `UCk_VoiceChatSynthComponent : USynthComponent` with an adaptive jitter buffer
(start ≈60 ms, grow/shrink from inter-arrival variance), sequence reordering, Opus PLC on gaps.
**Rejected:** engine `UVoipListenerSynthComponent`.
**Why:** the engine component drags in the OnlineSubsystemUtils plugin and carries the known
seamless-travel GC bug (**UE-146893** [CONFIRMED], engine issue tracker); its buffering is opaque
and was still being tuned by reference plugins in 2026. A pull-model synth is small, and owning it
makes the jitter policy unit-testable (§7.5) and the travel lifecycle ours.
Component ownership copies the CkVfx pattern exactly [CONFIRMED]: `TStrongObjectPtr` member
(`CkVfxCue_Fragment.h:39`), Setup creates with `bAutoActivate=false`, a monitor observes and
never destroys, EndPlay destroys + resets (`CkVfxCue_Processor.cpp:310-327`). Spatial update
copies `FProcessor_AudioTrack_SpatialUpdate` (`CkAudioTrack_Processor.h:158` — `FTag_Transform_Updated`-gated).

### ADR-4 — Transport: relay-actor RPCs (Unreliable), paced — **needs doctrine adjudication**

**Decision:** `ACk_VoiceChatRelay_UE : ACk_ActorRelay_UE`, one channel per player (subsystem =
two overrides, per the CkRenderTarget subsystem [CONFIRMED] `CkRenderTargetRelay_Subsystem.h:12`).
Voice RPCs are **Unreliable** (CkCue precedent — `CkCueRelay_Actor.h:24/62`), bodies **enqueue
only** and never apply inline, and the sender is **stamped server-side from the channel owner**
(both [CONFIRMED] as the documented CkRenderTarget relay contract,
`CkRenderTargetRelay_Actor.h:16-17, :58-59`). Per-player stream state + per-tick byte budget copy
`FFragment_RenderTarget_HostStreams` / `FProcessor_RenderTarget_PaceStreams`
(`CkRenderTarget_Fragment.h:280/296`, `CkRenderTarget_Processor.h:394`) [CONFIRMED].
**Tension to adjudicate:** `CkActorRelay/CLAUDE.md` states relays are for events, not
high-frequency data. CkRenderTarget already bends this with explicit pacing and budgets; voice is
the same shape with far smaller payloads (~200 B per RPC vs 32 KB chunks). Either the doctrine
gains a "paced budgeted streams are permitted" clause, or ADR-4 falls back to the alternative.
**Rejected (for now):** a custom `UChannel` (the Onset approach). Technically the cleanest
transport (no actor/RPC overhead), but it adds fork-coupled netcode, unknown Iris interactions,
and channel-registration fragility (`ChannelDefinitions` ini + `StaticChannelIndex` collisions
were that product's worst documented setup pain). Revisit if profiling shows relay overhead
matters at target scale.
**Sizing [CONFIRMED math]:** no chunking or compression is needed — a 20 ms Opus frame at 24 kbps
is ~60 B; a 3-frame bundle plus 7-byte header is ~190 B, orders of magnitude under the 64 KB bunch
ceiling CkRenderTarget's chunker exists for.

### ADR-5 — Channels are entities, with per-channel audio config

**Decision:** `FCk_Handle_VoiceChannel` entities identified by `FGameplayTag`, each carrying:
spatialization policy (`Positional3D` | `Global2D` | `HybridRadio`), `TSoftObjectPtr<USoundAttenuation>`,
`TSoftObjectPtr<USoundEffectSourcePresetChain>`, audible range (drives the routing probe),
priority, and a membership Record with per-member `CanTalk`/`CanListen` flags. "Proximity chat" is
simply a default auto-joined `Positional3D` channel; dead-chat/spectator are membership-flag
configurations, not modes. `HybridRadio` = Onset's "prefer 3D if near": one wire copy, played
positionally when the speaker is within proximity range, as filtered 2D radio otherwise.
**Rejected:** integer channel ids with global audio config (every reference does this; per-channel
attenuation/effects is their most-requested missing feature), and asset-swapping as mode switching
(Antize).
**Consequences:** the fix for cook-breaking string paths (soft refs resolved via CkResourceLoader);
walkie-talkie roger-beep/bip polish attaches per-channel via CkCue.

### ADR-6 — Proximity routing set: persistent probe, event-driven

**Decision:** one persistent sphere probe per transmitting talker (CkSpatialQuery/Jolt:
`ECk_MotionType` per talker mobility, `PersistContacts::Enabled`, probe name
`Probe.VoiceChat.Range`), with `BindTo_OnBeginOverlap`/`OnEndOverlap` maintaining the candidate
listener set incrementally, plus ±hysteresis on range so speakers don't pop mid-word. Final send
set = probe set ∩ channel membership ∩ mute matrix, minus the sender's own connection, capped
top-N talkers per listener by reported amplitude (server-side, **no decode** — G6).
**Rejected:** per-frame `Get_CurrentOverlaps` polling (explicit CkSpatialQuery anti-pattern) and
anything resembling world scans in the packet path (the confirmed root cause of a reference
plugin's large-level lag/crashes).
**Filter-direction note:** probe matching is `ProbeName.MatchesAny(Filter)` — documented past bug
in `CkSpatialQuery/CLAUDE.md`; the implementation must not invert it.

### ADR-7 — Deferred: engine `IVoiceChat` facade

The engine's modern voice abstraction (`IVoiceChat`/`IVoiceChatUser`) is implementable by third
parties and would make CkVoiceChat a drop-in provider for engine-level consumers. Deferred: its
login/session-shaped semantics fit poorly over ECS composition, CkFoundation's three-environment
Utils surface is the actual product, and an adapter can be added later without reshaping the
module. Recorded so the fork isn't re-litigated.

---

## 7. Detailed design

### 7.1 Module topology

- **Name:** `CkVoiceChat` (Tier 4). Collision-checked [CONFIRMED]: the engine owns module name
  `Voice`; no `Voice|Voip|Mic|Speech` identifiers exist in CkFoundation Source (2 false positives
  are local Material variables in CkParticles).
- **Ck deps:** `Core, Ecs, EcsExt, Label, Log, Record, Settings, Profile, ActorRelay,
  SpatialQuery, Relationship, ResourceLoader, Timer` (+`CkCue` if the roger-beep polish lands in
  the module rather than as a consumer recipe — reviewer call).
- **Engine deps:** `AudioMixer` (public — `USynthComponent`), `AudioCaptureCore`, `Voice`,
  `NetCore`, `DeveloperSettings` (private). All present in the fork [CONFIRMED §ADR-2 + agent
  inventory of `Engine/Source/Runtime/AudioCaptureCore`, `AudioMixer/Public/Components/SynthComponent.h`].
- **Layout** (CkTimer quartet × three features + the CkRenderTarget-style subfolders):

```
Source/CkVoiceChat/
  CkVoiceChat.Build.cs, CkVoiceChat_Module.{h,cpp}, CkVoiceChat_Log.{h,cpp},
  CkVoiceChat_Stats.h, Claude.md
  Public/CkVoiceChat/
    VoiceTalker/   CkVoiceTalker_{Fragment_Data,Fragment,Processor,Utils}.{h,cpp}
    VoiceChannel/  CkVoiceChannel_{Fragment_Data,Fragment,Processor,Utils}.{h,cpp}
    VoiceListener/ CkVoiceListener_{Fragment_Data,Fragment,Processor,Utils}.{h,cpp}
    Codec/         CkVoiceChat_Codec.{h,cpp}        (pure functions; no UObjects/ECS)
    Net/           CkVoiceChatRelay_Actor.{h,cpp}, CkVoiceChatRelay_Subsystem.{h,cpp},
                   CkVoiceChat_RepData.{h,cpp}, CkVoiceChat_Replication.cpp
    Playback/      CkVoiceChatSynth_Component.{h,cpp}
    Settings/      CkVoiceChat_Settings.{h,cpp}
```

### 7.2 Entity model

**VoiceTalker** — composed onto any transform-bearing entity (typically a player's pawn entity;
also valid on AI/NPC entities later — nothing player-specific in the data model).

- ParamsData: transmit mode (`ECk_VoiceChat_TransmitMode: PushToTalk | OpenMic | Disabled`),
  input gain (`float`, default **1.2**), VAD threshold (default **0.07** — both defaults adopted
  from the best-documented reference and re-validated in the gym), auto-join channel tags,
  optional attach socket name for the playback component (mouth/head).
- Current fragment (friend-scoped per house encapsulation): capture handle + encoder state
  (local owner only), `TStrongObjectPtr<UCk_VoiceChatSynthComponent>` (remote clients),
  outbound frame queue, `uint16 _Seq`, quantized amplitude, speaking flag + hysteresis timers.
- Tags: `FTag_VoiceTalker_NeedsSetup`, `_IsTransmitting`, `_IsSpeaking` (+ counted where needed).
- Requests: `Request_StartTransmit(ChannelTags)`, `Request_StopTransmit`,
  `Request_SetTransmitMode`, `Request_SetInputGain`, `Request_SetSelfMute` — all deferred, all
  carrying the standard trailing completion delegate.
- Signals: `OnTransmitStarted/Stopped` (local), `OnSpeakingStateChanged` (all machines, driven by
  real frame flow — never by the PTT key), `OnVoiceFramesCaptured` (local raw-PCM tap — the
  reference-validated extension point for STT/custom DSP).

**VoiceChannel** — child entity under whatever long-lived host entity the consumer chooses
(`UCk_Utils_VoiceChannel_UE::Add(HostHandle, ParamsData)`), labeled by its channel tag,
connected via a Record on the host (the CkAudio Director→Track topology [CONFIRMED]
`CkAudioTrack_Utils.cpp:19-38`).

- ParamsData: channel tag, spatialization policy, audible range, attenuation + effect-chain soft
  refs, priority, `bAutoJoinTalkers` (the world-proximity channel sets this).
- Requests: `Request_Join(Talker, MemberFlags)`, `Request_Leave(Talker)`,
  `Request_SetMemberFlags`, `Request_ServerMute(Talker)` / `Request_ServerUnmute(Talker)` —
  authority-validated.
- Signals: `OnMemberJoined/Left`, `OnMemberSpeakingChanged`.

**VoiceListener** — composed on the local player's ears entity (camera or pawn).

- Requests: `Request_MuteTalker` / `Request_UnmuteTalker` (client mute — also replicated upstream
  as a routing exclusion so muted audio is never sent at all; privacy property: mute means the
  server stops forwarding, not that the client discards), `Request_SetTalkerVolume(Talker, Mult)`.
- Getters: `Get_AudibleTalkers`, `Get_IsTalkerMuted`, `Get_TalkerVolume`.

### 7.3 Wire format and routing

RPC payload (`Codec/` pack/unpack, unit-tested):

```
Header  { uint16 Seq;  uint8 ChannelIdx;  uint8 AmplitudeQ8;  uint8 NumFrames; }
Frames  NumFrames × { uint16 EncodedSize; uint8 Data[EncodedSize] }   // Opus, 20 ms each
```

- `ChannelIdx` indexes the replicated channel registry (tag → u8), avoiding per-packet
  `FGameplayTag` serialization. `FCk_Handle` identifies the talker entity in the RPC signature —
  handles are RPC-serializable [CONFIRMED: `FCk_HandleNetSerializer`; every existing relay RPC
  passes `FCk_Handle InOwnerEntity`].
- Cadence: 2–3 frames per RPC → ~16–25 RPCs/s while speaking.
- Server routing is a single processor (`FProcessor_VoiceChat_Route`, AuthorityOnly) draining the
  relay inbox; it re-validates every packet against the authorization state (membership, CanTalk,
  server mute, channel range clamp — the reference-validated anti-cheat gates) and **drops
  invalid traffic silently after one ensure per offending talker** (rate-limited diagnostics, not
  log spam).
- Listen-server host: the host's own capture output is injected directly into the routing inbox
  (no self-RPC); a talker's frames are never forwarded back to their own connection; host
  self-monitoring exists only via the loopback CVar.

**Budgets [CONFIRMED math; targets, to be measured per non-negotiable #7]:**
per talker→listener stream ≈ 3.2–4 kB/s including RPC overhead; a client capped at N=8 audible
talkers receives ≤ ~32 kB/s; server outbound scales with Σ(listeners per talker), bounded by the
per-connection budget and the top-N cap. Mouth-to-ear ≈ frame(20) + bundle(≤40) + net + jitter(60
adaptive) + render quantum — inside the 100–150 ms target on LAN/regional RTTs.

### 7.4 Replication (control plane)

`FCk_RepData_VoiceChat` — channel registry (tag, u8 index, policy, range), memberships + member
flags, server-mute matrix. Registered `Register_NetOnly` (nothing is ever saved) via a file-static
module-prefixed registrar, `NetApply` returning `NotReady` until the voice features are composed —
the standard contract [CONFIRMED exemplar `CkTeam_Fragment.cpp:11-41`; four named shapes at
`CkPersistenceHandlerRegistry.h:132-145`]. Late joiners therefore receive the full control plane
before any audio; audio itself needs no catch-up (streams simply resume).

### 7.5 Threading model

- **Capture thread → game thread:** the engine capture object buffers; a Talker processor drains
  on tick (house pattern; capture cadence is 20 ms against a ~8 ms tick, so drain-on-tick adds ≤1
  frame of latency).
- **Encode/route on game thread:** Opus encode of one 20 ms mono frame is microseconds-scale
  [INFERRED from codec literature; confirmed by the P1 benchmark before any claim is made].
  Profiled under `CkVoiceChat_Stats.h` counters from day one.
- **Game thread → audio render thread:** per-talker lock-free SPSC ring of encoded frames;
  `OnGenerateAudio` (pull) pops, decodes with the talker's **persistent** decoder (PLC requires
  per-stream state — decoders are per-talker, pooled, never shared), conceals gaps, adapts playout
  depth. No locks, no allocation on the audio thread after warm-up.

### 7.6 The capture seam (testability)

Capture is isolated behind a minimal in-module interface (`ICk_VoiceChat_CaptureSource`: start,
stop, drain-PCM) with two implementations: the engine `IVoiceCapture` wrapper, and a fake that
injects PCM fixtures (sine sweeps, recorded phrases, silence). Every automated test runs the full
pipeline — gate → encode → wire → route → jitter → decode — headless with the fake; only the
final "real microphone works" check is a human `[EDITOR-VERIFY]` gym step with loopback.

### 7.7 Settings and CVars

`UCk_VoiceChat_ProjectSettings_UE : UCk_Plugin_ProjectSettings_UE` (pattern + constexpr fallbacks
per `CkRenderTarget_Settings.h` [CONFIRMED]): sample rate, bitrate, VBR, frame ms, frames-per-RPC,
jitter min/target/max, per-connection byte budget, MaxAudibleSpeakers (default 8), default
attenuation soft ref, hysteresis margin. CVars via CkCVar: `ck.VoiceChat.Loopback`,
`ck.VoiceChat.Debug` (overlay: transmit/receive state, seq gaps, buffer depth, per-talker
bandwidth), `ck.VoiceChat.MuteAll`, `ck.VoiceChat.InputGain` (session override).

### 7.8 Lifecycle

- Setup/monitor/EndPlay component discipline per CkVfx; `FGroup_EndPlay` cancel-pending processor
  fires `Failed_Cancelled` on queued requests (CkTimer contract).
- Teardown mid-transmit: EndPlay stops capture before releasing the encoder, flushes the routing
  entry, destroys synth components on all clients (driven by the talker entity's own teardown —
  no orphaned audio). Explicit test.
- Seamless travel: relay channels are world-scoped and respawned by the ActorRelay base subsystem
  flow; talker/channel entities re-compose in the new world; test asserts voice resumes without
  manual re-registration (the failure every reference shipped).
- Dedicated server: capture and playback never initialize (`VOICE_MODULE_WITH_CAPTURE=0` +
  NetMode-gated processors); routing is pure pass-through.

---

## 8. Doctrine compliance map

| Non-negotiable | How the design satisfies it |
|---|---|
| Research before writing | This document; scaffold by mimicry of CkTimer/CkVfx/CkRenderTarget/CkAudio, cited per section |
| `CK_ENSURE_IF_NOT` + separate early-out | All request validation and packet-path validation; rate-limited ensures in the routing hot path |
| Never silently handle an error | Invalid packets ensure-once-then-drop with explicit rejection state; no fallbacks |
| Three environments | Utils-only public surface; delegates carry `AutoCreateRefTerm`; AS wrappers generated; P5 gate verifies all three |
| Requests are deferred | Every mutation is a `Request_*` drained by processors; completion delegates per the request-completion contract |
| Typesafe handles via `Cast/CastChecked` | `FCk_Handle_VoiceTalker/VoiceChannel/VoiceListener` declared in `_Fragment_Data.h` |
| Signals are local-only | All `OnSpeaking*`/`OnMember*` signals fire from local processors reacting to local state/frame flow |
| Measure before claiming | §7.3/7.5 numbers are targets; P1/P3 gates include benchmarks before any performance claim |

---

## 9. Testing strategy

| Layer | Coverage |
|---|---|
| C++ unit (CkTests) | `Codec/` pure layer: header pack/unpack roundtrip, VAD attack/release against PCM fixtures, jitter-buffer reorder/gap/PLC-decision/adaptive-depth, pacing math, amplitude quantization |
| AutoTest (headless PIE) | Compose/teardown of all three features; request validation incl. invalid-input rejection tests per validation boundary (non-negotiable #3); channel join/leave/flags; mute semantics matrix; auto-join; teardown mid-transmit; fake-capture full-pipeline loopback |
| Net tests (two-world) | Membership/mute replication + late join; routing: forward-only-in-range with hysteresis, sender exclusion, server-mute stops forwarding (not just playback), top-N culling under synthetic load; listen-server host paths (host talks / host hears / client↔client) |
| Gym `[EDITOR-VERIFY]` | Real microphone loopback; spatialization/attenuation audition; HybridRadio audition; debug overlay |
| Packaged smoke `[EDITOR-VERIFY]` | Opus init + capture + loopback in a packaged client (the documented packaged-only Opus failure class) |

---

## 10. Delivery plan

| Phase | Content | Gate |
|---|---|---|
| P0 | Module skeleton, uplugin entry, Build.cs, settings, native tags; **spike: Unreliable unicast RPC semantics on relay actors under the fork's net stack** (the one load-bearing [INFERRED] assumption) | Compiles in all three environments; spike memo |
| P1 | `Codec/` pure layer + unit tests + encode/decode micro-benchmark | Headless unit green; benchmark recorded |
| P2 | VoiceTalker + capture seam + local loopback playback (no net) | AutoTests green with fake capture; `[EDITOR-VERIFY]` mic loopback |
| P3 | Relay transport, routing processor, Positional3D + Global2D channels, control-plane replication | Net tests green incl. late-join + host asymmetry |
| P4 | HybridRadio, per-channel attenuation/effect chains, roger-beep/bip polish, moderation matrix, amplitude/speaking polish | AutoTest + net green; radio audition |
| P5 | Hardening (seamless travel, teardown storms, packaged smoke), docs (module `Claude.md`, `Source/CLAUDE.md` row + decision-tree entries), AS/BP surface verification | Full suite delta-zero vs baseline; docs merged |

Execution routing: design/audit at top tier; P0–P5 implementation delegated with per-phase gate
review (phase-gate docs per `ck-methodology` once approved).

---

## 11. Risks

| Risk | Severity | Mitigation |
|---|---|---|
| ADR-4 doctrine conflict (relay "events only") | Design-blocking if adjudicated against | CTO adjudication requested here; fallback = custom UChannel (scoped spike exists in Onset precedent) |
| Iris/unreliable-RPC semantics differ on 5.7 fork | High if wrong | P0 spike before any dependent code |
| Audio-thread underrun artifacts (robotic voice) from mis-tuned jitter defaults | Medium | Unit-tested adaptive policy + debug overlay depth metrics + gym audition |
| Encode cost on game thread at high talker counts | Low at target scale | Stats counters from day one; P1 benchmark; task-graph offload is a contained refactor if measured necessary |
| Mic permission/platform variance | Deferred by scope | Capture seam isolates it; Windows-only v1 stated openly |
| AS codegen for the new delegate surface | Low | Standard generator path; P5 gate verifies |

---

## 12. Open questions for review

1. **ADR-4 adjudication** — bless "paced, budgeted streams over relay actors" as doctrine
   (RenderTarget + VoiceChat as the two adopters), or direct the custom-UChannel route?
2. Module name: `CkVoiceChat` (recommended) vs `CkProximityVoice`.
3. Channels-as-entities from day one (recommended) vs proximity-only v1.
4. Does the roger-beep/bip polish live in-module (CkCue dep) or as a consumer recipe in docs?
5. `VoiceListener` as a separate feature (recommended — keeps per-listener state off the talker)
   vs folding into VoiceTalker.
6. MaxAudibleSpeakers default (proposed 8) and whether the cap is server-only or also client-side.

---

## Appendix A — Verified engine symbols (fork: UnrealEngine-Angelscript, UE 5.7.4)

| Capability | Location | Key symbols |
|---|---|---|
| Voice factories | `Engine/Source/Runtime/Online/Voice/Public/VoiceModule.h:70-98` | `CreateVoiceCapture`, `CreateVoiceEncoder`, `CreateVoiceDecoder`, `DoesPlatformSupportVoiceCapture` |
| Codec interfaces | `.../Voice/Public/Interfaces/VoiceCodec.h:29,109` | `IVoiceEncoder`, `IVoiceDecoder` (Opus impls private) |
| Capture interface | `.../Voice/Public/Interfaces/VoiceCapture.h` | `IVoiceCapture` |
| Dedicated-server guard | `.../Voice/Voice.Build.cs:12-21` | `VOICE_MODULE_WITH_CAPTURE=0` on Server targets |
| libOpus | `Engine/Source/ThirdParty/libOpus/` | opus-1.3.1-12 (also 1.0.3, 1.1) |
| Synth base | `Engine/Source/Runtime/AudioMixer/Public/Components/SynthComponent.h` | `USynthComponent::OnGenerateAudio` |
| Low-level capture | `Engine/Source/Runtime/AudioCaptureCore/` | backend-agnostic capture core |

## Appendix B — In-house precedents copied (all verified by inspection 2026-08-02)

| Pattern | Source |
|---|---|
| Relay RPC contract (enqueue-only bodies, server-side sender stamping, reliable-channel note) | `CkRenderTarget/.../Net/CkRenderTargetRelay_Actor.h:16-17, :29-68` |
| Per-player stream map + byte-budget pacing + relevancy gate | `CkRenderTarget_Fragment.h:280-324`; `CkRenderTarget_Processor.h:364-511`; `.cpp:1501-1540` |
| Unreliable relay RPCs | `CkCue/CkCueRelay_Actor.h:24,62,74` |
| Relay group subsystem (2-override) + per-player channels | `CkRenderTargetRelay_Subsystem.h:12`; `CkActorRelay_GroupSubsystem.h:62-106` |
| Component lifetime (`TStrongObjectPtr`, Setup/Monitor/EndPlay, AutoActivate=false) | `CkVfxCue_Fragment.h:26-49`; `CkVfxCue_Processor.cpp:34,255,310-327` |
| Spatial update via `FTag_Transform_Updated`; attenuation resolution chain | `CkAudioTrack_Processor.h:158`; `.cpp:139-245,723-725` |
| Director→child-entity Record topology | `CkAudioTrack_Utils.cpp:19-38` |
| Probe overlap signals, persistent contacts, filter direction | `CkProbe_Utils.h:36-51,146-178,237-273`; `CkProbe_Fragment_Data.h:124-182,635-791`; `CkSpatialQuery/CLAUDE.md` |
| Net-only rep registrar | `CkTeam_Fragment.cpp:11-41`; `CkPersistenceHandlerRegistry.h:132-145` |
| ProjectSettings + constexpr fallback | `CkRenderTarget/Settings/CkRenderTarget_Settings.h:11-88` |
| Request-completion + EndPlay cancellation | `CkTimer` (contract owner); `CkAudioTrack_Processor.h:107` |

## Appendix C — External sources (primary)

Engine: UE Voice Chat Interface docs; `UVoipListenerSynthComponent` 5.7 API; issue **UE-146893**;
`UAudioCapture`/`UAudioCaptureComponent` 5.7 API; `USoundWaveProcedural`;
`FSoundAttenuationSettings::OcclusionLowPassFilterFrequency`. Codec: opus-codec.org encoder docs
(frame sizes, FEC/PLC). Services (comparison): EOS Voice overview + Web API; Vivox positional
channel properties (defaults: ConversationalDistance 90, AudibleDistance 2700, InverseByDistance);
ODIN Unreal C++ guide; Steam Voice; Mumble Link; Discord Social SDK. Community failure corpus:
Epic forums threads on packaged-Opus failure, seamless-travel VOIP loss, listen-server VOIP
asymmetries; the four reference products' manuals, changelogs, review corpora (retained in the
research session digests).
