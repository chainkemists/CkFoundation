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

## Exit criteria

- [ ] All 8 work items landed, each with its tests.
- [ ] `--build --test --test-pattern VoiceChat --discover-fresh` on BusterBlock: green, EXIT 0,
      0 `Angelscript: Error`, count **≥ 30** (the entry baseline; new specs raise it).
- [ ] RenderTarget **22/22** delta-zero on the SAME binary, no rebuild between runs.
- [ ] Freshness chain recorded (sources → DLL → run mtimes monotonic) — no stale-green.
- [ ] F5 resolved; F4 explicitly restated as a P5 obligation.
- [ ] N4 dep budget re-checked: every Build.cs entry has a recorded earning event.
- [ ] `[EDITOR-VERIFY]` steps written with exact clicks for the attenuation/effect/HybridRadio
      auditions (agents cannot launch PIE — root non-negotiable #7).
- [ ] Comment audit run over the P4 diff (no new campaign breadcrumbs — F4's lesson).
- [ ] Fresh top-tier audit appended to this doc before P5 opens.
