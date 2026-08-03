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
| C++ build green (editor closed) | `UnrealToolbox --build --config=Development --target=Editor` → `Result: Succeeded` (`Saved/Logs/Build-Editor-VoiceChatP0.log`); final binary includes CkTests 8808cae |
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
