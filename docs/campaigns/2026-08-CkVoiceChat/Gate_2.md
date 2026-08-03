# Gate 2 — VoiceTalker capture pipeline + local loopback playback (P2)

> **Status:** 🟡 Machine portion ✅ CLOSED (2026-08-03 — audit GO WITH CONDITIONS, both conditions
> resolved + re-gated same session, resolution table below) — ⏸ awaiting the two HUMAN items
> (mic `[EDITOR-VERIFY]`, N5 packaged smoke). P3 does not start before both.
> **Depends on:** Gate 1 ✅ (audit GO WITH CONDITIONS — all conditions resolved, 2ed23fff0)
> **Estimate:** 1–2 sessions — machine portion: 1 session (2026-08-03)

## Goal

After this gate: a VoiceTalker on a local entity captures microphone (or fake-fixture) PCM,
gates it through VAD, encodes Opus frames, and — with loopback enabled — hears itself through
the spatialized synth component, all with NO networking (transport is P3). The full pipeline
gate→encode→jitter→decode runs headless under AutoTests via the fake capture source; the real
microphone is a human `[EDITOR-VERIFY]` gym step; a packaged Opus-init smoke runs per review N5.

## Binding constraints carried into this gate

- **N5:** packaged Opus-init smoke at THIS gate (capture+codec+playback local), not just P5.
- **N6 synth pins:** initialize the synth at 48 kHz mono; the audio mixer's SRC owns device-rate
  conversion (never resample inside `OnGenerateAudio`); Start/Stop are game-thread-only; the SPSC
  ring must survive Stop→Start without tearing. Verify against 5.7 `SynthComponent.h`.
- **Gate-1 discoveries:** host project (or test boot) needs `[Voice] bEnabled=true` for the
  capture factory; every decode target buffer holds ≥ `MAX_OPUS_FRAMES` (6) frames
  (`VoiceCodecOpus.cpp:20`); jitter buffer's discontinuity contract is in place — the synth's
  receive path just Pushes/Pops.
- **Dep budget (N4):** this gate earns `Voice`, `AudioMixer` (public — `USynthComponent`), and
  `AudioCaptureCore` ONLY if actually consumed; CkVoiceChat_Stats.h gets its first real counters
  (audit Gate-0 finding 5) with the capture/encode processors.
- **Component lifetime:** CkVfx discipline — `TStrongObjectPtr` in Current, Setup creates
  (`bAutoActivate=false`), monitor observes and never destroys, EndPlay destroys + resets.
- **Requests:** full completion contract (trailing delegate, `MakeCompletionGuard`,
  `TExclude<FTag_DestroyEntity_Initiate>`, `FGroup_EndPlay` cancel processor per CkTimer).

## Work items

1. **Capture seam** (`Capture/` subfolder): `ICk_VoiceChat_CaptureSource` (Start/Stop/DrainPcm) +
   `FCk_VoiceChat_CaptureSource_Engine` (wraps `IVoiceCapture`) + `FCk_VoiceChat_CaptureSource_Fake`
   (scripted PCM fixtures: sine, silence, programmable spurts) — the fake is the CI path.
2. **VoiceTalker P2 surface:** requests (`Request_StartTransmit/StopTransmit/SetTransmitMode/
   SetInputGain/SetSelfMute`) with the completion contract + cancel processor; signals
   (`OnTransmitStarted/Stopped`, `OnSpeakingStateChanged`, `OnVoiceFramesCaptured` raw-PCM tap);
   Current grows capture/encoder state (friend-scoped).
3. **Processors:** `Capture` (drain → gain → RMS → VAD → encode → bundle; consumes the transmit
   state) + `HandleRequests` + `CancelPendingRequests` + `EndPlay` (stop capture before releasing
   the encoder — spec §7.8 teardown order).
4. **Playback:** `UCk_VoiceChatSynthComponent : USynthComponent` (48 kHz mono, SPSC ring of
   encoded frames, persistent per-talker decoder, jitter Pop + Opus PLC on Conceal) + talker-side
   Setup/EndPlay component lifetime; loopback CVar `ck.VoiceChat.Loopback` routes the local
   talker's bundles into its own synth.
5. **Tests (CkTests):** AutoTests with the fake source — compose/teardown, transmit start/stop,
   VAD gates frames (silence produces none), loopback pipeline produces decoded PCM; C++ unit
   tests for any new pure pieces. Invalid-input tests for each new validation boundary
   (non-negotiable #3).
6. **N5 packaged smoke:** packaged client boots, Opus factories initialize, loopback produces
   audio — `[EDITOR-VERIFY]`/human-run packaging; exact steps listed at gate close.

## Expected observations at the gate — and what to do on each branch

| I will run | I expect to observe | If instead I see | Prewritten response |
|---|---|---|---|
| AutoTests with fake capture | Full pipeline green headless: frames captured→encoded→looped→decoded PCM non-silent | Red at a stage boundary | The seam isolates the stage; fix that stage — no cross-stage guessing |
| VAD autotest | Silence fixtures produce ZERO encoded frames | Frames leak through | VAD gating bug — fix before playback work continues |
| Synth under `-nullrhi` | Component creates + ring accepts frames; audio render callback may not fire (no audio device) — the DECODE path is exercised via the jitter Pop directly in tests | Crash/ensure on headless create | Gate synth creation on `FApp::CanEverRender()`-style audio availability; decode-path tests stay headless |
| `[EDITOR-VERIFY]` real mic loopback | Human hears themselves with ~100–150 ms latency | Silence/artifacts | Debug via the capture-seam stage taps; jitter overlay numbers |
| N5 packaged smoke | Opus init + loopback in packaged client | Packaged-only init failure | THE community failure class — capture verbatim, fix before P3 |

## Exit criteria — machine portion complete 2026-08-03; human portion listed below

- [x] AutoTests green: **18/18** under pattern `VoiceChat` (14 P1 unit tests + 2 spike canaries +
      `Ck.VoiceChat.Pipeline.FakeCapture_LoopbackDecodes` + `...StartTransmit_DisabledMode_Rejected`),
      EXIT 0, Test-VoiceChatP2-final.log. Pipeline decodes **1.20 s from 1.9 s scripted input**
      (sine + VAD release tail; both silences gated) — the full
      capture→gain→VAD→encode→jitter→decode loop, headless, with the synth in the loop.
- [x] RenderTarget suite **22/22** delta-zero on the same binary (Test-P2-Regression.log).
- [x] AS boot clean (0 `Angelscript: Error`); AS wrappers regenerate for the new request/signal
      surface. Runtime AS exercise deferred to the gym step below.
- [x] Invalid-input test for the gate's new validation boundary (Disabled-mode StartTransmit →
      ensure + zero partial state).
- [x] PROGRESS.md dated entry; audit requested (response appended below when it lands).
- [ ] **HUMAN: `[EDITOR-VERIFY]` mic loopback** — see below.
- [ ] **HUMAN: N5 packaged Opus smoke** — see below.

## Design deviations recorded for the audit

1. **Playback uses `CreateSoundGenerator`/`ISoundGenerator`, not `OnGenerateAudio` on the
   component** — the engine marks the latter soon-to-be-deprecated and the generator path is the
   engine's own no-UObjects-on-render-thread pattern, which is §7.5's discipline anyway.
2. **Decode runs on the GAME thread, not the audio render thread** (spec §7.5 said render
   thread): measured at P1 as 27 µs per 20 ms frame (~1.1% of one core at the full 8-talker
   cap), so the render thread consumes only ready float PCM through a lock-free SPSC queue —
   no decoder state, locks, or UObjects near the render thread at all. Revisit only if a
   profile ever says otherwise (non-negotiable #7 cuts both ways).
3. **Conceal zero-fills instead of Opus PLC** (work item 4 said "Opus PLC on Conceal"):
   `FVoiceDecoderOpus::Decode` early-outs with `OutRawDataSize = 0` on null compressed input
   (engine `VoiceCodecOpus.cpp`), so packet-level Opus PLC is NOT reachable through the ADR-2
   `IVoiceDecoder` factory surface — zero-fill is the only conceal available there. P3's
   listener playback path inherits the same constraint; revisit only if we ever bypass the
   factory for raw libopus access. (Recorded per Gate-2 audit condition 2; the auditor verified
   the engine surface independently.)
4. **Loopback synth is created in HandleRequests (on StartTransmit), not in Setup** ("CkVfx
   discipline: Setup creates"): loopback playback is conditional on a runtime request, so
   creation happens at the request boundary. Every other clause of the component-lifetime
   discipline holds — `TStrongObjectPtr` in Current, `bAutoActivate=false`, no destroy outside
   EndPlay, `Stop` + `DestroyComponent` + `Reset` at teardown. (Recorded per Gate-2 audit
   finding 4.)

## `[EDITOR-VERIFY]` — mic loopback (human)

1. Add `[Voice]` + `bEnabled=true` to `BusterBlock/Config/DefaultEngine.ini` (REQUIRED — the
   engine Voice module is config-gated; without it every capture/codec factory returns null).
   This line is a permanent production prerequisite for voice, not a test hack — landing it is a
   BB-repo decision.
2. Build + open the BB editor with a working microphone. In any level's Level Blueprint (or an
   AS snippet): on BeginPlay — create an entity (or use an existing pawn's), call
   `[Ck][VoiceTalker] Add Feature` with TransmitMode=OpenMic, Loopback=Enable, then
   `[Ck][VoiceTalker] Request Start Transmit`.
3. PIE and speak: you should hear yourself with roughly 100–300 ms delay (loopback rides the
   real jitter policy). `[Ck][VoiceTalker] Get Is Speaking` flips with your voice; silence
   closes it after ~200 ms.
4. While there: the Gate-0 BP checklist items (nodes under `Ck|Utils|VoiceChat|…`, enum
   dropdowns, autocasts, the Project Settings → Voice Chat page defaults).

## N5 packaged Opus smoke (human)

Package a Development client (`runreal buildgraph run ./.runreal/buildgraph/build.xml
-set:ClientConfigurations=Development -Target="Build Clients"` per BB CLAUDE.md), include the
step-2 test level, run the packaged client with the config line from step 1, and verify: no
Opus/Voice init errors in the packaged log, and audible mic loopback. This is the community's
marquee packaged-only failure class — capture any failure verbatim before P3 work continues.

---

## Top-tier audit response (Gate 2, machine portion)

### Verdict

**GO WITH CONDITIONS** — the machine portion is sound and P3 design may begin once the two
conditions below land (both mechanical; neither invalidates any code that shipped) and the two
human items pass. No blocker found in the capture seam, the talker request/signal surface, the
processor pipeline, the synth component, or the tests.

1. **Stats counters (this gate's own contract, "Binding constraints" above):** `CkVoiceChat_Stats.h`
   still declares only `STATGROUP_CkVoiceChat` — zero counters exist anywhere in
   `Source/CkVoiceChat` (rg for `DECLARE_.*STAT|INC_|SET_` matches only the group declaration).
   The gate text promised "its first real counters ... with the capture/encode processors."
   Either land counters (frames captured / encoded / concealed / decoded is enough) or amend this
   gate doc to record the deferral to P3 where ADR-4 clause (f) makes them non-negotiable. Doc or
   ~20-line code change; pick one and record it.
2. **Record the Conceal behavior as deviation 3:** work item 4 promises "Opus PLC on Conceal" but
   the implementation zero-fills (`CkVoiceTalker_Processor.cpp:389-399`). I verified the engine
   surface: `FVoiceDecoderOpus::Decode` early-outs with `OutRawDataSize = 0` on null input
   (engine `VoiceCodecOpus.cpp`, `!InCompressedData` check at the top of Decode) — packet-level
   Opus PLC is **not reachable** through the ADR-2 `IVoiceDecoder` factory surface, so zero-fill
   is the defensible choice, not a shortcut. But it is an undocumented deviation from this gate's
   own work-item text, and P3's listener playback path will inherit the same constraint. Record
   it (doc-only).

### Spot-checks performed

- **A — Capture seam** (`Capture/CkVoiceChat_CaptureSource.{h,cpp}`): interface is the minimal
  Start/Stop/Tick/DrainPcm game-thread seam. The engine wrapper's `[Voice] bEnabled` precondition
  is the exact non-negotiable-#3 shape — hoisted `CaptureCreated` local, `CK_ENSURE_IF_NOT` with
  empty body naming the fix, separate ordinary `if (NOT CaptureCreated) return false`
  (.cpp:29-36). Fake is deterministic: no RNG, continuous `_SampleCounter` phase across segments,
  time-accumulator drain; Stop resets pending time only (.cpp:92-99, 145-189).
- **B — Talker P2 surface**: completion contract matches CkTimer doctrine — trailing
  `AutoCreateRefTerm` delegate with no C++ default on all five requests (Utils.h:76-123);
  `IsBound → Set_CompletionDelegate` on a named request local at every boundary (Utils.cpp:40-45
  et al.; `Set_CompletionDelegate` is const-qualified over the mutable transport,
  `CkRequest_Data.h:95,124`, so the `const auto Request` locals are correct);
  `MakeCompletionGuard` declared AFTER the `Result` local (Processor.cpp:102-103);
  `TExclude<FTag_DestroyEntity_Initiate>` + `CK_IGNORE_PENDING_KILL` on the HandleRequests view
  (Processor.h:47-48); `FGroup_EndPlay` cancel processor calls
  `ck::request::FireCancelledForPending` (Processor.cpp:264-273). Signals defined via
  `CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE` and bound via `CK_SIGNAL_BIND`/`CK_SIGNAL_UNBIND` —
  no CkTimer direct-Bind divergence (Utils.cpp:171-265). Pipeline order confirmed
  drain→gain→RMS→VAD→encode→loopback (Processor.cpp:277-426) with mode/mute/encoder gates before
  encode. EndPlay order: capture Stop first, synth Stop+DestroyComponent+Reset, then
  encoder/decoder release (Processor.cpp:430-456) — spec §7.8 honored. Friend lists on
  Current/Requests name exactly the actual writers (Fragment.h:43-47, 88-90; the Utils friend
  covers the Debug_ test seams). Idempotent no-ops report Succeeded (start-while-transmitting,
  stop-while-stopped) per the result-semantics doctrine.
- **C — Playback** (`Playback/CkVoiceChatSynth_Component.{h,cpp}`): N6 pins honored — `Init`
  writes the settings rate through the `int32& SampleRate` ref + `NumChannels = 1` (.cpp:49-64);
  zero resampling in the generator (pure memcpy/zero-fill, .cpp:13-45); the SPSC queue is held by
  `TSharedPtr` on both component and generator so Stop→Start recreates the generator over the
  same queue untorn (.h:33,68; .cpp:72-77). `OnGenerateAudio` has no locks and no allocations
  beyond the dequeued array's buffer release. Verified against the real 5.7.4 engine header
  (`SynthComponent.h:340` `virtual bool Init(int32& SampleRate)`; :355 carries the engine's own
  "soon to be deprecated, use CreateSoundGenerator" comment; :360 the virtual) — **deviation 1 is
  the engine's own recommendation, not a liberty**. Deviation 2 (game-thread decode) is backed by
  the recorded P1 measurement (26.9 µs / 20 ms frame, Gate_1 benchmark) and buys a decoder-free,
  UObject-free render thread; sound, revisit only on profile per non-negotiable #7. Start/Stop
  both happen only in game-thread processor code.
- **D — Tests** (`CkTests .../Net/CkVoiceChat_PipelineLoopback.spec.cpp`): the loopback test pins
  VAD both ways — lower bound ≥0.5 s decoded proves the spurt passed; upper bound ≤1.5 s from
  1.9 s scripted input fails a never-gating VAD (which would decode ~1.9 s); the loud-sample scan
  proves sine content, not zeros (:139-152). Measured 1.20 s is consistent with 1.0 s sine +
  release tail. The rejection test pins zero partial state (no transmit tag, no speaking tag,
  EMPTY decoded buffer) plus the expected ensure text (:177, :215-220) — and the handler
  validates mode before creating any resource, so the pin is real. Config hygiene: original
  `[Voice] bEnabled` captured (including key-absence) and restored on both terminal paths
  (:43-60); `bSuppressLogErrors/Warnings` is the established house pattern across 17+ sibling
  Net specs.
- **E — Evidence chain**: CkFoundation `origin/dev..HEAD` tip = 08a152b60 (docs-only above
  421d99baa — `git diff 421d99baa..08a152b60 -- Source/` is empty); CkTests tip = 5a0141c. Both
  match PROGRESS. `Test-VoiceChatP2-final.log`: `Passed: 18`, all 3 lanes `EXIT CODE: 0`, the
  `[VoicePipeline] loopback decoded 115200 bytes (1.20 s)` line present, 0 `Angelscript: Error`,
  0 `Result={Fail|Timeout}`. `Test-P2-Regression.log`: 22 `Result={Success}`, `Passed: 22`, all
  lanes EXIT 0. Binary freshness: `BusterBlockEditor-CkVoiceChat.dll` 03:08:37,
  `BusterBlockEditor-CkTests.dll` 03:13:41, run window 03:14–03:16, and **no file under
  `Source/CkVoiceChat` is newer than the DLL** — stale-green ruled out.
- **F — Adversarial**: dep budget clean — `AudioMixer` (USynthComponent) and `Voice`
  (FVoiceModule/IVoiceCapture/IVoice{En,De}coder) both consumed; `AudioCaptureCore` correctly NOT
  added. Headless/dedicated-server safe: synth creation gated on
  `World->GetAudioDevice().IsValid()` (Processor.cpp:171); everything else in the pipeline is
  audio-device-free; a capture-create failure ensures once and completes the request `Failed`
  through the ordinary branch. P3 seam is clean: encoded frames exist with Seq + AmplitudeQ8 at a
  single insertion point (Processor.cpp:345-353) and the P1 Bundle/pacing structs already exist —
  transport is one outbound-fragment push added there, no signature churn.

### Findings

1. **(Condition 1 — gate-contract miss)** Stats counters promised at this gate do not exist; see
   Verdict. Not recorded as a deviation.
2. **(Condition 2 — unrecorded deviation)** Conceal zero-fills instead of Opus PLC; engine
   factory surface cannot express packet-level PLC, so record it and carry the constraint to P3.
3. **(Non-blocking)** StartTransmit creates the encoder and creates+Starts the loopback synth
   BEFORE the capture `Start()` gate (Processor.cpp:143-184): a capture-start failure leaves a
   registered, running (silent) synth until EndPlay. The validation boundary itself is clean
   (mode checked first; the rejection test proves zero state), but moving synth creation after
   `CaptureStarted` removes the residue for free. Suggest folding into P3 work.
4. **(Non-blocking)** Synth creation lives in the HandleRequests processor, not Setup — a
   deviation from the letter of this gate's "CkVfx discipline: Setup creates". Defensible
   (loopback is conditional on a runtime request) and every other clause of the discipline is
   honored (TStrongObjectPtr, bAutoActivate=false, no destroy outside EndPlay,
   Stop+DestroyComponent+Reset at Processor.cpp:445-451). Worth one line in the deviations list
   if anyone re-reads this gate later.
5. **(Non-blocking — P5 perf-pass ledger)** Per-tick/per-frame allocations in the Capture
   processor: the raw-tap copy is built even with zero bound listeners (Processor.cpp:302-305);
   per-frame `FramePcm` + 2048-B `Encoded` allocs (:320-340); 11.5-KB `Decoded` alloc per pop
   (:402-404); float-array + TQueue-node alloc per enqueue (Synth .cpp:89-97); and
   `_PendingPcm.RemoveAt(0, FrameBytes)` is an O(n) front-shift per frame (:322). None matter at
   1 talker; all are on the 8-talker hot path.
6. **(Non-blocking — cosmetic)** `_AmplitudeQ8` is overwritten with `Quantize(0)` on any tick
   that completes no frame (Processor.cpp:315,356), so `Get_CurrentAmplitude` flickers to 0 at
   tick rate mid-speech (960-sample frames vs ~800 samples/tick at 60 fps). Wire use at P3 is
   unaffected (bundles only exist when frames exist); fix is a decay or last-frame-hold.
7. **(Inferred, named honestly)** The exit-criterion phrase "with the synth in the loop" headless
   is inferred, not log-proven: no log line demonstrates `_LoopbackSynth` was created in the PIE
   lanes (creation is silent and the decode assertions read `_LoopbackDecodedPcm`, which works
   either way). The synth's audible path is exactly what the two HUMAN items exist to confirm —
   no action needed beyond running them.

### Auditor

Fresh top-tier session (did not author the work) — date 2026-08-03

---

## Audit conditions & findings — resolution (2026-08-03, same session)

| # | Item | Resolution | Evidence |
|---|---|---|---|
| C1 | Stats counters promised at this gate don't exist | **Landed as code** (not deferred — a doc-only deferral would have been the second consecutive punt of the same item): four frame counters (`Captured/Encoded/Concealed/Decoded`) declared in `CkVoiceTalker_Processor.cpp` against `STATGROUP_CkVoiceChat` (`CkVoiceChat_Stats.h` now bound), incremented at the frame-consume, post-encode, conceal, and decode sites. Commit 136ca780f. | Re-gate on the rebuilt binary: **18/18 VoiceChat, all 3 lanes EXIT 0, 0 fails, 0 `Angelscript: Error`** (`Test-VoiceChatP2-statscounters.log`); pipeline line unchanged (`loopback decoded 115200 bytes (1.20 s)`). Freshness: source 03:36:52 → `BusterBlockEditor-CkVoiceChat.dll` 03:37:46 → run 03:38–03:40. |
| C2 | Conceal zero-fill was an unrecorded deviation | Recorded as **deviation 3** above, incl. the auditor's engine-surface verification and the P3 carry-forward. | doc-only |
| F3 | Synth created+started before the capture `Start()` gate — silent-synth residue on capture failure | Carried to **P3 open items** (PROGRESS.md) — fold the reorder into the P3 talker work, per the audit's own suggestion. | doc-only |
| F4 | Synth created in HandleRequests, not Setup | Recorded as **deviation 4** above. | doc-only |
| F5 | Capture hot-path allocations (raw-tap copy with zero listeners, per-frame allocs, O(n) front-shift) | Carried to the **P5 perf-pass ledger** (PROGRESS.md open items). None matter at 1 talker; all on the 8-talker path. | doc-only |
| F6 | `_AmplitudeQ8` flickers to 0 on frameless ticks | Carried to **P3 open items** (fix = decay or last-frame-hold; wire use unaffected — bundles only exist when frames exist). | doc-only |
| F7 | "Synth in the loop" headless is inferred, not log-proven | No action — the two HUMAN items are exactly the audible-path confirmation. | — |
