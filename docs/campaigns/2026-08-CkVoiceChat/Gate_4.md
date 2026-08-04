# Gate 4 — HybridRadio, per-channel audio config, moderation matrix (P4)

> **Status:** 🟡 In progress (opened 2026-08-04)
> **Depends on:** Gate 3 machine portion ✅ CLOSED — audit **GO WITH CONDITIONS**, all three
> conditions implemented AND re-gated green 2026-08-04 (resolution block in [Gate_3.md](Gate_3.md)
> § "Audit-condition re-gate").
> The two Gate-2 HUMAN items (mic `[EDITOR-VERIFY]`, N5 packaged smoke) remain OPEN as
> **P2-verification obligations gating P5 ship**, not this gate. Same reasoning as at Gate-3 open:
> P4 changes *which* listeners hear a stream and *how it is filtered on playback*, not whether the
> capture→encode path works. Revert hook: say "wait for the human items" and P4 pauses at the next
> commit.
> **Estimate:** 2–3 sessions. Re-date at each session entry (per `ck-methodology` § "Budget honestly").
> **Death condition:** superseded by the P5 gate doc, or tombstoned if P4 scope is cut.

## Goal

After this gate: a channel's **audio configuration is honored end-to-end**. `HybridRadio` earns its
name — one wire copy, played spatialized when the speaker is within the channel's range and as
filtered 2D radio otherwise, decided per recipient. Per-channel `USoundAttenuation` and
`USoundEffectSourcePresetChain` are resolved and applied at playback instead of sitting unread in
the params struct. The moderation surface (`CanTalk` / `CanListen` / server-mute, and their
interaction with listener mute) is proven by a test matrix rather than by the single-axis specs P3
left behind.

## Binding constraints carried into this gate

- **ADR-5 (per-channel audio config):** spatialization policy, attenuation, source-effect chain,
  audible range, and priority live on the channel entity. Soft refs are resolved through
  **CkResourceLoader** — the spec names this as the fix for cook-breaking string paths. A raw
  `LoadSynchronous` on the audio path is a review rejection.
- **ADR-5 `HybridRadio` definition (verbatim intent):** "one wire copy, played positionally when
  the speaker is within proximity range, as filtered 2D radio otherwise." **One wire copy** is
  binding — the near/far decision is a *playback* decision on the receiving client, NOT a routing
  fork that sends twice.
- **N4 (dep budget):** every new dependency must be earned by code that consumes it in this phase.
  `CkResourceLoader` is expected to be earned here (ADR-5 consequence); anything else needs a
  recorded earning event. `CkRelationship` remains banned.
- **Roger-beep (Ruling Q4):** **NO `CkCue` dependency in v1.** P4 ships a *documented consumer
  recipe* (bind `OnTransmitStarted`/`OnTransmitStopped` → cue) plus a gym audition — not a
  first-party beep implementation.
- **Non-negotiable #3** still governs every new validation boundary (hoisted validity local,
  ensure with empty body, separate always-on failure branch, focused invalid-input test).
- **N2 attribution** now exists (Gate-3 C3); any NEW drop/reject site added in P4 must join the
  per-drain tally rather than inventing a second logging scheme.

## Carried items from the Gate-3 audit

- **F5 (LOW, slow leak) — P4 hygiene, in scope here.** `FFragment_VoiceChat_ServeHistory::_LastServedFrame`
  (outer key weak `APlayerState`, inner key talker handle) and
  `FFragment_VoiceChat_ListenerMuteMatrix::_MutedByPlayer` never prune entries for departed players
  or destroyed talkers. Harmless at session scale; a long-lived server with churn accumulates stale
  weak keys and dead handles. Work item 6.
- **F4 (LOW, doctrine) — DEFERRED TO P5, recorded so it is not lost.** Process-breadcrumb comments
  naming a Gate/Phase/PROMPT/campaign/review remain in shipped code (`CkVoiceChatControlRelay_Actor.h:16`,
  `CkVoiceChatRelay_Actor.h:19`, `CkVoiceTalker_Processor.cpp:55,59`,
  `CkVoiceChat_Route_Processor.h:50,83-84`, `CkVoiceChatSynth_Component.h:41,14`). The technical
  *why* content stays; the campaign labels go. Strip during the P5 module-doc pass.

## Entry criteria (run 2026-08-04 at open)

- [x] Gate-3 exit re-verified on current HEAD, post-rebase: CkFoundation `feature/voice-chat`
      @ `cc95c7760` (docs commit `287cd0ba0` on top), CkTests `feature/voice-chat-wip` @ `52a4d9f`.
      CkFoundation tree clean.
- [x] **Baseline captured (this is what every later "no regressions" diffs against):**
      VoiceChat **30/30** (`Test-VoiceChatP3-postrebase2.log`) + RenderTarget **22/22**
      (`Test-P3-RenderTarget-postrebase2.log`), both EXIT 0, 0 `Angelscript: Error`, same binary,
      no rebuild between runs. Zero failing tests at entry — any red at exit is ours.
- [x] Host confirmed: **BusterBlock is the only net-capable host.** The CkPlugins host cannot run
      net specs (`DisableEnginePluginsByDefault: true` + slimmed closure excludes
      OnlineSubsystem/OnlineSubsystemUtils, no `NetDriverDefinitions` → `NetDriverCreateFailure ...
      Driver = NONE`, 11/11 net specs fail on environment). Do not re-litigate; see PROGRESS.md
      2026-08-04.
- [x] Current-state survey done at open (read, not remembered):
      - `ECk_VoiceChat_SpatializationPolicy` already declares all three policies
        (`CkVoiceChannel_Fragment_Data.h:36-48`).
      - `_Attenuation` / `_SourceEffectChain` params already exist
        (`CkVoiceChannel_Fragment_Data.h:84-90`) but **`Get_Attenuation` / `Get_SourceEffectChain`
        have ZERO consumers** — the config is declared and unread.
      - **`HybridRadio` currently routes exactly like `Global2D`**: only `Positional3D` takes the
        proximity-gated branch (`CkVoiceChat_Route_Processor.cpp:461`; same single-policy check at
        `:161` and `CkVoiceChannel_Processor.cpp:72`). No near/far behavior exists yet.
      - Moderation primitives exist (`_ServerMuted` set + RepData mirror, `CanTalk`/`CanListen`
        member flags, listener mute matrix) — P4 adds the *matrix coverage*, not the mechanism.
      - **CORRECTION (deeper survey, same day): playback today is entirely NON-spatialized.**
        `TryCreate_PlaybackSynth` (`CkVoiceTalker_Processor.cpp:763`) creates a bare synth — no
        attach to the talker's transform, no attenuation, no spatialization flags — and
        `Get_DefaultAttenuation` has ZERO consumers. Positional3D gates *routing*, but the received
        audio renders flat 2D. Item 2 therefore lands spatialized playback itself, not merely a
        per-channel attenuation swap. (Gate-3's "synth attached at the talker's location" goal line
        was aspirational; the code never did it.)

## Decision recorded at open — which channel's audio config a synth uses

A talker has ONE synth per listening machine but may deliver on MULTIPLE channels; audio config is
per-channel. The wire header carries `ChannelIdx` per bundle, so the receive path knows each
bundle's channel. **Decision:** the synth adopts the config of the **highest-`_Priority` channel**
among those currently delivering that talker's stream to this listener — the field exists for
exactly this (ADR-5), and the Onset reference's "single-send + single-playback dedupe across shared
channels" is the precedent. Ties break toward the most recently delivering channel. Low-blast,
reversible — say the word if you want per-channel synths instead (rejected here: N synths per
talker multiplies audio components and reintroduces the double-playback the reference dedupes).
- [x] **Carried blocker CLEARED 2026-08-04:** CkTests `35fcde38` commits the one-line fix
      (mirrors `e5bb948b`) and the follow-up merge of `e5bb948b` itself brings the branch to
      **0 behind / 28 ahead** of `origin/dev`. `feature/voice-chat-wip` AS-compiles standalone.

## Work items (sequenced; each names its exemplar or is flagged NEW)

1. **Soft-ref resolution for channel audio assets (ADR-5 consequence).** Resolve `_Attenuation` and
   `_SourceEffectChain` through `CkResourceLoader` at channel composition, holding the resolved
   objects on a channel fragment. Earns the `CkResourceLoader` dep (record the earning event in
   Build.cs review per N4). Exemplar: an existing CkResourceLoader adopter — name it in the commit,
   do not invent a loading shape.
2. **Spatialized playback + per-channel attenuation (bigger than first scoped — see the survey
   correction).** Attach the synth at the talker's transform (the params' playback attach socket
   has waited for this since P0), enable spatialization, and apply the channel-resolved attenuation
   per the priority decision above. Falls back to
   `UCk_Utils_VoiceChat_Settings_UE::Get_DefaultAttenuation()` when the channel specifies none
   (getter exists, `CkVoiceChat_Settings.h:191` — currently zero consumers). Global2D channels keep
   the non-spatialized render. Verify `USynthComponent`'s attenuation/spatialization surface
   against 5.7 source at implementation, not from memory.
3. **Per-channel source-effect chain at playback.** Apply the resolved
   `USoundEffectSourcePresetChain` to the same component. This is the mechanism `HybridRadio`'s
   "filtered 2D radio" leans on — land it before item 4.
4. **HybridRadio near/far playback (NEW — the gate's headline).** Route HybridRadio to the FULL
   member set (one wire copy, as today), and decide *per recipient at playback* whether the stream
   plays spatialized (speaker within the channel's `AudibleRange`) or as non-spatialized filtered
   radio. Reuse the P3 hysteresis shape so a speaker hovering at the range boundary does not
   flip modes every frame — `CkVoiceChat_Route_Processor.cpp` already implements per-(recipient,
   channel-idx) hysteresis for the Positional3D gate; mirror it, do not re-derive it.
   **Explicitly NOT a routing fork** — assert one send per recipient per bundle.
5. **Moderation matrix tests.** A test matrix over `CanTalk` × `CanListen` × server-mute ×
   listener-mute, including: server-mute survives leave/rejoin (already pinned singly at P3 —
   fold into the matrix), a `CanListen=false` member never receives, a `CanTalk=false` member's
   bundles are dropped and counted, and listener-mute composed with server-mute (both directions,
   no interaction bug). Net where the axis is net-observable; unit where it is policy.
6. **F5 hygiene: prune the unbounded maps.** `_LastServedFrame` and `_MutedByPlayer` drop entries
   for departed players and destroyed talkers. Prefer pruning at the existing membership/EndPlay
   seams over a periodic sweep — the leave and destroy paths already run.
7. **Amplitude / speaking polish.** Remote `Get_CurrentAmplitude` parity and the speaking-state
   signal edge cases (P3 mirrored header amplitude; verify hold/decay behavior on the remote side
   matches the local side F6 fix).
8. **Roger-beep consumer recipe (docs + gym only).** Documented `OnTransmitStarted`/`OnTransmitStopped`
   → cue recipe in `Source/CkVoiceChat/Claude.md` plus a gym audition station. **No CkCue dep.**

Tests land WITH their work item, not batched: attenuation/effect application is `[EDITOR-VERIFY]`
(audition) plus a unit assert that the resolved asset reached the component; HybridRadio gets a net
spec proving near→spatialized / far→2D for the SAME single wire copy; the moderation matrix lands
with item 5.

## Expected observations at the gate — and what to do on each branch

| Observation | Reading | Action |
|---|---|---|
| HybridRadio near/far flips cleanly, one send per recipient | Intended | Proceed |
| HybridRadio sends twice (once per mode) | ADR-5 "one wire copy" violated | STOP — this is a routing fork; move the decision to playback |
| Mode flickers at the range boundary | Hysteresis not applied | Reuse the P3 hysteresis, do not add a timer |
| Attenuation asset null at playback | Soft-ref resolution raced composition | Fix at the resolution seam (item 1), never `LoadSynchronous` on the audio path |
| Moderation matrix red on a combination P3 "already covered" | P3's single-axis specs hid an interaction | Real finding — fix and record; do not weaken the matrix |
| Any net spec red on BusterBlock | Could be ours or upstream drift | A/B against the entry baseline (30/30) before blaming own code |

## `[EDITOR-VERIFY]` — the audition block (human, exact steps)

Prerequisite for ALL steps: `[Voice]` + `bEnabled=true` in the host project's
`Config/DefaultEngine.ini` (the standing P2 human decision), and a working microphone.

**A. Mic loopback + transmit edges (single player — the P2 obligation, folded in):**
1. Open the editor on a host with CkTests → PIE on `TestGyms_CkTests_Level`.
2. Tab → cycle to **"Voice Chat"** → the MIC LOOPBACK station.
3. HOLD **V** and speak. Expect: your own voice back (full capture→encode→decode→synth path)
   and a green **TX START** flash; release → yellow **TX END** flash. Console fallbacks:
   `Ck_GymVoiceChat_TalkStart` / `Ck_GymVoiceChat_TalkStop`.
4. Silence WITH the flashes working = capture never opened → re-check the `[Voice]` line.

**B. Per-channel attenuation + spatialization (2 PIE clients):**
1. Author a `USoundAttenuation` asset (any obvious falloff, e.g. 500→2000cm) and set it as
   `_Attenuation` on a Positional3D test channel both clients join (or set it as the module
   default via Project Settings → Ck VoiceChat → DefaultAttenuation).
2. Play In Editor with **Number of Players = 2**, net mode listen server.
3. Talk on client A while moving A's pawn toward/away from B's. Expect at B: A's voice
   attenuates with distance and pans with direction (spatialized at A's pawn).
4. This is ALSO item 1's positive-path proof: a resolved-through-CkResourceLoader asset
   audibly applied (the machine half only pins the negative/no-asset branches).

**C. Effect chain + HybridRadio near/far (2 PIE clients):**
1. Author a `USoundEffectSourcePresetChain` with an unmistakable filter (e.g. aggressive
   low-pass "radio" EQ); set it on a HybridRadio channel with a small AudibleRange (~500cm).
2. Talk on A. At B, walk B's pawn: inside 500cm expect plain spatialized speech (NO filter);
   beyond ~600cm (range+margin) expect flat filtered "radio". Walk back into the 500-600 band:
   must STAY radio until inside 500 (the hysteresis asymmetry — the state machine half is
   already pinned by `Ck.VoiceChat.Net.HybridRenderMode`).
3. The flip itself should be a clean cut, not a stutter — the PCM queue survives the synth's
   Stop→Start by design; a stutter here is a real finding (file it, don't shrug it off).

## Exit criteria — machine portion complete 2026-08-04

- [x] All 8 work items landed with tests (items 2-4's render application half + item 1's
      positive path are the `[EDITOR-VERIFY]` block's steps B/C by recorded scoping decision;
      their machine-assertable halves are spec-pinned: ModerationMatrix, PlaybackConfig,
      HybridRenderMode).
- [x] Full VoiceChat suite on BusterBlock: **33/33, 0 failed, 0 contaminated, EXIT 0, 0
      `Angelscript: Error`, 3m18s** (Test-VoiceChatP4-exitsweep2.log). Two spec fix cycles en
      route, both harness lessons recorded in commit history (tombstone handles cannot anchor
      or key lookups; a deliberately transient state needs FCk_Latent_WaitUntil, not a settle).
- [x] RenderTarget **22/22, 0 failed** on the SAME binary, no rebuild
      (Test-P4-RenderTarget-exitsweep.log).
- [x] Freshness chain monotonic: CkVoiceChat DLL 16:36:01 → VoiceChat sweep 16:45:41 →
      RenderTarget 16:49:55.
- [x] F5 resolved (item 6 + the PlaybackConfig prune act proving it live); F4 RESOLVED EARLY
      (campaign labels stripped, `50bfa61fa`) rather than deferred.
- [x] N4 re-checked: Build.cs = ActorRelay/Core/Ecs/EcsExt/Label/Log/Record/ResourceLoader/
      Settings/Shapes/SpatialQuery, each with a recorded earning event; ResourceLoader earned
      at item 1 (`c4e923a65`); never Relationship.
- [x] `[EDITOR-VERIFY]` block written above (steps A/B/C with exact clicks; gym station
      "Voice Chat" registered and AS-compiles).
- [x] Comment audit over the P4 diff: no new campaign breadcrumbs (module-doc/ADR references
      only where the module already carries them).
- [ ] Fresh top-tier audit appended to this doc before P5 opens.
