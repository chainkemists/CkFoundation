# Gate 2 — VoiceTalker capture pipeline + local loopback playback (P2)

> **Status:** 🟡 Machine portion COMPLETE (2026-08-03) — ⏸ awaiting the Gate-2 audit + the two
> HUMAN items (mic `[EDITOR-VERIFY]`, N5 packaged smoke). P3 does not start before all three.
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
