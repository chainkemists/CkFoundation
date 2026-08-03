# Gate 1 — Codec pure layer, unit tests, encode/decode micro-benchmark (P1)

> **Status:** ✅ Work complete (2026-08-03) — ⏸ awaiting the top-tier audit (appended below when
> it lands). P2 does not start until it passes.
> **Depends on:** Gate 0 ✅ — audit verdict **GO WITH CONDITIONS** (b3a5c3cf6,
> [Gate_0_ReviewPackage.md](Gate_0_ReviewPackage.md) § Top-tier audit response)
> **Estimate:** 1 session — actual: same session as the Gate-0 close (2026-08-03)

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

- [x] Every expected observation confirmed; evidence in PROGRESS.md 2026-08-03 P1 entry
      (run 4: **10/10 passed, EXIT CODE: 0**, Test-VoiceChatP1-run4.log; 0 `Angelscript: Error`).
- [x] Benchmark recorded — see § Benchmark record below.
- [x] Audit condition C1 satisfied: `CkTests.UnitTests.CkVoiceChat.Channel.AddInvalidNameRejected`
      green — invalid ChannelName → ensure (whitelisted) + invalid handle + zero partial state on
      the host (no record, no channel fragments, no label), on a bare slot-table registry.
- [x] `ck-change-control` done-checklist (class 2 additive): compile green, tests reported as
      counts, AS boot clean; BP surface unchanged beyond categories (already in the
      `[EDITOR-VERIFY]` list).
- [x] This file's Status flipped; PROGRESS.md dated entry appended.
- [x] Audit section appended below by a fresh top-tier session; campaign pauses until its verdict.

## Benchmark record (2026-08-03, run 4)

`[VoiceBench] Opus 48 kHz mono @ 24000 bps, 20 ms frames, 500 timed frames: encode 61.3 us/frame,
decode 26.9 us/frame, avg encoded 47.7 B/frame (first decoded sizes: 1920 1920 1920)`

- Per-talker game-thread encode at 20 ms cadence ≈ **0.31% of one core** (61.3 µs / 20 ms) — the
  spec §7.5 encode-cost claim now exists as a measurement, and the "Low" risk rating stands.
- Decode for the full 8-talker cap ≈ 8 × 26.9 µs / 20 ms ≈ **1.1% of one core** on the audio-pull
  side.
- 47.7 B/frame average at 24 kbps VBR on voiced-band input — 3-frame bundles land ~150 B + wire
  framing, comfortably under the 256 B split threshold (S3).
- Engine-contract discoveries recorded for P2 (both found by failing first): `[Voice] bEnabled`
  must be true in the HOST PROJECT's DefaultEngine.ini or every factory returns null (the
  benchmark self-enables via the config cache); `FVoiceDecoderOpus::Decode` silently outputs 0
  unless the output buffer has ≥ `MAX_OPUS_FRAMES` (6) frames of capacity
  (fork `VoiceCodecOpus.cpp:20` + the `UncompressedBufferAvail` gate).

---

## Top-tier audit response (Gate 1)

### Verdict

**GO WITH CONDITIONS** — P2 may start immediately; the gate's contracted deliverables are all
delivered, green on the final binary, and C1 is satisfied. The conditions are mechanical and land
with early P2 work:

1. **Decide, implement (or document), and unit-test the jitter buffer's stream-discontinuity
   contract before P2 wires the synth pull to `Pop`.** Three coherence gaps share one root — the
   buffer has no notion of a talk-spurt boundary:
   - `Push` (`CkVoiceChat_Codec.cpp:73-81`) updates the inter-arrival jitter EWMA
     unconditionally, so a VAD-gated silence gap is counted as network jitter: any pause ≥ ~0.5 s
     makes the next arrival's deviation clamp target depth to `_MaxDepth` (200 ms = 10 frames),
     and the post-underrun re-warm (`Pop`, :107-113) then holds ~200 ms of audio before EVERY
     post-pause utterance starts, decaying only over ~20+ steady arrivals (EWMA α = 0.1). The
     adaptive depth was designed to track NETWORK jitter (ADR-3), not speech cadence.
   - If the frame seq advances across a silence gap, `Pop` walks the gap one `Conceal` per pop
     (:129-131) with no cursor resync — a G-frame forward gap yields G conceal pops AND a
     standing ~G-frame latency (arrivals refill at the same rate the cursor advances); if the seq
     instead restarts lower, `Push` late-drops everything up to ~32k frames silently (:83-87).
   - There is no explicit reset; `Buffer = FCk_VoiceChat_JitterBuffer{};` works (plain value
     type — no API break), but nothing states whether the P2 caller must do that per spurt
     (which also discards learned jitter + counters) or whether the buffer will handle
     discontinuities internally (skip EWMA update on implausible inter-arrival; resync cursor
     when the earliest buffered seq is beyond the max depth). Either answer is fine — it must be
     WRITTEN and tested, because S4 (teardown/stream-restart windows) and the VAD design both
     guarantee P2 hits it.
2. **Pin seq-wrap with a unit test.** `Get_SeqDistance` (`CkVoiceChat_Codec.cpp:25-28`) is
   correct by inspection (u16 subtract → int16 reinterpret), and the header claims wrap support
   (`CkVoiceChat_Codec.h:163-164`), but no test crosses 65535→0 (push/pop continuity + late-drop
   across the boundary). u16 wraps every ~22 min of continuous frames; P3 leans on this. Five
   lines in `Test_VoiceChat_Codec.cpp`.
3. **Benchmark config cleanup must restore, not remove, once P2 lands `[Voice] bEnabled=true` in
   the host DefaultEngine.ini** (a P2 obligation this gate itself recorded).
   `Test_VoiceChat_OpusBenchmark.cpp:73/83/90` does SetBool → RemoveKey; today the key does not
   exist so the cleanup is exact, but after P2 the RemoveKey strips a REAL project key from the
   in-memory config cache for the rest of a warm-editor session. Capture presence+value up
   front, restore on exit. Due with the P2 change that adds the key.

### Spot-checks performed

All on the working tree at CkFoundation `571e6b28f` / CkTests `0019a63` (both
`feature/voice-chat`), 2026-08-03. Every claim below is confirmed against the named source
unless marked inferred.

- **(A) Codec layer in full** — `CkVoiceChat_Codec.h` (301 lines) + `.cpp` (326 lines) read
  whole. Pure-layer discipline holds: includes are CkCore + engine containers only, no
  UObjects/ECS anywhere — matches the `CkRenderTarget_PixelMath` exemplar (compared side by
  side, including its `CK_GENERATED_BODY`/`CK_PROPERTY_GET`/`CK_DEFINE_CONSTRUCTORS` struct
  shape). House style: trailing returns throughout, `_`-members + `CK_PROPERTY(_GET)`,
  `CK_DEFINE_CONSTRUCTORS` essentials-only where essentials exist (VadParams/JitterParams
  correctly omit it — all-optional, per the Gate-0-noted precedent), filename-derived named
  namespace `ck_voice_chat_codec` (not anonymous), `NOT` macro, both ensures in the exact
  non-negotiable-#3 shape (hoisted side-effect-free condition, empty ensure body, separate
  ordinary early-out — `.cpp:150-155`, `:177-187`). Strict `Unpack_Bundle` contract verified by
  reading the parse loop (`.cpp:212-248`): unset on short header, truncated prefix, zero-size
  frame, truncated body, and trailing garbage (`Cursor != Num` at :244); allocation from hostile
  input is bounded (NumFrames ≤ 255, appends bounded by received bytes). Seq-wrap delta trick
  correct (:25-28). Pacing kernel: stale-drop filter runs BEFORE the budget pass and stale
  entries never consume budget (:295-303) — amendment S2's order honored; oldest-first FIFO with
  stop-at-first-misfit is deliberate and commented for the ordered transport (:305-310); an
  oversized head entry cannot wedge permanently (staleness eventually clears it).
  Pop/warmup/underrun-rewarm state machine traced by hand: warmup gates on target depth,
  underrun drops back to warmup, cursor can never advance past frames it should have played
  (Push drops behind-cursor frames, so the buffer only ever holds ≥-cursor seqs). The
  far-AHEAD-of-cursor case is the one coherence gap → condition 1. Minor start-edge behavior
  noted as finding 4.
- **(B) Unit tests** — `Test_VoiceChat_Codec.cpp` read whole (8 tests). Pins: wire round-trip +
  the S3 < 256 B bound (:59-62) + zero-frame-bundle symmetry; all five malformed classes;
  pack-side out-of-contract rejection with the ensure whitelisted; amplitude clamp + u8-grid
  identity; VAD attack/release incl. the accumulator-reset-on-recross contract; jitter warmup /
  reorder / conceal(+count) / late-drop(+count, not stashed) / underrun-rewarm; adaptive depth
  grow (bursty), MaxDepth ceiling, and the MinDepth-floor convergence on steady arrivals
  (:303-306 — the run-1 fix, asserted with the correct contract); pacing staleness + budget +
  drain-all. Not pinned: seq wrap (condition 2) and any pause/discontinuity arrival pattern
  (condition 1).
- **(C) Benchmark** — `Test_VoiceChat_OpusBenchmark.cpp` read whole. Config-cache enable +
  unload-if-loaded-disabled fallback + key removal on BOTH exit paths (:83, :90) — sound today;
  the P2 interaction is condition 3. Aggregate asserts are real contract asserts
  (every encode consumed its input; ≥ 500/510 frames decoded to full 20 ms PCM), timings logged
  not asserted — correct per non-negotiable #7. Both engine claims re-verified at fork source
  (`D:/Repositories/UnrealEngine-Angelscript`): `FVoiceModule::StartupModule` reads
  `[Voice] bEnabled` from `GEngineIni` exactly once (`VoiceModule.cpp:34-41`);
  `MAX_OPUS_FRAMES` = 6 at `VoiceCodecOpus.cpp:20` and the
  `UncompressedBufferAvail >= (MAX_OPUS_FRAMES * BytesPerFrame)` per-frame skip gate in
  `FVoiceDecoderOpus::Decode` (~:655-658) — a 1-frame output buffer silently yields 0, as the
  benchmark comment states. Benchmark-record arithmetic in this file checks out (61.3/20000 =
  0.31%; 8 × 26.9/20000 ≈ 1.1%; 3 × 47.7 + 6 + 5 ≈ 154 B < 256).
- **(D) C1** — `Test_VoiceChat_ChannelAdd_InvalidInput.cpp` read whole against
  `CkVoiceChannel_Utils.cpp:18-23`: the guard precedes ALL mutation, and the test asserts
  invalid handle out + no record (`Has_Any`) + host-not-a-channel (`Has`) + no label on the
  host, with the ensure whitelisted by its exact message. Bare slot-table registry usage
  mimics the `Test_Signal_InvalidHandle.cpp` precedent line-for-line
  (Allocate/create/handle/Free; same whitelist comment). C1 SATISFIED as demanded.
- **(E) Evidence chain** — final-binary green CONFIRMED, not stale: last CkFoundation source
  edit 02:09:27 (`Codec.h` comment) < `BusterBlockEditor-CkVoiceChat.dll` 02:10:38; last CkTests
  source edit 02:18:08 (`OpusBenchmark.cpp`) < `BusterBlockEditor-CkTests.dll` 02:18:45; run-4
  log finished 02:20:46, canary 02:23, regression 02:25. `Test-VoiceChatP1-run4.log`: all 10
  `Result={Success}` rows by name, `Passed: 10 / Failed: 0`, every lane `EXIT CODE: 0`, and the
  `[VoiceBench]` line matches this file's benchmark record verbatim. `Test-P1-Canary.log` 2/2,
  `Test-P1-Regression.log` 22/22, both EXIT 0; 0 `Angelscript: Error` across all three logs.
  `git log origin/dev..HEAD` matches PROGRESS in both repos (CkF 13 commits, codec layer =
  `59a7de954`; CkTests `9d9b9d2..0019a63` with the honest 4-run fix history). Trees clean but
  for the sibling's untracked `docs/digests/`. Nothing pushed.
- **(F) Adversarial pass** — CkTests.Build.cs delta vs the TRUE branch base (`417b062`) is
  exactly `+Voice, +CkVoiceChat` (a naive diff vs origin/dev misleadingly shows
  Niagara/CkParticles removals — that is origin/dev having moved ahead with a sibling's work,
  verified via merge-base; this branch removed nothing). `Voice` is an engine Runtime module
  whose own Build.cs handles server targets (`bDontNeedCapture`) — safe for CkTests
  (Runtime/Default) on all its targets. `CkVoiceChat.Build.cs` unchanged at P1: still the 7
  earned Ck deps from P0; the codec code consumes only CkCore + engine Core — dep budget (N4)
  honored; the Voice dep lives in CkTests where the benchmark earns it. Audit finding 7
  (per-feature BP categories) landed as `db1a7efb9`, verified in all three utils headers +
  Settings.

### Findings

1. **(Condition 1 — before P2 wires playback)** Jitter policy has no stream-discontinuity
   contract: silence gaps pollute the jitter EWMA (post-pause utterances warm at MaxDepth ≈
   +200 ms), a far-ahead seq gap conceal-walks with a standing latency, and reset-on-restart is
   possible but unstated. Details + file:lines in the verdict. Not a P1 blocker: the layer is
   correct for the continuous streams P1's tests model, and the fix is cheap and unit-testable
   in the pure layer.
2. **(Condition 2 — mechanical)** Seq-wrap claimed and correct by inspection but unpinned by any
   test. P3 leans on it.
3. **(Condition 3 — due with the P2 config change)** Benchmark's `RemoveKey` cleanup becomes
   destructive the moment the host project legitimately ships `[Voice] bEnabled=true`. Switch to
   capture-and-restore.
4. **(Non-blocking)** Start-of-stream cursor seeds from the FIRST arrival, not the lowest
   buffered seq at warmup (`CkVoiceChat_Codec.cpp:91-95`): if the very first packet arrives
   reordered-high, the earlier frames that arrive during warmup are late-dropped and playout
   clips the utterance onset by the reorder depth (~20-40 ms, one-time per stream). Cheap fix
   if P2 cares: snap `_NextPlayoutSeq` to the min buffered seq when warmup completes. Executor's
   call — fold into condition 1's work if convenient.
5. **(Non-blocking, cosmetic)** The three file-local helpers in `ck_voice_chat_codec`
   (`CkVoiceChat_Codec.cpp:10-28`) use single-line signatures where the exemplar's helpers
   (`CkRenderTarget_PixelMath.cpp`, `ck_render_target_pixel_math`) use the split house shape.
   Trivial; fix opportunistically.
6. **(Non-blocking, observation — no action)** The benchmark leaves the Voice module loaded and
   enabled for the remainder of a warm-editor session (module state, not config; the key itself
   is cleaned). Benign — unloading at test end would risk yanking the module out from under any
   other holder — but worth knowing when reading warm-server (`--live`) runs.
7. **(Positive, recorded so it isn't re-litigated)** Zero-frame bundles round-trip by design on
   both sides (pack allows an empty frame LIST; the wire rejects zero-SIZE frames) and the test
   pins the symmetry — this is consistent with the gate contract's wording, not a hole. The
   pacing kernel's stop-at-first-misfit (no skip-fit) is deliberate and correctly commented for
   an ordered transport.

### Auditor

Fresh top-tier session (did not author the work) — date 2026-08-03

---

## Conditions resolution (executor, 2026-08-03 — after the audit)

| Audit item | Resolution | Verified |
|---|---|---|
| Condition 1 — stream-discontinuity contract | In-buffer: arrival gap > `_ArrivalGapReset` (0.2 s default) or seq jump > `_MaxSeqJumpFrames` (50) reseeds at the incoming seq, re-warms, skips that arrival's jitter-EWMA update (estimate persists across spurts), counts `_DiscontinuityCount` (CkF 624bb5581) | `Jitter.SpurtBoundaryDiscontinuity` + `Jitter.FarAheadJumpReseeds` green |
| Condition 2 — seq-wrap test | `Jitter.SeqWrap` crosses 65535→0 incl. wrap-safe late-drop (CkTests a0a8bba) | green |
| Condition 3 — benchmark config cleanup | Capture-and-restore replaces SetBool→RemoveKey | green (run 5) |
| Finding 4 — warmup onset clip | During warmup, behind-cursor arrivals extend the window backward; `Jitter.WarmupBackwardExtend` pins it | green |
| Finding 5 — helper signature shape | Split to house shape | compile |

Run 5 (final binary, all of the above): **14/14 passed, EXIT CODE: 0** (Test-VoiceChatP1-run5.log),
0 `Angelscript: Error`, benchmark stable (encode 63.5 µs / decode 24.7 µs / 47.7 B per frame).
