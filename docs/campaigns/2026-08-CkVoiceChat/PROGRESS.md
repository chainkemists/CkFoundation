# CkVoiceChat — PROGRESS.md (living log)

## Current state  <!-- supersedes everything below; update at EVERY gate and session end -->
**As of 2026-08-03 (branch `feature/voice-chat`, base d02278cdd):** Gate 0 in progress —
campaign docs landed; skeleton scaffold next.
**Baseline being diffed against:** not yet captured (pre-build; capture before the gate's test
runs).
**Next action:** scaffold `Source/CkVoiceChat/`.
**Blocked on:** nothing. NOTE: a sibling session's headless test run (`BusterBlockEditor-Cmd`
PID 22316 + UnrealToolbox) was live at session start — `--build` waits until it ends.

## Decision log
| Date | Decision | Why | Revisit when |
|---|---|---|---|
| 2026-08-02 | ADR-4 blessed (paced budgeted relay-actor streams) with clause (a)–(g); UChannel = named fallback only | CTO adjudication | P0 spike shows pathological drop/starvation, or P3 profile shows relay overhead |
| 2026-08-02 | Dep budget: minimal earned deps at P0 (`CkCore/Ecs/EcsExt/Label/Log/Record/Settings` + engine `Core/CoreUObject/Engine/GameplayTags/DeveloperSettings`); no Relationship ever; Timer only if earned; ActorRelay/SpatialQuery/ResourceLoader/Profile added at the phase whose code consumes them | Review N4 ("every dep must be earned") | Each phase's Build.cs review |
| 2026-08-03 | P0 skeleton carries NO requests/signals/Codec/Net/Playback files — deferred to the phase that implements them; VoiceListener has no Processor files until P3 | Skeleton = compiling topology, not speculative surface; empty files are dead weight | P1 (Codec), P2 (Talker requests/signals/Playback), P3 (Net, Listener processors) |

## Dated entries (append-only, newest first)

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
| Skeleton scaffold | 🟡 in progress | land, then build gate |
| ADR-4 doctrine amendment | ⏳ | after scaffold |
| P0 spike + memo | ⏳ | after amendment; CkTests branch |
| Build + AS gates | ⏳ | after sibling's editor run ends |
| Gate-review package | ⏳ | last |
