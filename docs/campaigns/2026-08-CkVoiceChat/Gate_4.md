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

## Gate-4 top-tier audit — 2026-08-04

Fresh-context adversarial audit; every claim below is grounded in a file:line, a log line, or an
mtime I read myself. Nothing was taken from the executor's narrative on trust.

### Verdict: **GO WITH CONDITIONS** for opening P5

The P4 machinery is real and house-conformant, the run evidence is authentic (summary blocks,
name-level diffs, binary mtimes, and the no-rebuild claim all check out), and the three new net
specs are load-bearing (differential controls, RPC-boundary counters, a correctly polled transient
window). The conditions are one small code defect on the gate's own headline path and one
proof-gap-plus-record-overstatement against non-negotiable #3 — both mechanical, both fixable as
the first P4-followup/P5 commits. Nothing requires re-opening the design.

### Findings (ranked)

**F1 — MEDIUM (code defect, the gate's headline path): a channel still loading its audio assets
can be latched as the playback config and is never re-applied when the load completes.**
`TryGet_ChannelByIdx` does not gate on `FTag_VoiceChannel_PendingAssetLoad`
(`CkVoiceChannel_Utils.cpp:337-361`), and the client control-plane apply publishes the idx as soon
as the channel *composes* — its NotReady check is `TryGet_VoiceChannel` by name only
(`CkVoiceChannel_Utils.cpp:376-380`), not Setup completion. So on a receiving client the drain can
select a channel whose `RootedBatch` is still in flight; `Apply_SynthChannelConfig` then reads
null `Get_ResolvedAttenuation`/`Get_ResolvedSourceEffectChain` and the synth latches a config with
the authored assets missing. The only re-apply triggers are a config-channel CHANGE
(`CkVoiceTalker_Processor.cpp:697-702`), synth creation (:837), and a hybrid flip (:942) — load
completion re-applies nothing, so the wrong render persists indefinitely. Real-world trigger: a
listener joins while a talker is already streaming (channel composes, bundles arrive within the
async-load window — a common scenario, not a corner). The authority side is protected
(`FProcessor_VoiceChannel_HandleRequests` excludes `FTag_VoiceChannel_NeedsSetup`,
`CkVoiceChannel_Processor.h:79`, so joins wait for resolution); clients are not. This is the gate's
own "Attenuation asset null at playback" observation row, whose prescribed response is "fix at the
resolution seam". **Required action (Condition 1):** in the selection block
(`CkVoiceTalker_Processor.cpp:659-671`), skip a `DeliveringChannel` still tagged
`FTag_VoiceChannel_PendingAssetLoad` (leaving `_PlaybackConfigChannel` unset so the next drain
retries), or re-apply when the tag clears. One-guard fix; the HybridRenderMode/PlaybackConfig seams
can pin it.

**F2 — MEDIUM (proof gap + record overstatement, non-negotiable #3): the failed-resolve branch of
the item-1 validation boundary is machine-tested by NOTHING, and the record claims otherwise.**
PROGRESS (2026-08-04 evening) records: "the machine half pins the no-asset and failed-resolve
branches." I grepped both repos (`Set_Attenuation|Set_SourceEffectChain` over CkTests `Source/` +
`Script/` with `--no-ignore`, and the CkVoiceChat gym scripts): **zero tests author either soft
ref** — no positive resolution, no failed resolution, ever exercised by a machine test. The
no-asset branch is exercised implicitly (every net spec composes bare channels), but the
failed-resolve branch — the actual new validation boundary, `EveryAuthoredAssetResolved` at
`CkVoiceChannel_Processor.cpp:146-167` — is pinned by nothing. Non-negotiable #3 requires a
focused invalid-input test at every new validation boundary. The branch *code* is correct-by-read
(hoisted side-effect-safe local, empty-body ensure, separate always-on reset branch that clears
the batch + resolved ptrs, removes the pending tag, and completes setup — no partial state).
**Required action (Condition 2):** author a focused spec with a bogus `FSoftObjectPath` asserting
the fallback state (resolved getters null, setup completed, no crash, expected-ensure handled), or
record the coverage gap as a numbered deviation with maintainer sign-off and correct the PROGRESS
claim. The first option is the fuller one.

**F3 — LOW (doctrine, contradicts a ticked exit item): the P4 diff added two new phase-labeled
comments.** The exit criterion "no new campaign breadcrumbs" is ticked, but `abee82dc1` added
"P4 playback-config seams" and "P4 prune seam" (`CkVoiceTalker_Utils.h:271,282`), and both
survived the F4 strip (`50bfa61fa`, which came later the same day). Also pre-existing and now
factually stale: "(P3)" at `CkVoiceTalker_Fragment_Data.h:99` ("routing consumes this once
transport exists (P3)" — transport has existed since P3 closed). Strip the labels at the next
touch; the technical content stays.

**F4 — LOW (overstated equivalence claim): `faf5b96f0`'s "No behavior change where an audio
device exists" has two corners.** (a) The render attaches at `PlaybackAttachSocketName`
(`CkVoiceTalker_Processor.cpp:829-830`) but the near/far state now measures from the actor origin
(:927) — divergence up to the socket offset, negligible against range+margin but not zero.
(b) A near-state with an UNATTACHED synth renders flat WITHOUT the radio chain: `Spatialize`
fails on the attach guard while `ApplyEffectChain = NOT HybridNear` stays false (:868-877) —
"all fallbacks collapse to radio" breaks in exactly that corner (previously the early-out kept
the state unset → radio). Narrow (needs an owning actor with no attachable root); worth a
one-line guard (`HybridNear` should also require the attach parent) whenever the file is next
touched, not a re-gate.

**F5 — LOW (edge, deviation-adjacent): per-drain reselection can flap the config across drains.**
The "ties keep earliest" rule stabilizes selection *within* one drain
(`CkVoiceTalker_Processor.cpp:645-671`, strict `>`), but selection recomputes per drain from that
drain's bundles only: a talker delivering on two channels whose copies split across drains (loss,
reorder, tick phase) flips the config down and back, each flip costing a synth Stop→Start. In
practice both copies ride the same server flush, so this is bounded — but the deviation's
"no config flapping between equal-priority channels" claim only holds per-drain. Note for the P5
soak; no action now.

**F6 — INFO: first-drain double Stop→Start.** On the first drain, `TryCreate_PlaybackSynth`
applies the config (:837), then the first `Evaluate_HybridRenderMode` sets `_HybridRenderNear`
from unset and re-applies (:938-942) even when the resulting render is identical. Harmless — the
PCM queue survives by design — just redundant.

### Claims verified (exact evidence per exit criterion)

- **VoiceChat 33/33, EXIT 0, 0 AS errors** — `Test-VoiceChatP4-exitsweep2.log:4749-4753`
  (`Total: 33 / Passed: 33 / Failed: 0 / Skipped: 0 / Contaminated: 0`), five `EXIT CODE: 0`
  lines (3 lanes + net; :1792,2678,3675,4747), `grep -c 'Angelscript: Error'` = 0,
  `grep -c 'Result={Fail'` = 0.
- **RenderTarget 22/22 on the same binary, no rebuild** — `Test-P4-RenderTarget-exitsweep.log:
  7584-7588`, EXIT 0 ×5, 0 AS errors; every "Compil-" hit in BOTH logs is
  `LogShaderCompilers ... AutogenShaderHeaders.ush` — zero C++ compile/link actions. Name-level
  diff of the 22 Success paths vs `Test-P3-RenderTarget-postrebase2.log` → **identical sets**.
- **+3 tests exactly, zero regressions** — name diff of the 30 baseline Success paths
  (`Test-VoiceChatP3-postrebase2.log`) against the 33: all 30 present; new =
  {`Net.HybridRenderMode`, `Net.ModerationMatrix`, `Net.PlaybackConfig`} — nothing else.
- **Freshness chain monotonic** — sources (`CkVoiceTalker_Utils.h/.cpp`, the `9c6cab9e4` fix)
  16:35:15 → `BusterBlockEditor-CkVoiceChat.dll` 16:36:01 → `BusterBlockEditor-CkTests.dll`
  16:40:45 (the checklist omits this one; it is the `157f491b` spec-fix rebuild and still
  predates the sweep) → VoiceChat sweep 16:45 → RenderTarget 16:49. No DLL under
  `BusterBlock/Binaries/Win64` newer than 16:40:45. BB trees at the audited tips: CkFoundation
  clean @ `9c6cab9e4`, CkTests @ `157f491` with only `Script/Generated/CkTestsAssets.as` dirty —
  editor-boot regeneration adding OTHER plugins' autotest entries (Aggro/Crowd/AudioTrack), benign
  run residue, not tampering. The two intermediate logs (`-exitsweep-fix.log` 16:37,
  `-exitsweep-fix2.log` 16:41) match the two recorded fix cycles — honest history.
- **Stop→set→Start engine claim** — `USynthComponent::Start()` copies `AttenuationSettings` /
  `bAllowSpatialization` to the AudioComponent and `SourceEffectChain` to the Synth sound
  (`Runtime/AudioMixer/Private/Components/SynthComponent.cpp` ≈443-490, copy block ≈464-479 —
  the commit's ":465-479" cite is accurate to within drift); `Stop()` never touches queued data,
  and the PCM queue survives because the generator holds it by shared ref
  (`CkVoiceChatSynth_Component.h:16-20,33`).
- **RootedBatch GC-root claim** — `FCk_ResourceLoader_RootedAssetBatch` holds
  `TSharedPtr<FStreamableHandle>`; the type's own contract states the handle IS the GC root and
  reset/destruction releases (`CkResourceLoader_Fragment_Data.h:157-182`). The resolved objects
  are held as `TWeakObjectPtr` behind that root — correct non-owning observation per doctrine.
- **No-EndPlay-reset rationale** — `FProcessor_VoiceChannel_EndPlay` is AuthorityOnly
  (`CkVoiceChannel_Processor.h:144`); the batch releases via fragment destruction at entity
  teardown on every machine. Rationale holds.
- **Receive-drain ordering** — selection precedes the config-change apply, which precedes
  `TryCreate_PlaybackSynth` (`CkVoiceTalker_Processor.cpp:645-724`): on the first drain the
  latch is set before creation and `TryCreate` applies post-attach (:837); on later drains the
  change-apply hits the live synth. No misorder found.
- **One wire copy (the STOP condition)** — item 4 (`da8ae684f`) and its follow-up (`faf5b96f0`)
  touch ONLY VoiceTalker playback files; the Route processor's HybridRadio row is unchanged in
  the whole P4 range. The no-routing-fork property holds structurally.
- **Hysteresis mirror** — become-near at range, stay-near to range+margin
  (`CkVoiceTalker_Processor.cpp:929-934`), pinned by the four-beat walk (300→near, 550→holds,
  700→far, 550→stays far; `Net/CkVoiceChat_HybridRender.spec.cpp:360-370`) — the asymmetry is
  proven by the SAME 550 placement asserting opposite states on approach direction.
- **ModerationMatrix is load-bearing** — one identical inject seam across six acts
  (`Net/CkVoiceChat_ModerationMatrix.spec.cpp:105-120`), arrival-counter asserts at the RPC
  boundary (`Debug_Get_ReceiveArrivedBundles`), the act-3 differential (B frozen WHILE C grows,
  :361-363) separates moderation from a dead pipe, growth positive-controls bracket every frozen
  assert, and the composition is proven both directions (:460-461, :500).
- **PlaybackConfig spec** — selection asserted by wire idx through the seam (:245-246); the
  transient amplitude window POLLED via `FCk_Latent_WaitUntil` (:214-224, the recorded fix-cycle-2
  lesson) rather than settled; sticky selection through silence (:279-281); prune proven with a
  populated-first precondition (>0 at :319-320, ==0 at :343-344) against the live-anchor
  total-count seam (`9c6cab9e4` — the tombstone lesson is real: a destroyed talker's handle
  cannot anchor `Has<>` nor safely key matches).
- **F5 (Gate-3 carry) resolved** — EndPlay sweeps both world maps, dropping stale player keys en
  route (`CkVoiceTalker_Processor.cpp:939-977` region); the stated residual (a departed player's
  own outer key falls to the next sweep) is honest and bounded by player count.
- **Item 7 release** — arrival-clock guard keeps the release off loopback talkers
  (`CkVoiceTalker_Processor.cpp:710-718`); release threshold = the sender's own stale-drop age.
- **N4 dep budget** — Build.cs Ck deps are EXACTLY
  ActorRelay/Core/Ecs/EcsExt/Label/Log/Record/ResourceLoader/Settings/Shapes/SpatialQuery +
  engine AudioMixer/Core/CoreUObject/DeveloperSettings/Engine/GameplayTags/NetCore/Voice;
  `CkRelationship` absent; ResourceLoader earned at item 1 with the earning event in the
  Build.cs why-comment.
- **F4 strip (Gate-3 carry)** — all five flagged sites re-swept clean (amendment/campaign/review
  labels gone, HoL-blocking and measurement why-content retained). Residue = this audit's F3.
- **Gym + docs** — "Voice Chat" registered (`CkTests_GymRegistry.as:77`); 0 AS errors in the
  sweep proves it compiles; module Claude.md carries the P4 playback-config section + roger-beep
  recipe; `Source/CLAUDE.md` tier row updated (the P4 range's only out-of-module source touch —
  recorded, `f94dfaa38`).

### Deviations ruling

1. **Priority ties keep EARLIEST (vs the gate doc's "most recent") — ACCEPTED.** Recorded in the
   commit, PROGRESS, and here; strict `>` is the simpler implementation and strictly better
   within a drain. The stability rationale is bounded per-drain (finding F5) — acceptable for v1,
   re-examine only if the P5 soak shows audible config churn.
2. **Item-1 positive path scoped to the audition — SPLIT RULING.** The positive half is ACCEPTED:
   no engine-shipped `USoundAttenuation` exists, authoring a content asset in CkTests was a real
   alternative but the `[EDITOR-VERIFY]` step B covers the same claim with more fidelity (audible
   application, not just pointer equality). The failed-resolve half is REJECTED AS RECORDED: the
   record claims machine coverage that does not exist (finding F2 / Condition 2).
3. **F4 stripped early instead of deferred to P5 — ACCEPTED.** Verified on all five flagged
   sites; doing it early was the better call. The two new P4-labeled seam comments (F3) are the
   only residue.
4. **(Unrecorded but honest) specs batched to the BB gate rather than landing strictly WITH their
   work items — ACCEPTED as circumstance.** BusterBlock is the only net-capable host and was
   blocked mid-campaign (sibling session, stale generated scripts); each item still carried a
   local delta-zero gate and the deferred-verification ledger named what remained. The evidence
   chain never broke.

### Scope check

`git diff a16c55389..7978e9771 --name-only` touches only `Source/CkVoiceChat/`,
`docs/campaigns/2026-08-CkVoiceChat/`, and `Source/CLAUDE.md` (the recorded tier-row update).
CkTests commits (`f67bc2b7`, `5205d36b`, `e49fe505`, `157f491b`) touch only VoiceChat specs, the
net-subject helper, and the gym scripts/registry. No unrecorded scope creep. Dep budget honored
(see Claims). The `[EDITOR-VERIFY]` block is genuinely followable (exact stations, keys, console
fallbacks, expected observations per step, including the flip-must-be-clean check with a
prewritten response).

## Audit-condition resolution — 2026-08-04 (same session)

**All four actionable findings resolved and re-gated green. Gate 4 machine portion CLOSED.**

| Finding | Resolution |
|---|---|
| C1/F1 (mid-load latch, never re-applied) | The receive drain skips a delivering channel still carrying `FTag_VoiceChannel_PendingAssetLoad` — the next drain after the load completes selects and applies (CkF fix commit). Late-join now gets authored config as soon as it resolves. |
| C2/F2 (failed-resolve branch untested; record overstated) | `Ck.VoiceChat.Channel.AudioAssetResolveFails` — bogus authored soft ref ensures once, setup completes on defaults, resolved getters null, membership still works (zero partial state). Green FIRST run. The earlier PROGRESS claim that this branch was already pinned is hereby corrected: it was not, until this spec. |
| F3 (new P4 breadcrumbs) | Both seam labels + the stale "(P3)" stripped; technical content retained. |
| F4b (unattached-near loses the radio chain) | Render-near now requires attachment: an unattached synth with a computed-near state renders RADIO (flat + chain), never bare flat. The state stays attachment-independent (F4a's socket-offset divergence accepted as negligible vs range+margin, per the audit's own read). |

**Re-gate (BusterBlock): VoiceChat 34/34 — 0 failed, 0 contaminated, 0 `Angelscript: Error`,
3m18s (Test-VoiceChatP4-auditres.log) + RenderTarget 22/22 on the SAME binary, no rebuild
(Test-P4-RenderTarget-auditres.log).** The suite baseline is now **34**.
