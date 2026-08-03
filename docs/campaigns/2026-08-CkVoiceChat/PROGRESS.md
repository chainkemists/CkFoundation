# CkVoiceChat — PROGRESS.md (living log)

## Current state  <!-- supersedes everything below; update at EVERY gate and session end -->
**As of 2026-08-03 (CkFoundation `feature/voice-chat` at the Gate-0 close commit; CkTests
`feature/voice-chat` at 8808cae):** **Gate 0 work COMPLETE — campaign PAUSED awaiting the
top-tier audit** ([Gate_0_ReviewPackage.md](Gate_0_ReviewPackage.md)). Spike verdict:
**PROCEED — ADR-4 holds**; amendments S1–S5 binding on P3.
**Baseline being diffed against:** waived for the purely-additive branch (honest gap flagged to
the auditor); regression evidence = RenderTarget suite 22/22 on the final binary + 3 clean boots.
**Next action:** ~~top-tier audit~~ DONE 2026-08-03 — **GO WITH CONDITIONS** (b3a5c3cf6): C1
invalid-input test for VoiceChannel::Add due at P1 gate close; C2 citation fix landed with the
audit commit. **Gate 1 OPEN** ([Gate_1.md](Gate_1.md)) — Codec pure layer + unit tests +
encode/decode micro-benchmark in progress.
**Blocked on:** nothing. Nothing pushed anywhere; superproject gitlink untouched.

## Decision log
| Date | Decision | Why | Revisit when |
|---|---|---|---|
| 2026-08-02 | ADR-4 blessed (paced budgeted relay-actor streams) with clause (a)–(g); UChannel = named fallback only | CTO adjudication | P0 spike shows pathological drop/starvation, or P3 profile shows relay overhead |
| 2026-08-02 | Dep budget: minimal earned deps at P0 (`CkCore/Ecs/EcsExt/Label/Log/Record/Settings` + engine `Core/CoreUObject/Engine/GameplayTags/DeveloperSettings`); no Relationship ever; Timer only if earned; ActorRelay/SpatialQuery/ResourceLoader/Profile added at the phase whose code consumes them | Review N4 ("every dep must be earned") | Each phase's Build.cs review |
| 2026-08-03 | P0 skeleton carries NO requests/signals/Codec/Net/Playback files — deferred to the phase that implements them; VoiceListener has no Processor files until P3 | Skeleton = compiling topology, not speculative surface; empty files are dead weight | P1 (Codec), P2 (Talker requests/signals/Playback), P3 (Net, Listener processors) |

## Dated entries (append-only, newest first)

### 2026-08-03 — Gate 0 closed: build + AS + spike green; verdict PROCEED
- Ran: `UnrealToolbox --build --config=Development --target=Editor` → `Result: Succeeded`
  (Build-Editor-VoiceChatP0.log). Machine gotcha recorded: `--generate` fails here (no VS2022
  IDE — project-file generation needs it, UBT does not); a first failed toolbox invocation also
  sat on the machine-wide build lock without exiting and had to be killed (own process, verified
  by start time + already-failed log).
- Ran: spike suite three times (fix cycle recorded in the memo's Run record). Final:
  `--test --test-pattern VoiceChat.Spike --discover-fresh` → **2/2 passed, EXIT CODE: 0**
  (Test-VoiceChatSpike3.log). Numbers: Phase A 300/300, lag −2 const (zero added latency),
  ~214 B/RPC; Phase B 2400/2400 drained, lag max 298; Phase C (+60 KB/t churn) 2400/2400, lag
  37/484/641; unresolved-target burst 25/25 delivered, 0 net warnings.
- Ran: AS gate — 0 `Angelscript: Error` in the run-3 boot; 4 `utils_voice_*` wrappers emitted.
- Ran: adjacent regression — `--test --test-pattern RenderTarget` → **22/22 passed, EXIT 0**,
  2m48s (Test-RenderTargetRegression.log), on the final binary.
- Confirmed: the three load-bearing Iris claims re-read by hand (`NetRPCHandler.cpp:24-33`
  Ordered flag on all unicast RPCs; `AttachmentReplication.cpp:215-226` Ordered→reliable-queue
  routing; `PartialNetObjectAttachmentHandler.h:30` 256 B server unreliable split threshold).
- Spike harness defects found+fixed en route (in the committed history, not hidden): 65,536-byte
  pressure payload tripped Iris's 65,535-element array cap; send-window fencepost (299/300);
  AddInfo lines don't reach captured logs (→ UE_LOG); fixed-settle windows measure the window,
  not the transport (→ drain-to-completion + lag as the pressure metric).
- Inferred (unconfirmed, named for the auditor): BP-editor surface renders correctly
  (`[EDITOR-VERIFY]` steps in the review package); production-config drain budget (S5 makes it a
  P3 measurement); lag-tick→wall-clock conversion assumes ~60 fps PIE ticking.

## Decision log (Gate-0 close additions)
| Date | Decision | Why | Revisit when |
|---|---|---|---|
| 2026-08-03 | Spike verdict PROCEED; latency (not loss) is the pressure response; S1–S5 bound it | Memo § Verdict — mechanism + measurement agree | P3 profile (S5 measurement) contradicts |
| 2026-08-03 | Spike tests stay committed + green under `Ck.VoiceChat.Spike.*`, marked THROWAWAY | Red-by-design tests would pollute the suite; green canaries are harmless | Campaign end (delete) or auditor keeps them |

### 2026-08-03 — skeleton + doctrine + spike code landed; build gate running
- Ran: scaffold committed as CkFoundation 9b0a55d22 (28 files: 3 quartets, Settings, Log/Stats/
  Module, uplugin entry, tier row, module Claude.md stub); doctrine amendment 1ff7decf9
  (CkActorRelay Anti-patterns split: event channels events-only verbatim; relay actors may carry
  paced budgeted streams under clause (a)-(g)); campaign docs ffa02f6c0.
- Ran: CkTests branch `feature/voice-chat` cut from origin/dev (417b062); spike committed 9d9b9d2
  (channel actor mirroring relay wire shape bReplicates+bAlwaysRelevant+Owner=PlayerState per
  CkActorRelay_Actor.cpp:23-24 / _GroupSubsystem.cpp:452; pressure actor churning a 64 KB
  replicated payload; specs Ck.VoiceChat.Spike.{UnreliableUnicast_DeliveryUnderLoad,
  UnresolvedTarget_SilentDrop}).
- Confirmed: Iris is ON for the host project (`net.Iris.UseIrisReplication=1`,
  BusterBlock/Config/DefaultEngine.ini:6).
- Confirmed: pre-commit sweep clean — no stock ensure/check in Source/CkVoiceChat (rg exit 1);
  every mimicked API verified at source (Request_CreateEntity_AsTypeSafe, Get_ValidEntry_ByTag,
  record policies, ck::IsValid(FGameplayTag) at CkIsValid_Defaults.h:94, FCk_Time ctor
  CkTime.h:85, GameplayLabel Add CkLabel_Utils.h:24-26).
- Decision (spike vehicle): plain replicated actor copying the relay channel's wire shape instead
  of subclassing abstract ACk_ActorRelay_UE — the base adds only group-subsystem registration
  bookkeeping (no netcode; verified by reading CkActorRelay_Actor.h in full), and a subsystem-less
  subclass would spin its registration retry loop. Transport composite is identical.
- Inferred (pending): Iris unreliable-attachment mechanics — Explore agent reading the fork's
  Iris source (queue/drop policy, unresolved-target path, per-RPC overhead, SaturateBandwidth
  cvar); build gate running in background (Build-Editor-VoiceChatP0.log).

### 2026-08-03 — Gate 0 opened: branch, docs, research
- Ran: `git checkout -b feature/voice-chat origin/dev` (base d02278cdd) → cherry-pick d6f6edd6c
  → 7a66ff11e (CTO review rides the branch); `git add -f docs/specs/2026-08-02-CkVoiceChat-technical-review.md`
  → 21bf89232. Superproject gitlink untouched.
- Confirmed: editor-lock probe found `BusterBlock_2.log` + `PickupGate-Snapshot.log` exclusively
  locked by a sibling session's `BusterBlockEditor-Cmd` (PID 22316) — branch ops touched docs
  only (safe); C++ build deferred until the sibling run ends.
- Research (exemplars read on HEAD, per non-negotiable #1). The neighboring features I will
  mimic are **CkTimer** (quartet), **CkRenderTarget** (module topology/Settings/relay), **CkCue**
  (unreliable relay RPCs), **CkAudio** (Stats.h; Director→Track child-entity Create):
  - `CkTimer/{CkTimer.Build.cs, CkTimer_Module.h/.cpp, CkTimer_Log.h/.cpp}`
  - `CkTimer/Public/CkTimer/{CkTimer_Fragment_Data.h, CkTimer_Fragment.h, CkTimer_Processor.h}`;
    `CkTimer_Utils.h:1-150`, `CkTimer_Utils.cpp:1-140` (Add at :38-78; HAS_CAST at :128)
  - `CkRenderTarget/{CkRenderTarget.Build.cs}`, `Settings/CkRenderTarget_Settings.h/.cpp`
    (constexpr-fallback getter shape), `Net/CkRenderTargetRelay_Actor.h` (enqueue-only +
    server-stamp contract comments :16-17, :58-59), `Net/CkRenderTargetRelay_Subsystem.h`
    (2-override), subsystem .cpp:14 (`ResolveGameplayTag("ActorRelay.RenderTarget")`)
  - `CkCue/Public/CkCue/CkCueRelay_Actor.h` (Unreliable Server :24 / Multicast :62 RPCs)
  - `CkActorRelay/Public/CkActorRelay/CkActorRelay_GroupSubsystem.h` + `CkActorRelay/CLAUDE.md`
    (anti-pattern text to amend, plus bNetStartup/Iris scar at "Channel pools")
  - `CkAudio/CkAudio_Stats.h`; `CkAudio/Public/CkAudio/AudioTrack/CkAudioTrack_Utils.cpp:17-46`
    (child-entity Create + record connect + HAS_CAST)
  - `CkFoundation.uplugin` entry shape (CkRenderTarget row, :898-908)
  - Native-tag exemplar: `CkRenderTarget_Fragment_Data.cpp:5` / `.h:20`
- Inferred (unconfirmed): unreliable unicast Client RPC behavior under the fork's net stack —
  exactly what the P0 spike exists to confirm.

## Open items
| Item | Status | Next step |
|---|---|---|
| Skeleton scaffold | ✅ 9b0a55d22 | audited at gate review |
| ADR-4 doctrine amendment | ✅ 1ff7decf9 | audited at gate review |
| P0 spike + memo | ✅ memo FINAL, tests 2/2 green | audited at gate review |
| Build + AS + regression gates | ✅ all green (see 2026-08-03 entry) | — |
| Gate-review package | ✅ Gate_0_ReviewPackage.md | **top-tier audit — the only open item** |
| `[EDITOR-VERIFY]` BP surface | ⏳ human | steps in the review package |
