# Gate 0 — Module skeleton, ADR-4 doctrine amendment, Iris transport spike

> **Status:** 🟡 In progress (2026-08-02)
> **Depends on:** CTO green-light (docs/reviews/2026-08-02-CkVoiceChat-CTO-review.md) ✅
> **Estimate:** 1 session — re-date at entry; record actual at exit

## Goal

After this gate: `Source/CkVoiceChat/` exists as a compiling Tier-4 skeleton (three feature
quartets + Settings/Log/Stats/module files, uplugin entry, native tags); the ADR-4 doctrine
clause is live in `Source/CkActorRelay/CLAUDE.md`; and a committed spike memo answers the three
unreliable-unicast-RPC questions under the fork's net stack — or reports pathological
drop/starvation, which STOPS the campaign and re-opens ADR-4 with the UChannel fallback.

## Entry criteria (pre-flight)

- [x] CTO review verdict read; N-notes folded into PROMPT.md (this commit's sibling).
- [x] Branch `feature/voice-chat` cut from `origin/dev` (d02278cdd); review commit cherry-picked
      (7a66ff11e); spec force-added past `.gitignore` `*.md` (21bf89232).
- [x] Exemplars read on current HEAD: CkTimer quartet + module files, CkRenderTarget
      Build.cs/Settings/Relay actor+subsystem, CkCueRelay_Actor.h, CkActorRelay_GroupSubsystem.h
      + CLAUDE.md, CkAudio_Stats.h + CkAudioTrack_Utils.cpp (child-entity Create), uplugin entry
      shape. (List with paths: PROGRESS.md 2026-08-02 entry.)
- [ ] Test baseline captured before the gate's build/test runs (counts + failing names).

## Work items

1. **Skeleton `Source/CkVoiceChat/`** — mimicry: Build.cs (CkModuleRules, CkRenderTarget shape,
   **minimal earned deps only** per N4), `CkVoiceChat_Module.{h,cpp}` (CkTimer shape),
   `CkVoiceChat_Log.{h,cpp}` (namespace `ck::voice_chat`), `CkVoiceChat_Stats.h` (CkAudio shape),
   `Settings/CkVoiceChat_Settings.{h,cpp}` (CkRenderTarget shape + constexpr fallbacks),
   uplugin entry, `Source/CLAUDE.md` tier row, stub module `Claude.md`.
2. **Three feature quartets, minimal-real** (CkTimer quartet shapes):
   - VoiceTalker: transmit-mode enum, ParamsData, typesafe handle, tags
     (NeedsSetup/IsTransmitting/IsSpeaking), Current{Seq, AmplitudeQ8}, Utils Add/Has/Cast +
     getters, Setup processor (consumes NeedsSetup).
   - VoiceChannel: spatialization-policy enum, ParamsData (channel tag/policy/range/attenuation/
     effect-chain/priority/auto-join), handle, TRANSIENT record-of-channels on the host,
     child-entity Add (CkAudioTrack Create shape), TryGet by tag, Setup processor.
   - VoiceListener: handle, Current{muted set, volume map}, Utils Add/Has/Cast + getters.
     No processor files until a processor exists (P3) — deviation from spec §7.1 layout, recorded.
   - Deferred to their phases: requests + completion contract (P2/P3), signals (P2), Codec/ (P1),
     Net/ (P3), Playback/ (P2), CVars (P2 consumers). NO speculative fields.
3. **ADR-4 doctrine amendment** — reword `Source/CkActorRelay/CLAUDE.md` Anti-patterns:
   Broadcast/Bind event channels keep "events only" verbatim; relay ACTORS as per-player RPC
   endpoints may carry paced budgeted streams under clause (a)–(g); adopters named.
4. **P0 spike** (throwaway code, CkTests `feature/voice-chat` branch; memo is the artifact):
   sustained-rate Unreliable unicast Client-direction RPCs on a relay actor under the fork's
   net stack. Must answer: (a) delivery under packet-fill pressure — does the stack
   coalesce/starve unreliable attachments; (b) unresolved target channel actor on client —
   confirm silent vanish, no ensure/log storm; (c) per-RPC overhead at ~25 Hz — should the
   bundle grow past 3 frames. Memo: `SpikeMemo_P0_UnreliableUnicastClientRPC.md` (this folder).

## Expected observations at the gate — and what to do on each branch

| I will run | I expect to observe | If instead I see | Prewritten response |
|---|---|---|---|
| Toolbox `--build` (editor closed) | Exit 0, CkVoiceChat compiles | Compile errors | Fix; they're mine by construction (new module) |
| Headless AS boot + fresh-log grep | Zero `Angelscript: Error` naming CkVoiceChat files | AS binding errors | `ck-angelscript-interop` skill; fix generator-facing surface |
| Spike: sustained 25 Hz unreliable unicast for ~10 s under load | Delivery ratio high on LAN-ish loopback; drops are silent; no log storm on unresolved channel | Pathological drop/starvation (e.g. unreliable RPCs starved to near-zero under packet fill) | **STOP the campaign** — write memo, report; ADR-4 re-opens with UChannel fallback. Do NOT proceed to P1 |
| Spike: client RPC to not-yet-resolved channel actor | Silent vanish (no ensure, no per-packet error spam) | Ensure/log storm | Record exact log class in memo; routing design must gate sends on client-ack/resolve — flag as P3 design amendment |
| Targeted test suite after skeleton lands | Delta-zero vs captured baseline | New failures | Isolate: A/B stash skeleton; own-change vs pre-existing before touching anything |

## Exit criteria — ALL land with the gate-closing commit

- [ ] Every expected observation confirmed; evidence (commands, exit codes, log lines) in
      PROGRESS.md.
- [ ] `ck-change-control` done-checklist run (class 2 — additive module; doctrine edit is
      class 1 doc change verified against the review text).
- [ ] `[EDITOR-VERIFY]` items listed with exact steps (BP node checks for the new Utils surface).
- [ ] Spike memo committed; verdict quoted in PROGRESS.md.
- [ ] This file's Status header flipped; PROGRESS.md dated entry appended.
- [ ] Gate-review package written for the top-tier audit; campaign PAUSES until audited.
