# Gate 2 — VoiceTalker capture pipeline + local loopback playback (P2)

> **Status:** 🟡 In progress (2026-08-03)
> **Depends on:** Gate 1 ✅ (audit GO WITH CONDITIONS — all conditions resolved, 2ed23fff0)
> **Estimate:** 1–2 sessions — re-date at entry; record actual at exit

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

## Exit criteria — ALL land with the gate-closing commit

- [ ] AutoTests green (counts vs the 14/14 P1 baseline + new tests named).
- [ ] Spike canaries + RenderTarget suite delta-zero.
- [ ] AS boot clean; new AS surface (requests/signals) exercised from an AS test if feasible.
- [ ] `[EDITOR-VERIFY]` list for the human (mic loopback, settings audition).
- [ ] N5 packaged smoke run + result recorded (or explicitly listed as the human's step with
      exact commands if packaging infra requires it).
- [ ] PROGRESS.md dated entry; this file's Status flipped; audit section appended; campaign
      pauses for the Gate-2 audit.
