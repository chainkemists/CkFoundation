# CkVoiceChat — Proximity Voice Chat Module — CTO Design Review

> **Workflow:** Review the brief below, then fill in the **CTO Review Response** section at the bottom of this file. Commit your changes — the design author's assistant will pick up your notes from there.

> **Pre-implementation review.** No code has been written. The artifact under review is the **design spec** (7 ADRs + full module design + phased plan). If you flag a blocker now, we revise the spec; if you green-light, implementation proceeds P0-first with per-phase gate review. One item is explicitly a **doctrine adjudication request**, not just a design check: ADR-4 asks you to rule on whether paced, budgeted data streams over relay actors are permitted doctrine (see §A and §F below).

---

## Reviewer brief

### Your role

Senior reviewer / architect. You are reviewing the design for **CkVoiceChat**, a proposed new Tier-4 runtime module: microphone capture, Opus encoding, server-routed transport, and spatialized playback as three composable ECS features. Specifically:

1. Catch architectural issues that would be expensive to discover mid-build — the design commits to a transport (relay-actor RPCs), a threading model (game-thread encode, audio-render-thread decode), and a wire format before any code exists.
2. **Adjudicate ADR-4** — the relay-actor transport conflicts with the letter of `CkActorRelay`'s "events, not high-frequency data" doctrine; CkRenderTarget already bends it with pacing/budgets. Bless the pattern, or direct the custom-UChannel fallback.
3. Check convention compliance against the doctrine of record — the spec claims mimicry of CkTimer/CkVfx/CkRenderTarget/CkAudio with file:line citations throughout; spot-check the ones that carry the most weight (§D).
4. Rule on the six open questions in spec §12 — they are deliberately unsettled and your ruling is wanted, not just review.
5. Either green-light for implementation, or list specific blocking concerns.

You are expected to **read code in the repo** — don't review the spec in isolation. Every load-bearing claim in the spec is marked **[CONFIRMED]** (with citation) or **[INFERRED]** (with what would confirm it); the one load-bearing [INFERRED] is the P0 spike subject.

### What's being built

A self-contained proximity voice chat stack built entirely on engine primitives the fork already ships (engine `Voice` module capture/Opus factories, `USynthComponent` playback) and on transport patterns CkFoundation already proved (CkRenderTarget's paced relay-actor streaming, CkCue's unreliable relay RPCs). Zero coupling to OnlineSubsystem, sessions, or external services — works over any UNetDriver, listen or dedicated server, LAN/offline.

Three composable features: **VoiceTalker** (capture + transmit + remote playback), **VoiceChannel** (runtime channel entities with per-channel spatialization policy / attenuation / effect chain / membership flags), **VoiceListener** (per-talker client mute + receive volume). Headline differentiators over the four commercial references analyzed: proximity culls **bandwidth server-side** (not just volume client-side); channels are entities with per-channel audio config (the ecosystem's #1 unfulfilled request); real voice-activity signals (not PTT key state); and the community's recurring failure modes (seamless travel, late join, listen-server asymmetries, packaged-Opus init) are the test matrix from day one.

### Design spec location

[2026-08-02-CkVoiceChat-technical-review.md](../specs/2026-08-02-CkVoiceChat-technical-review.md)

### Critical context — read before reviewing

- `Plugins/CkFoundation/CLAUDE.md` + [Source/CLAUDE.md](../../Source/CLAUDE.md) — doctrine of record (non-negotiables, request-completion contract, tier table the new module's deps must respect).
- **The doctrine under tension:** [CkActorRelay/Claude.md](../../Source/CkActorRelay/Claude.md) — the "events, not high-frequency data" position ADR-4 asks you to adjudicate.
- **The transport precedent the design copies:** `Source/CkRenderTarget/Public/CkRenderTarget/Net/CkRenderTargetRelay_Actor.h` (enqueue-only RPC bodies, server-side sender stamping), `CkRenderTarget_Fragment.h:280-324` + `CkRenderTarget_Processor.h:364-511` (per-player stream map, byte-budget pacing), `CkRenderTargetRelay_Subsystem.h` (2-override relay subsystem). Unreliable-RPC precedent: `Source/CkCue/Public/CkCue/CueRelay/CkCueRelay_Actor.h:24,62`.
- **Component-lifetime pattern the playback copies:** `Source/CkVfx/Public/CkVfx/VfxCue/CkVfxCue_Fragment.h:26-49` + `CkVfxCue_Processor.cpp:310-327` (`TStrongObjectPtr`, Setup/Monitor/EndPlay, AutoActivate=false). Spatial update: `Source/CkAudio/Public/CkAudio/AudioTrack/CkAudioTrack_Processor.h:158` (`FTag_Transform_Updated`-gated).
- **Proximity routing substrate:** [CkSpatialQuery/Claude.md](../../Source/CkSpatialQuery/Claude.md) — persistent probes, overlap signals, and the documented probe filter-direction bug the spec calls out (ADR-6).
- **Net-only replication registrar exemplar:** `Source/CkEcs/Public/CkEcs/Net/CkTeam_Fragment.cpp` shape via `CkPersistenceHandlerRegistry.h:132-145` (`Register_NetOnly`, NotReady-before-mutation).
- **Engine surface (fork):** `Engine/Source/Runtime/Online/Voice/Public/VoiceModule.h:70-98` factories, `Voice.Build.cs:12-21` (`VOICE_MODULE_WITH_CAPTURE=0` on server targets), `Engine/Source/Runtime/AudioMixer/Public/Components/SynthComponent.h`. Engine root: `D:\Repositories\UnrealEngine-Angelscript` (UE **5.7.4**).

### Design decisions already settled (do NOT relitigate unless you see a real problem)

1. **Own stack on engine primitives; no managed service** (ADR-1) — EOS RTC/Vivox/ODIN provide no server-side positional culling, require backend connectivity (breaks LAN/offline/deterministic tests), and a framework module cannot couple CkFoundation to a third-party service contract.
2. **Capture + codec via engine `Voice` module factories** (ADR-2) — verified present in the fork; Opus impls are private so the factory surface is the supported route; direct libOpus linking rejected.
3. **Custom `USynthComponent` + own jitter buffer** (ADR-3) — engine `UVoipListenerSynthComponent` drags in OnlineSubsystemUtils and carries the UE-146893 seamless-travel GC bug; owning the jitter policy makes it unit-testable.
4. **Channels are entities with per-channel audio config** (ADR-5) — proximity = a default auto-joined Positional3D channel; dead-chat/spectator are membership-flag configurations, not modes.
5. **Probe-based event-driven routing set** (ADR-6) — persistent sphere probe + overlap signals + hysteresis; per-frame overlap polling and world scans in the packet path are explicitly rejected (the confirmed root cause of a reference plugin's large-level failures).
6. **`IVoiceChat` engine facade deferred** (ADR-7) — recorded so the fork isn't re-litigated; an adapter can be added later without reshaping the module.
7. **Canonical wire recipe** — 48 kHz mono Opus, 20 ms frames, 2–3 frames per RPC, adaptive jitter buffer starting ~60 ms, Opus PLC; amplitude quantized in the frame header so the server never decodes.
8. **CkFoundation-only scope** — the module ships in the framework with no game- or service-specific coupling.

**ADR-4 (relay-actor transport) is deliberately NOT settled** — it is the adjudication request.

### What I specifically want you to scrutinize

#### A. Architecture / decomposition

- **ADR-4, the load-bearing fork:** is "paced, budgeted streams over relay actors" acceptable doctrine (RenderTarget + VoiceChat as the two adopters), or does voice deserve the custom-UChannel route despite its fork-coupled netcode, unknown Iris interactions, and the channel-registration fragility documented in the Onset precedent?
- **Control-plane / data-plane ordering:** the spec claims late joiners "receive the full control plane before any audio" (§7.4) because audio RPCs reference a replicated channel registry. Property replication and unreliable RPCs have no cross-channel ordering guarantee — what happens when a client receives a voice packet whose `ChannelIdx` it can't resolve yet? Is drop-until-resolvable stated, and is it sufficient?
- **Self-reported amplitude drives server-side top-N culling** (§7.3): `AmplitudeQ8` is computed by the sending client. A modified client reporting max amplitude always survives the cull. The spec's authorization gates cover membership/mute/range but not this. Acceptable at target scale, or does the cap need a server-side fairness fallback (e.g. round-robin among maxed reporters)?
- **Routing per policy:** the probe set applies to `Positional3D`; confirm the routing formula for `Global2D`/`HybridRadio` (membership ∩ mute matrix, no probe) is well-defined and that HybridRadio's one-wire-copy client-side 3D/2D decision doesn't leak listeners the server thinks are out of proximity range.
- **Three-feature decomposition** — Talker/Channel/Listener the right cut (also open question 5)?

#### B. Convention compliance

- Quartet × 3 features + `Codec/`/`Net/`/`Playback/`/`Settings/` subfolders (§7.1) — matches the CkRenderTarget layout precedent?
- The Ck dep list (§7.1, 13 modules incl. `Relationship`, `Timer`, `ResourceLoader`) — tier-clean, and is each dep actually earned by the v1 scope? (`Relationship` in particular — team channels can be expressed as membership without it.)
- Requests all deferred with trailing completion delegates; typesafe handles declared in `_Fragment_Data.h`; signals local-only (§8 compliance map) — any gaps?
- **Packet-path validation shape:** "drops invalid traffic silently after one ensure per offending talker — rate-limited diagnostics" (§7.3). `CK_ENSURE_IF_NOT` fires once *per site* by design; per-talker rate-limiting is a different mechanism. Is the proposed shape compatible with non-negotiable #3 (ensure + separate ordinary early-out, no validation living only inside the macro)?

#### C. Version-specific API specifics

- **The P0 spike: unreliable unicast RPC semantics on relay actors under the fork's 5.7 net stack (Iris)** — the design's one load-bearing [INFERRED] assumption. Is a P0 spike the right gate, or does anything else deserve pre-P0 verification?
- The packaged-only Opus-init failure class is covered by a P5 `[EDITOR-VERIFY]` packaged smoke — late enough to be cheap, early enough to matter?
- `USynthComponent::OnGenerateAudio` pull-model contract + no-alloc/no-lock audio-thread discipline (§7.5) — anything 5.7-audio-mixer-specific the spec should pin?

#### D. Highest-weight spot-checks (the claims the design stands on)

- `CkRenderTargetRelay_Actor.h:16-17, :29-68` — the documented relay contract (enqueue-only bodies, server-side sender stamping) the voice relay copies.
- `CkRenderTarget_Processor.h:364-511` — the pacing/budget machinery cited as precedent for per-connection voice budgets.
- `CkVfxCue_Processor.cpp:310-327` — the Setup/Monitor/EndPlay component-lifetime discipline the synth component copies.
- `CkPersistenceHandlerRegistry.h:132-145` — `Register_NetOnly` exists as a named shape and fits the control-plane payload.
- `VoiceModule.h:70-98` + `Voice.Build.cs:12-21` (engine) — factories and the dedicated-server capture compile-out the design leans on.

#### E. Risks — sized correctly? (spec §11)

- ADR-4 adjudicated against = design-blocking; is the custom-UChannel fallback credible enough to be named as the mitigation, or should it get its own spike now?
- Jitter-buffer mis-tuning (robotic voice) rated Medium with unit-tested adaptive policy + overlay metrics + gym audition — sufficient, given every reference shipped multiple rounds of buffer re-tuning?
- Game-thread encode cost rated Low with stats-from-day-one + P1 benchmark — agree, or require the benchmark before P2?

#### F. Open questions (spec §12 — your ruling wanted, not just review)

1. **ADR-4 adjudication** — bless paced/budgeted relay streams as doctrine, or direct custom UChannel.
2. Module name: `CkVoiceChat` (recommended) vs `CkProximityVoice`.
3. Channels-as-entities from day one (recommended) vs proximity-only v1.
4. Roger-beep/bip polish in-module (adds CkCue dep) vs consumer recipe in docs.
5. `VoiceListener` as a separate feature (recommended) vs folding into VoiceTalker.
6. MaxAudibleSpeakers default (proposed 8); server-only cap or also client-side.

### Output format — fill in the CTO Review Response section below

Be direct. If the design is good, say so and green-light it — don't manufacture issues to look thorough. Specific blockers tied to spec sections, not vague concerns. If you rule CHANGES REQUESTED, give the exact, minimal sign-off conditions.

---

## CTO Review Response

### Verdict

**GREEN-LIGHT WITH NON-BLOCKING NOTES.** Implementation may start P0-first. One note (N1, the
§7.4 ordering over-claim) is a **standing condition of this green-light**: the spec amendment must
land before the P3 gate. Nothing below reshapes the architecture.

### ADR-4 adjudication

**Blessed: paced, budgeted streams over relay actors are permitted doctrine.** Custom UChannel
rejected for v1; it remains the named fallback. Reasoning:

1. **The doctrine tension is smaller than the brief frames it, because "relay" names two different
   mechanisms.** `CkActorRelay/CLAUDE.md`'s anti-pattern ("relay is for events, not high-frequency
   data — use fragment state for continuous data") was written about the **Broadcast/Bind event
   channel**. CkRenderTarget — and now CkVoiceChat — use the relay **actor** as a per-player
   replicated net endpoint to hang RPCs on. Different mechanism, and the anti-pattern's prescribed
   alternative is actively wrong here: replicated fragment state rides reliable, delta-serialized
   container replication, and retransmitting stale audio is strictly worse than dropping it. Voice
   is a third category the doctrine never anticipated — a lossy, paced, loss-tolerant stream — so
   the doctrine gains a clause rather than the design gaining an exception.
2. **Every load-bearing element already exists on this exact substrate** [CONFIRMED by
   inspection]: per-player channels via the 2-override group subsystem
   (`CkRenderTargetRelay_Subsystem.h:12`), enqueue-only RPC bodies + server-side sender stamping
   (`CkRenderTargetRelay_Actor.h:16-17, :58-59`), per-(player, stream) byte-budget pacing
   (`CkRenderTarget_Fragment.h:280-313`, `FProcessor_RenderTarget_PaceStreams` at
   `CkRenderTarget_Processor.h:393-435`), client-side retry until the channel resolves
   (`FProcessor_RenderTarget_ClientNetMaintenance` :509+), and Unreliable RPCs on a relay actor in
   production (`CkCueRelay_Actor.h:24, :62`). Voice's ~190 B @ 16–25 Hz per talker is two-plus
   orders of magnitude below the 32 KB chunks that machinery paces — capacity-wise this is the
   easy case of an already-proven pattern.
3. **The UChannel buys nothing measured and costs three known things:** fork-coupled netcode,
   Iris interaction unknowns (the relay CLAUDE.md's `bNetStartup`/path-serialization scar shows
   how deep that water is; a custom channel type under Iris is deeper), and the Onset-documented
   `ChannelDefinitions`/`StaticChannelIndex` registration fragility. Per non-negotiable #7,
   revisit only if the P3 profile shows relay overhead matters. **No pre-emptive UChannel spike**
   (answers §E bullet 1) — it would spend days de-risking a path we may never take; the named
   fallback is credible enough as a mitigation.
4. **The doctrine clause** (amend `CkActorRelay/CLAUDE.md`'s Anti-patterns section as part of P0 —
   cheap doc edit, so doctrine and code never disagree). A stream over relay actors is permitted
   iff ALL of: (a) reliability class chosen from payload semantics — Reliable for stateful
   must-apply payloads (RenderTarget: reliable ⇒ ordered ⇒ no resend/reorder logic), Unreliable
   for disposable time-sensitive payloads that tolerate silent loss end-to-end (voice: Opus PLC);
   (b) RPC bodies enqueue-only, never apply inline; (c) sender identity stamped server-side from
   the channel owner; (d) a per-connection byte budget drained by a pacing processor; (e) payload
   bounded well under the bunch ceiling (needing chunking ⇒ needing reliability ⇒ re-justify);
   (f) stats counters from day one; (g) an explicit, stated drop policy. Adopters: CkRenderTarget,
   CkVoiceChat. Keep the Broadcast/Bind events-only rule for the event-channel mechanism verbatim.
5. **The P0 spike stays the gate, sharpened.** What's actually unproven is the composite:
   **sustained-rate Unreliable unicast Client-direction RPCs under the fork's Iris stack** —
   CkCue proves Unreliable Server/Multicast relay RPCs at event rates; RenderTarget proves
   per-player unicast Client RPCs but Reliable. The spike memo must answer: delivery behavior
   under packet-fill pressure (does Iris coalesce/starve unreliable attachments), what happens to
   an unreliable RPC targeting a channel the client hasn't resolved yet (silent vanish is
   acceptable for voice — confirm it's silent, not an ensure/log storm), and per-RPC overhead
   (whether 25 Hz cadence should bundle more frames). If the spike shows pathological
   drop/starvation, ADR-4 re-opens with the UChannel fallback — that is the one outcome that
   overturns this ruling.

### Blocking issues

None that block P0. N1 below is mandatory before the P3 gate (standing condition of the verdict).

### Non-blocking suggestions

1. **[N1 — mandatory pre-P3] §7.4 over-claims cross-channel ordering.** "Late joiners receive the
   full control plane before any audio" is not a guarantee the transport provides: the channel
   registry replicates as container-fragment property data on one actor; voice rides unreliable
   RPCs on another. Iris gives no cross-object ordering, so a voice packet with an unresolvable
   `ChannelIdx` WILL arrive. Replace the sentence with an explicit rule: **drop unresolvable
   packets, count them in a stat/overlay counter, never stash** (contrast RenderTarget's chunk
   stash — pixels are stateful and must eventually apply; audio is disposable and the stream
   resumes). A P3 net test must cover voice-arrives-before-registry. Left as-is, the sentence
   becomes a flaky-by-design test invariant.
2. **§7.3 validation wording conflicts with how `CK_ENSURE_IF_NOT` works.** "One ensure per
   offending talker" isn't a thing — ensures fire once per **site**. Shape it per non-negotiable
   #3: hoist the validity computation, ensure with empty body (fires once per site as the
   debugger breadcrumb), separate ordinary always-on drop branch, and per-talker **attribution**
   via throttled `ck::voice_chat` Warning + a stat counter. Validation must never live only
   inside the macro (`CK_DISABLE_ENSURE_CHECKS` compiles it out).
3. **`ChannelIdx` (u8) reuse hazard.** Runtime channels can be destroyed; if the registry reuses
   a freed index, an in-flight unreliable packet stamped with the old idx can resolve to the
   reused channel. Server-side membership re-validation catches most misroutes, but state the
   policy in the spec: registry indices are session-append-only (ensure on wrap past 255), or
   pair the idx with an epoch byte. One line now saves a subtle P4 bug.
4. **Dep list trim (§7.1).** Cut `Relationship` — nothing in the design queries attitude; team
   channels are membership-flag configurations by ADR-5's own argument. Audit `Timer` at P0:
   speaking-flag hysteresis and VAD attack/release are internal bookkeeping, which the house
   decision tree routes to `FCk_Chrono` (CkCore), not timer entities — keep the dep only if a
   real timer-entity use materializes. Every dep must be earned at the P0 Build.cs review.
5. **Pull one packaged Opus-init smoke forward to the P2 gate** (capture + codec + playback all
   live locally, pre-transport), keeping the full P5 packaged smoke. The packaged-only Opus
   failure class is the community's marquee failure; discovering it at P5 would sit under three
   phases of transport work built on the factory route. One extra packaging run is cheap
   insurance at the right time.
6. **§7.5 synth pin [INFERRED — verify against 5.7 `SynthComponent.h` at P2]:** initialize the
   synth at 48 kHz mono and let the audio mixer's SRC own device-rate conversion — never resample
   inside `OnGenerateAudio`. Also pin: Start/Stop are game-thread-only, and the SPSC ring must
   survive Stop→Start without tearing (re-entry of the render consumer).
7. **Routing-policy matrix in the module `Claude.md` at P3.** The per-policy send-set formula
   (Positional3D: probe ∩ membership ∩ mute; Global2D/HybridRadio: membership ∩ mute, no probe)
   is well-defined in the spec but scattered — one table in the module doc prevents the next
   engineer from "fixing" HybridRadio by adding a probe to it.
8. **Brief citation drift (this file, not the spec):** `CkCueRelay_Actor.h` lives at
   `CkCue/Public/CkCue/` (no `CueRelay/` subfolder); VfxCue files live under `CkVfx/Public/CkVfx/Cue/`
   (not `VfxCue/`); the Team registrar lives in `CkRelationship/Public/CkRelationship/Team/CkTeam_Fragment.cpp`
   (not CkEcs/Net) and uses `Register_NetAndSave_SharedApply` (the `Register_NetOnly` shape itself
   is confirmed at `CkPersistenceHandlerRegistry.h:131-133`). Line numbers were correct in all
   three. Cosmetic — fix if this doc is kept as a record.

### Rulings on open questions 2–6

2. **`CkVoiceChat`.** The module is more than proximity (G2's whole channel matrix);
   `CkProximityVoice` would misname it. Collision check re-verified clean by grep.
3. **Channels-as-entities from day one.** Proximity-only v1 is a false economy — retrofitting
   channel entities reshapes the wire format (`ChannelIdx`) and the routing processor, the two
   most expensive things to churn.
4. **Consumer recipe in docs; no CkCue dep in v1.** The beep is consumer-flavor presentation, not
   voice-transport identity (non-negotiable #9 cuts both ways). Reword P4: ship the documented
   recipe (bind `OnTransmitStarted/Stopped` → cue) + gym audition. Promoting it in-module later
   is additive and cheap if demand shows.
5. **Yes — `VoiceListener` stays a separate feature.** Per-listener mute/volume is local-machine
   state with its own lifetime (ears entity); folding it into Talker smears local state onto a
   replicated feature — the exact shape the CkPoi v2 case study warns about.
6. **Default 8, server-only cap in v1.** The client only receives what the server forwards, so a
   client-side cap is redundant; per-client decode cost is bounded by the same N. Revisit only if
   profiling shows client decode hot. 8 matches the reference-validated range and human
   simultaneous-speaker comprehension limits.

### Convention compliance spot-checks performed

- `CkActorRelay/CLAUDE.md` — the doctrine under tension (full read).
- `CkRenderTarget/Public/CkRenderTarget/Net/CkRenderTargetRelay_Actor.h` (full) — enqueue-only +
  server-side stamping contract comments at :16-17, :58-59 confirmed verbatim.
- `CkRenderTargetRelay_Subsystem.h` — 2-override subsystem at :12 confirmed.
- `CkRenderTarget_Fragment.h:270-329` — `FCk_RenderTarget_PlayerStream` :280,
  `FFragment_RenderTarget_HostStreams` :296 (per-player `TMap`, budget-drained) confirmed.
- `CkRenderTarget_Processor.h:360-514` — `DispatchPixelPayload` :364, `PaceStreams` :394 with
  per-stream per-tick byte-budget comment, `ClientNetMaintenance` retry :509+ confirmed.
- `CkCue/Public/CkCue/CkCueRelay_Actor.h` — Unreliable Server RPC :24, Unreliable Multicast :62
  confirmed (path drift noted in suggestion 8).
- `CkVfx/Public/CkVfx/Cue/CkVfxCue_Fragment.h:26-49` — friend-scoped
  `TStrongObjectPtr<UNiagaraComponent>` at :39 confirmed.
- `CkVfx/Public/CkVfx/Cue/CkVfxCue_Processor.cpp:309-327` — EndPlay destroy + `.Reset()`
  discipline confirmed.
- `CkEcs/Public/CkEcs/Persistence/CkPersistenceHandlerRegistry.h:100-178` — `Register_NetOnly` +
  designated-init args structs + compile-enforced required slots confirmed.
- `CkRelationship/Public/CkRelationship/Team/CkTeam_Fragment.cpp:11-41` — file-static registrar
  exemplar, NotReady-before-mutation confirmed.
- `CkAudio/Public/CkAudio/AudioTrack/CkAudioTrack_Processor.h:158` —
  `FProcessor_AudioTrack_SpatialUpdate`, `FTag_Transform_Updated`-gated, confirmed.
- `CkSpatialQuery/CLAUDE.md:47` — `ProbeName.MatchesAny(Filter)` direction confirmed; ADR-6's
  filter-direction note matches the recorded incident.
- Engine (fork, 5.7.4): `VoiceModule.h:70/81/91/98` factories + `DoesPlatformSupportVoiceCapture`
  confirmed; `Voice.Build.cs:12-21` `VOICE_MODULE_WITH_CAPTURE=0` on Server targets + vendored
  libOpus dep confirmed.
- `CkEcs/Public/CkEcs/Processor/CkProcessor_NetModePolicy.{h,cpp}` — `AuthorityOnly` now gates on
  `Get_IsEntityNetMode_Host` at the framework level (:25-31), so `FProcessor_VoiceChat_Route`
  declared `AuthorityOnly` is host-safe as designed; the historical admits-clients gotcha is
  fixed and needs no body-gate.
- `rg` sweep of `Source/` — no `voice`/`voip` identifier collisions (module name clean);
  `FCk_HandleNetSerializer` present in `CkEcs/Handle/CkHandle.h/.cpp` (handles-in-RPC-signatures
  claim holds).

### Design / architecture observations

- **The §A HybridRadio "leak" question resolves to no-leak.** For a radio channel, membership IS
  the entitlement — every member is meant to hear regardless of range; the probe exists only to
  cull *Positional3D bandwidth*. The one-wire-copy client-side 3D/2D decision needs the speaker's
  transform, which arrives via ordinary actor replication; if the speaker's pawn is
  relevancy-culled the client simply renders the 2D radio branch, which is the correct degradation.
  No server-side knowledge is leaked that membership didn't already grant.
- **Amplitude fairness (§A bullet 3): accept self-report for v1, with two cheap server-side
  hardenings.** A modified client reporting 255 is indistinguishable from a legitimately loud
  talker, and it occupies exactly one of N slots per listener — bounded damage. Add: (a) clamp
  the reported value into a per-talker envelope with decay (a constant-255 reporter decays toward
  a served-average rather than pinning the max), and (b) break top-N saturation ties by
  least-recently-served round-robin so N maxed reporters cannot permanently evict honest talkers.
  No decode required, a few lines in the routing processor. Document as a known v1 limit in the
  module `Claude.md`.
- **The three-feature cut is right** (open Q5 ruled above). Talker = replicated identity + frame
  flow; Channel = policy + membership; Listener = local ears. Each has a distinct lifetime and a
  distinct replication story, which is exactly when the house splits features.
- **The threading model's load-bearing detail is correctly identified:** persistent per-talker
  decoders (Opus PLC is stateful) pooled and never shared, SPSC rings, no-alloc/no-lock after
  warm-up. The drain-on-tick capture path adding ≤1 frame of latency against a 20 ms cadence is
  sound arithmetic.
- **Listen-server host handling (inject-to-inbox, never self-forward, loopback only via CVar)
  directly pins the host-echo/deaf-host failure class** every reference shipped broken. Good —
  keep the P3 host-asymmetry net tests as the gate of record for it.
- **Risk sizing (§E) is right.** Jitter at Medium is sufficient *because* ADR-3 makes the buffer
  policy unit-testable — the references re-tuned post-launch precisely because their buffers were
  opaque engine internals. Encode cost at Low with the benchmark already in the P1 gate needs no
  change; do not start P2 before the P1 benchmark is recorded (the plan already sequences this).
- **P0 spike is the right and only pre-P0-worthy verification** (§C bullet 1) — every other
  [INFERRED] has a phase gate that catches it at the cheapest point (with suggestion 5's P2
  packaged smoke closing the one gap I saw).
- Incidental repo-doc drift discovered while verifying (not this campaign's problem):
  `CkParticles` exists in `Source/` but has no row in `Source/CLAUDE.md`'s tier table.

### Sign-off conditions (only if "CHANGES REQUESTED")

N/A — green-lit. Standing condition restated: N1's spec amendment (drop-until-resolvable +
counter + net-test case) lands before the P3 gate.

---

### Reviewer

- **Name:** Claude (Fable 5) — CTO design review, fresh session (did not author the spec)
- **Date:** 2026-08-02
