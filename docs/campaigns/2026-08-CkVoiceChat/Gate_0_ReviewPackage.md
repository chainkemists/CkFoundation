# Gate 0 — review package for the top-tier audit

> **Written:** 2026-08-03 by the P0 executor session. The campaign is PAUSED at the P0 gate per
> the kickoff instruction; P1 does not start until this audit passes. Volatile state:
> [PROGRESS.md](PROGRESS.md). Contract: [Gate_0.md](Gate_0.md). Spike verdict:
> [SpikeMemo_P0_UnreliableUnicastClientRPC.md](SpikeMemo_P0_UnreliableUnicastClientRPC.md).

## What Gate 0 delivered

**CkFoundation `feature/voice-chat`** (base `d02278cdd` = origin/dev; nothing pushed):

| Commit | Content |
|---|---|
| 7a66ff11e | CTO review cherry-picked from local dev (rides the branch) |
| 21bf89232 | Design spec force-added past `.gitignore` `*.md` |
| ffa02f6c0 | Campaign doc set (PROMPT.md with N1–N8 ledger + rulings, Gate_0.md, PROGRESS.md) |
| 9b0a55d22 | **P0 module skeleton** — 28 files: 3 feature quartets, Settings, Log/Stats/Module, uplugin entry, `Source/CLAUDE.md` tier row + decision-tree row, module `Claude.md` stub |
| 1ff7decf9 | **ADR-4 doctrine amendment** — `Source/CkActorRelay/CLAUDE.md` Anti-patterns split (event channels events-only verbatim; relay actors may carry paced budgeted streams under clause (a)–(g); adopters named) |
| 146585209 | Spike memo draft + amendments S1–S4 in PROMPT.md |
| (gate-close commit) | Memo finalized (verdict PROCEED), S5, this package, Gate_0/PROGRESS updates |

**CkTests `feature/voice-chat`** (base `417b062` = origin/dev; nothing pushed): 9d9b9d2 →
8808cae — throwaway spike vehicles + `Ck.VoiceChat.Spike.*` specs (green at 8808cae; flagged for
deletion with the campaign's P0 artifacts).

**BusterBlock superproject:** untouched — no gitlink bump, no commits. One follow-up chip filed
(dead `net.Iris.SaturateBandwidth` config line).

## Exit-criteria evidence

| Gate_0 exit item | Evidence |
|---|---|
| C++ build green (editor closed) | `UnrealToolbox --build --config=Development --target=Editor` → `Result: Succeeded`. Final-binary freshness per the audit's verified chain (audit condition 2): `BusterBlockEditor-CkTests.dll` mtime 01:10 (after the 8808cae edit at 01:09:51) → spike run 3 booted 01:11+ → regression finished 01:16 |
| AS bindings clean | Run-3 test boot: **0** `Angelscript: Error` lines (`Test-VoiceChatSpike3.log`); all four wrappers emitted to `Script/Generated/` (`utils_voice_talker/channel/listener/chat_settings.as`, gitignored-by-design) |
| Spike memo committed, questions answered | Memo FINAL, verdict **PROCEED — ADR-4 holds**; STOP condition (pathological drop/starvation) not met by mechanism or measurement |
| Spike tests green | Run 3: `Total: 2, Passed: 2, Failed: 0` (`Test-VoiceChatSpike3.log`, `EXIT CODE: 0`) |
| Doctrine amendment landed | 1ff7decf9; clause text matches the CTO review's (a)–(g) verbatim in substance |
| Regression check | Adjacent suite (RenderTarget — the relay/net surface): **22/22 passed, 0 failed**, `EXIT CODE: 0`, 2m 48s, on the final binary (`Test-RenderTargetRegression.log`) |
| PROGRESS.md updated with real evidence | Dated entries 2026-08-03 (commands, exit codes, log lines, run record) |

**Baseline caveat (honest gap):** no pre-change test baseline was captured before the skeleton
landed — the branch is purely additive off origin/dev, and the regression evidence is the
adjacent RenderTarget suite run on the final binary plus three clean editor boots (module load +
processor registration + AS compile). If the auditor wants delta-zero against the full 1324-test
suite, that is a ~10–20 min toolbox run not performed this session.

## Executor decisions to veto cheaply (none block P1 if reversed)

1. **Spike vehicle:** plain replicated actor copying the relay channel's wire shape
   (`bReplicates + bAlwaysRelevant + Owner=PlayerState`) instead of subclassing abstract
   `ACk_ActorRelay_UE` — the base adds only group-subsystem registration bookkeeping (read in
   full; no netcode), and a subsystem-less subclass spins its registration retry loop.
2. **P0 skeleton scope:** no requests/signals/Codec/Net/Playback files, and no VoiceListener
   processor files — deferred to the phases that implement them (spec §7.1 shows the full final
   layout; empty stubs are dead weight). Recorded in PROGRESS decision log.
3. **Dep list:** 7 Ck deps + DeveloperSettings/GameplayTags (vs spec §7.1's 13) per review N4
   "every dep earned"; the phase-by-phase add plan is in the Build.cs comment and tier row.
4. **Data shapes:** Talker `Current` carries only wire-anchored `_Seq`/`_AmplitudeQ8`; Channel
   has Params only (`Has` = Params) until routing state exists at P3; Listener `Current` =
   mute set + volume map with real getters.
5. **Settings types:** jitter depths as `FCk_Time` (house rule "time properties use FCk_Time")
   vs the spec's ms integers; codec `_FrameSizeMs` stays int (an Opus frame-size selector, not a
   duration).
6. **Spike tests stay committed and green** in CkTests under `Ck.VoiceChat.Spike.*` — they're
   marked THROWAWAY in-file; delete at campaign end or keep as transport canaries (auditor's
   call).

## What to audit (suggested spot-checks)

- **Doctrine amendment wording** vs the review's constraint clause —
  `Source/CkActorRelay/CLAUDE.md` § Anti-patterns (the one doctrine-level artifact).
- **Skeleton style compliance** — pick one quartet (VoiceChannel is the richest:
  child-entity Add + ensure shape + record + label) against CkTimer/CkAudio exemplars.
- **The memo's verdict logic** — the STOP condition was defined for drop/starvation; the spike
  found lossless-but-queueing behavior and rules PROCEED with S1–S5 as the latency bound. If the
  auditor reads the latency data as STOP-worthy despite the amendments, ADR-4 re-opens per the
  review's own terms.
- **Spike claims** — the three load-bearing Iris citations were re-verified by hand
  (`NetRPCHandler.cpp:24-33`, `AttachmentReplication.cpp:215-226`,
  `PartialNetObjectAttachmentHandler.h:30`); the rest of the mechanism table came from a
  dedicated source-reading pass and is cited file:line in the memo.

## `[EDITOR-VERIFY]` — human steps not performable by this session

1. Open the BB editor → any BP graph → right-click → search `[Ck][VoiceTalker]` /
   `[Ck][VoiceChannel]` / `[Ck][VoiceListener]`: Add/Has/TryGet/getter nodes appear under those
   DisplayNames; enum params render as dropdowns (`ECk_VoiceChat_TransmitMode`,
   `ECk_VoiceChat_SpatializationPolicy`).
2. `[Ck][VoiceChannel] Cast` shows Succeeded/Failed exec pins; the `<AsVoiceTalker>` /
   `<AsVoiceChannel>` / `<AsVoiceListener>` autocast nodes accept a generic `FCk_Handle` wire.
3. Project Settings → search "Voice Chat": the Ck settings page shows Codec/Transport/Routing/
   Playback categories with the documented defaults (48000 / 24000 / 20 ms / 3 / 4096 / 8 /
   100 cm / 0.02 s / 0.06 s / 0.2 s).
4. `Make FCk_Fragment_VoiceChannel_ParamsData` node lists the private `_`-prefixed fields.

## Recommendation

**GO for P1** (Codec pure layer + unit tests + encode/decode micro-benchmark), with S1–S5 binding
on P3 and the N-notes ledger unchanged. The campaign holds at this gate until the audit says so.

---

## Top-tier audit response

### Verdict

**GO WITH CONDITIONS** — P1 may start immediately; the conditions are mechanical and land with
normal P1 work:

1. **Land the invalid-input test for the one new validation boundary** — root non-negotiable #3's
   closing requirement ("every new validation boundary requires a focused invalid-input test
   proving rejection, zero downstream mutation, no partial state") is currently unmet for
   `UCk_Utils_VoiceChannel_UE::Add` (`CkVoiceChannel_Utils.cpp:18-23`): a test that calls Add with
   an invalid `_ChannelName`, asserts the returned handle is invalid, and asserts NO record/label/
   fragment was added to the host. Due no later than the P1 gate close (P1 is the first
   test-bearing phase; home is CkTests per house rule).
2. **Correct the build-evidence citation in this package's exit-criteria table.**
   `Build-Editor-VoiceChatP0.log` (mtime 00:59) is the CkVoiceChat-skeleton build and CANNOT be
   the evidence that "final binary includes CkTests 8808cae" (committed 01:09:51) — that rebuild's
   log was not archived under a named file. The claim itself is TRUE — verified independently by
   this audit: `Binaries/Win64/BusterBlockEditor-CkTests.dll` mtime 2026-08-03 01:10 (after the
   8808cae edit), spike run 3 booted 01:11+ (in-log `05.11.32` UTC), RenderTarget regression
   finished 01:16 — so the green is NOT stale; only the citation was. Fix the table row or point
   it at this section's verified chain.

Ruling on executor decision 6 (delegated to the auditor): **keep the spike tests committed and
green** as transport canaries for the campaign's duration — they directly guard the ADR-4
mechanism assumptions (Ordered flag, drain behavior, split threshold) against engine-fork updates,
and their drain-to-completion structure makes them robust rather than window-flaky. Delete at
campaign end per plan unless P3 deliberately promotes them.

### Spot-checks performed

All file:line references verified against the working tree at `ae6894c46` (CkFoundation) /
`8808cae` (CkTests) on 2026-08-03.

- **(A) Doctrine amendment** — `Source/CkActorRelay/Claude.md:52-77` read in full + the
  `1ff7decf9` diff. All seven clause points (a)–(g) carried faithfully against the CTO review
  §ADR-4 pt 4 (review doc lines 147-156); adopters named with their reliability classes (:77);
  the mechanism-split rationale (replicated fragment state wrong for lossy streams) carried.
  Events-only rule: substance preserved exactly; wording necessarily re-scoped ("relay channels" →
  "the Broadcast/Bind channel mechanism") — see finding 3.
- **(B) VoiceChannel quartet in full** — `CkVoiceChannel_Fragment_Data.h` (typesafe handle
  declared there :42, not in `_Fragment.h`; ParamsData canonical shape with `_`-members,
  `CK_PROPERTY_GET` for the essential `_ChannelName`, `CK_DEFINE_CONSTRUCTORS` essentials-only
  :96; enum formatter :37); `_Fragment.h` (tag, Params alias, TRANSIENT record with why-comment);
  `_Processor.h/.cpp` (`ck_exp::TProcessor` shape matches `CkTimer_Processor.h:13-36` exactly;
  `FGroup_Gameplay_Audio` exists at `CkProcessorGroups.h:55` and is CkAudio's group;
  `CK_REGISTER_PROCESSOR` at .cpp:7); `_Utils.h/.cpp` (Add mirrors
  `CkAudioTrack_Utils.cpp:17-46` — CreateEntity_AsTypeSafe → Label → fragments → NeedsSetup →
  record AddIfMissing+Connect; ensure shape is the exact non-negotiable-#3 form at :18-23:
  hoisted local, empty ensure body, separate ordinary early-out; trailing returns everywhere;
  `Get_ValidEntry_ByTag` exists at `CkRecord_Utils.h:127`). No stock ensure/check anywhere in the
  module (rg: zero hits). `Get_InvalidHandle() { return {}; };` matches `CkTimer_Utils.h:113`
  character-for-character.
- **(B) Build.cs + Settings** — 7 Ck deps, each consumed by P0 code (EcsExt→`UCk_Utils_Ecs_Base_UE`,
  Label→`CkVoiceChannel_Utils.cpp:30`, Record→record-of-channels, Settings→
  `UCk_Plugin_ProjectSettings_UE` base confirmed at `CkProjectSettings.h:10`, same base as
  RenderTarget). No Relationship/Timer/ActorRelay/SpatialQuery/Profile — N4 honored. Settings
  getters match the RenderTarget constexpr-fallback shape; jitter depths are `FCk_Time` (house
  rule), `_FrameSizeMs` int with the Opus-selector justification recorded; `ECk_EnableDisable`
  over bool for `_Vbr`. Defaults match the EDITOR-VERIFY list (48000/24000/20/3/4096/8/100/
  0.02/0.06/0.2).
- **(C) VoiceTalker + VoiceListener quartets** — same rules hold. Talker Add composes directly on
  the target (correct simpler-feature shape) and `return Cast(InHandle)` matches the dominant
  house pattern (CkAggro/CkAudioDirector/CkTransform). Talker Params omitting
  `CK_DEFINE_CONSTRUCTORS` (no essentials) has ample precedent (CkAudioTrack, CkCamera, …).
  Current fragments friend-scoped with `CK_PROPERTY_GET` only. Listener volume lookup
  default-returns 1.0 — reasonable identity.
- **(D) Memo verdict logic + evidence** — memo read in full. The three load-bearing Iris
  citations RE-VERIFIED BY HAND in the fork: `NetRPCHandler.cpp` Ordered-for-all-unicast with the
  quoted engine comment (confirmed at the cited window), `AttachmentReplication.cpp`
  `Reliable | Ordered` → `FReliableSendQueue` routing (confirmed), and
  `PartialNetObjectAttachmentHandler.h:30` `ServerUnreliableBitCountSplitThreshold = (256)*8`
  (confirmed at exactly :30). Every empirical number in the memo's tables matches
  `Test-VoiceChatSpike3.log` verbatim (Phase A 300/300 lag −2/−2.0/−2 wire 76,954; B 2400/2400
  max 298 wire 512,574; C 2400/2400 37/484/641 wire 1,109,126; burst 25/25, 0 net warnings;
  0 `Angelscript: Error`). [CONFIRMED]/[INFERRED] discipline held throughout, including the
  honest run-1→3 defect history and the labeled lag-tick→wall-clock inference.
- **(E) Commits + tree state** — CkFoundation `origin/dev..HEAD` = exactly the 7 commits in the
  package table (gate-close = `ae6894c46`, docs-only, verified by --stat); skeleton commit is
  28 files as claimed; `git status` shows only the sibling's untracked `docs/digests/`. CkTests
  = 6 commits `9d9b9d2..8808cae`, tree clean. Nothing pushed (both branches have no upstream
  counterpart in the listed ranges).
- **(F) Adversarial pass** — uplugin entry matches the CkRenderTarget row shape exactly
  (Runtime/Default/Win64+Mac+Linux) and is alphabetically placed; `Source/CLAUDE.md` tier row
  (:224) matches Build.cs deps exactly, module count updated 75→76, decision-tree row added
  (:61); spike test code read in full (findings 6–7); stale-green audit run (condition 2);
  4 AS wrappers present on disk (`Script/Generated/utils_voice_*.as`, 00:59).

### Findings

**Blocking: none.**

Non-blocking (numbered; none reshape P0 output):

1. **Missing invalid-input test for the Add validation boundary** — promoted to condition 1
   (the only doctrine-grade gap found).
2. **Build-evidence citation imprecise** — promoted to condition 2; the underlying green is
   verified real, not stale.
3. **"Verbatim" is approximately true for the events-only rule.** Old: "Don't use relay channels
   for high-frequency data … relay is for events." New (`Claude.md:54-56`): "Don't use the
   Broadcast/Bind channel mechanism for high-frequency data … relay events are for events." The
   re-scoping is the amendment's entire point (the old words "relay channels" were the ambiguity
   being resolved), and the package's own claim was the honest "verbatim in substance." No action.
4. **Spike memo's ~850 B/tick PIE drain figure (S5) is derived, not tabled.** It back-computes
   from Phase B/C drain durations but the memo never shows the arithmetic. Consistent with the
   logged data (checked), and S5 makes it a P3 measurement anyway — but note the margin it
   implies: spike-config drain ≈ 51 kB/s at 60 ticks/s vs ~43 kB/s offered by 8 talkers — under
   20% headroom, and a 30 Hz server tick would invert the sign. This is why S5 deserves its
   "binding" status; it is the single most likely place ADR-4 legitimately re-opens. No action
   now.
5. **`CkVoiceChat_Stats.h` is included by no TU** — `STATGROUP_CkVoiceChat` is declared but never
   compiled into the binary, so it does not exist at runtime. Harmless dead weight at P0 (clause
   (f)'s "stats from day one" binds the stream code at P3), but mildly inconsistent with the
   executor's own "empty stubs are dead weight" principle. Include it from the first file that
   declares a counter (P2/P3); no action now.
6. **Spike suite-pollution check: clean.** The two specs are self-contained (own PIE session via
   `FCk_Latent_StartPIEMultiClient` + `FCk_Latent_EndPIE`, engine map `/Engine/Maps/Entry`, no Ck
   entities, no shared-world state). The static `GNetLogCounter` is Reset() on registration and
   static-storage-safe if a latent chain aborts (`FOutputDeviceRedirector` add is duplicate-safe);
   worst case is a stale registered device that counts silently — no test-visible effect since
   the count is only read inside test 2 after its own Reset. Drain-to-completion + 90 s timeout
   makes the delivery test robust on slow machines (it measures completion, not a fixed window).
   `bSuppressLogErrors/Warnings` is correct for a spike that deliberately saturates the net stack.
7. **Category metadata deviates from the per-feature norm.** All three features share
   `Category = "Ck|Utils|VoiceChat"`, where the observed house pattern is per-feature
   (`Ck|Utils|AudioTrack`, `Ck|Utils|Attribute|Float`, `Ck|Utils|Inventory` vs
   `Ck|Utils|ItemResolution`). DisplayNames (`[Ck][VoiceTalker] …`) disambiguate, so this is
   cosmetic BP-menu grouping only; if changed, `Ck|Utils|VoiceChat|Talker/Channel/Listener` is
   the natural shape. Metadata-only churn — executor's call, any phase.
8. **Memo verdict PROCEED is sound (spot-check D ruling).** The STOP condition ("pathological
   drop/starvation") is genuinely unmet by both mechanism (no drop site exists on the
   ordered-unreliable path short of the silent 4096 pre-queue rejection; unfitting attachments
   roll back and reschedule) and measurement (100% drain under 2.4× overdrive × 8 streams plus
   64 KB/tick state churn, with the reliable yardstick also 30/30). The latency data is NOT
   STOP-worthy: it was produced by deliberately unpaced overdrive, and the design has always been
   constitutionally required to pace (clause (d)/(g)); S2 converts that obligation into a
   freshness bound, which the send side fully controls. The genuinely new fact — unicast
   Unreliable is Ordered and rides the reliable-queue machinery — changes the mechanism, not the
   achievable end-to-end semantics (sent-once/never-resent still yields silent holes on real wire
   loss, which Opus PLC covers). Run 2's cutoff-window numbers (33.5% at fixed window) could have
   been misread as starvation; the executor correctly restructured to drain-to-completion and
   proved backlog-outliving-window instead. Amendments S1–S5 are the correct response, and S5
   keeps the re-open trigger alive at exactly the right place (finding 4).

### Auditor

Fresh top-tier session (did not author the work) — date 2026-08-03
