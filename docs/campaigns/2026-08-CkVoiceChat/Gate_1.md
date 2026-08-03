# Gate 1 — Codec pure layer, unit tests, encode/decode micro-benchmark (P1)

> **Status:** 🟡 In progress (2026-08-03)
> **Depends on:** Gate 0 ✅ — audit verdict **GO WITH CONDITIONS** (b3a5c3cf6,
> [Gate_0_ReviewPackage.md](Gate_0_ReviewPackage.md) § Top-tier audit response)
> **Estimate:** 1 session — re-date at entry; record actual at exit

## Goal

After this gate: `Source/CkVoiceChat/Public/CkVoiceChat/Codec/` exists as a pure-function layer
(no UObjects, no ECS — the `ck::render_target::pixel` shape) covering the wire format, amplitude
quantization, VAD gating, jitter-buffer policy, and pacing math; all of it unit-tested headless
in CkTests; and an Opus encode/decode micro-benchmark through the engine `Voice` factories is
recorded (per non-negotiable #7, the spec's §7.5 encode-cost claim stays unmade until this
number exists).

## Entry criteria (pre-flight)

- [x] Gate 0 audit GO (b3a5c3cf6); conditions carried into this gate's work items (C1 → work
      item 3; C2 already landed with the audit commit).
- [x] Baseline: spike suite 2/2 green (Test-VoiceChatSpike3.log), RenderTarget suite 22/22
      (Test-RenderTargetRegression.log), both on the binary at CkF ae6894c46 / CkTests 8808cae.
- [x] Plan shapes spot-checked against current doctrine: pure-layer exemplar
      `CkRenderTarget_PixelMath.h` (namespace API, TOptional-rejecting deserializers), unit-test
      exemplar `Test_RenderTarget_PixelMath.cpp` (`CkTests.UnitTests.<Module>.<Subject>.<Scenario>`,
      ProductFilter, deterministic LCG fixtures).

## Work items

1. **`Codec/CkVoiceChat_Codec.{h,cpp}`** — mimicry of `CkRenderTarget_PixelMath` (pure, no
   UObjects/ECS), namespace `ck::voice_chat::codec`:
   - Wire format (spec §7.3, settled): `FCk_VoiceChat_BundleHeader` {Seq u16, ChannelIdx u8,
     AmplitudeQ8 u8, NumFrames u8} + per-frame [u16 size][bytes]. `Pack_Bundle` /
     `Unpack_Bundle` (unpack returns unset on any malformed/truncated input — the PixelMath
     `Deserialize_Deltas` contract). Serialized bundle must stay < 256 B for the spec defaults
     (amendment S3) — asserted in tests, not at runtime.
   - Amplitude: `Quantize_Amplitude(float 0..1) -> uint8` / `Dequantize_Amplitude` (linear v1,
     documented).
   - VAD: `FCk_VoiceChat_VadState` + `Step_Vad(state, frame RMS, dt, threshold, attack, release)`
     — pure hysteresis gate; attack/release are inputs (FCk_Time), owner-tunable at P2.
     `Compute_Rms(PCM int16 span) -> float` alongside.
   - Jitter policy (ADR-3's unit-testable core): `FCk_VoiceChat_JitterState` +
     `Push_Frame(state, seq, now)` / `Pop_Frame(state, now) -> {Frame | Conceal | Wait}` +
     adaptive target depth from inter-arrival variance (grow/shrink between min/max). Late
     (below-window) frames drop with a counter — never stash (N1's philosophy at the buffer).
   - Pacing math (S2's kernel): `Select_BundlesToSend(queue with ages, byteBudget, maxAge)` →
     send list + stale-drop list. Freshness beats completeness by design.
2. **C++ unit tests** — `CkTests/.../UnitTests/CkVoiceChat/Test_VoiceChat_Codec.cpp`:
   pack/unpack roundtrip + truncation/garbage rejection + size-bound check; amplitude quantize
   roundtrip + clamping; VAD attack/release sequences against synthetic RMS series; jitter
   reorder/gap→PLC/late-drop/adaptive-depth grow+shrink; pacing budget/staleness cases.
3. **Audit condition C1:** invalid-input test proving `UCk_Utils_VoiceChannel_UE::Add` with an
   invalid `_ChannelName` returns an invalid handle and composes NOTHING on the host (no record,
   no label, no fragments) — CkTests, world-backed test layer.
4. **Micro-benchmark** — CkTests automation test creating the engine `Voice` factories
   (`FVoiceModule::Get().CreateVoiceEncoder/Decoder(48000, 1, ...)`), encoding/decoding N×20 ms
   synthetic frames, reporting µs/frame (log + PROGRESS). CkTests Build.cs earns the `Voice`
   dep. Numbers recorded, no perf claims beyond them.
5. **Audit finding 7 (executor accepts):** per-feature BP categories
   `Ck|Utils|VoiceChat|Talker/Channel/Listener` — metadata-only commit.
6. NOT this gate: wiring `CkVoiceChat_Stats.h` (audit finding 5 — binds to the first real
   counter at P2/P3); any capture/engine-Voice code inside CkVoiceChat itself (P2).

## Expected observations at the gate — and what to do on each branch

| I will run | I expect to observe | If instead I see | Prewritten response |
|---|---|---|---|
| Toolbox `--build` | Exit 0 | Compile errors | Mine by construction; fix |
| `--test --test-pattern UnitTests.CkVoiceChat --discover-fresh` | All codec unit tests green headless | Red | Fix the pure layer — no world, so failures are deterministic |
| C1 invalid-input test | Invalid handle out; host has no record/label/channel fragments; the ensure fires once (suppressed in the runner) with NO downstream mutation | Partial state on the host | Fix Add's early-out path — that IS the non-negotiable-#3 bug the test exists to catch |
| Benchmark run | A recorded µs/frame figure for encode and decode at 48 kHz mono 20 ms | Factories unavailable headless (`DoesPlatformSupportVoiceCapture` gates capture, not codec) | Record the failure mode verbatim; codec-factory availability headless is itself P2-relevant data |
| Spike canaries + RenderTarget suite | Still green (delta-zero vs Gate-0 baseline) | New reds | A/B stash; own-change vs pre-existing before touching anything |

## Exit criteria — ALL land with the gate-closing commit

- [ ] Every expected observation confirmed; evidence in PROGRESS.md.
- [ ] Benchmark numbers recorded in PROGRESS.md + memo-style note in this file.
- [ ] Audit condition C1 satisfied (test green, named).
- [ ] `ck-change-control` done-checklist (class 2 additive).
- [ ] This file's Status flipped; PROGRESS.md dated entry.
- [ ] Gate-review package (or appended section) for the top-tier audit; campaign pauses until
      audited.
