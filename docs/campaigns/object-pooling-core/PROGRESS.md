# Object pooling core — PROGRESS.md (living log)

## Current state  <!-- supersedes everything below; update at EVERY phase and session end -->
**As of 2026-07-11 (branch `feature/object-pooling-core`):** Phases 1-3 ✅ + per-use-param API trim
(user-driven). Next: Phase 4 (CkTests pooling suite) — the first runtime proof of the poolable
recycle path.
**Baseline being diffed against:** 1048 total / 1040 pass / 8 fail (names in PHASE_2.md exit
criteria; all pre-existing BB game tests). Reproduced IDENTICALLY on four binaries: Phase-1,
Phase-2, Phase-3-sweep, Phase-3+trim.
**Next action:** Phase 4 — new CkTests branch off its dev; autotests per PHASE_4.md (note: tests
must not reference per-use params — removed from the API).
**Blocked on:** nothing.

## Decision log
| Date | Decision | Why | Revisit when |
|---|---|---|---|
| 2026-07-11 | All 12 locked decisions — see PROMPT.md table (single source; not duplicated here) | user forks resolved via 4 questions + 2 flagged unilateral calls (enum additive, actors excluded) | per-row notes in PROMPT.md |

## Dated entries (append-only, newest first)

### 2026-07-11 — Phase 3 (sweep conversions) + per-use-param removal, gate-verified
- Converted all six members to subsystem-vended weak (DestroyOnRelease): CkUnrealComponent
  `_Component`, CkAudio `_AudioComponent`, CkPmg `_MeshComponent` ×2 (7 create sites across 7 TUs),
  CkUI `_WrapperWidget` + `_WidgetComponent`. Teardowns unpin immediately before DestroyComponent
  (destroy may garbage-mark → release's validity gate would fail; no GC between the calls).
- Subsystem: `DoRequest_Acquire` gained `InOuter` — DestroyOnRelease honors it (ComponentHost-actor
  outer keeps nav-octree `GetOwner()` working); Recycle pools stay world-outered.
- User correction accepted: per-use `FInstancedStruct` removed from the whole acquire chain
  (delegates now payload-free; EntityScript per-use data flows via spawn-params injection +
  Construct; synchronous callers configure directly). 11 call sites trimmed.
- Ran: sweep-only binary suite → 1048/1040/8 identical; rebuild with trim (green, 0 errors);
  final binary suite → 1048/1040/8 IDENTICAL names. Logs: scratchpad suite_p3_tests.log /
  suite_p3b_tests.log (session-local).
- Confirmed: only intentional TStrongObjectPtr keeps remain in the 4 modules
  (`_ContentWidgetHardRef`, PMG `BundledFont` static, `_FontOverride`).
- Accepted change: PMG components no longer RF_Transient (runtime-world objects, never
  level-serialized).

### 2026-07-11 — Phase 2 implemented and gate-verified (same session as Phase 1)
- `CkEntityScript.h`: enum + `InstancedPerEntity_Poolable`; `_PoolParams` EditDefaultsOnly property
  (EditCondition-gated, hidden otherwise) + getter.
- Spawn switch (`_Processor.cpp`): both instanced policies vend via pooled
  `Request_CreateNewObject` — Poolable uses CDO `_PoolParams` forced to Recycle; plain uses
  DestroyOnRelease; spawn params double as the participant per-use payload.
- `_Fragment.h/.cpp`: `_Script` → `TWeakObjectPtr`; `_SnapshotLoadPin` strong ptr covers the
  SerializeSnapshot mint (no world reachable there — save/load campaign owns removal); EndPlay
  processor now `TReadWrite` + fragment friend, releases via new
  `UCk_Utils_Object_UE::Get_IsPoolVendedObject` gate + `TryReleaseToPool`, then clears the weak ptr.
- Ran: baseline suite (Phase-1 binary) → 1048/1040/8; rebuild (2 iterations — the one error was the
  ck_exp access-intent static_assert, fixed with `ck::TReadWrite<>`); suite on Phase-2 binary →
  1048/1040/8 with IDENTICAL failing names. No regressions.
- Confirmed: all ~16 external `Get_Script()` deref sites are weak-compatible (`.Get()`/`->`/
  `ck::IsValid`) — audited by grep + proven by the green build.
- Inferred (unconfirmed until Phase 5): poolable recycle semantics at runtime (no asset uses the
  policy yet — the suite exercises only the force-new path).
- Known semantic change accepted: after EndPlay, the fragment's `_Script` is cleared for vended
  scripts — a late `Get_ScriptClass` on a dead entity now ensure-fails instead of returning the
  dying script's class.

### 2026-07-11 — Phase 1 implemented and gate-verified
- Wrote `Source/CkCore/Public/CkCore/ObjectPooling/`: `CkObjectPooling_Params.h` (3 enums incl.
  `ECk_ObjectPooling_RecyclePolicy` — DestroyOnRelease IS the force-new pinned vend; PoolParams;
  PoolStats), `CkObjectPoolingParticipant.h` (+ `_Utils.h/.cpp` — binds are idempotent per
  (object, function) via non-reflected bind-key ledgers, surviving the recycle reset that skips the
  participant property), `CkObjectPooling_Subsystem.h/.cpp` (pools keyed (class, archetype);
  archetype pinned; `_VendedUnique` pinned set; reset-to-archetype sweep skipping participant
  properties; lazy null-slot sweep = steal semantics; amortized prewarm tick; actor classes
  rejected), `CkObjectPooling_Settings.h/.cpp` (per-class project-settings OVERRIDE at pool
  creation).
- Hoisted into `CkObject_Utils.h/.cpp`: template `Request_CreateNewObject(Outer, Class, Archetype,
  PoolParams, PerUse, InitFunc)`, BP `Request_CreateNewObject_Pooled`, BP `TryReleaseToPool`;
  no-world Outer = ensure + plain caller-owned create fallback (documented recovery).
- Ran: `UnrealToolbox --build --target Editor --config Development` → 436 actions,
  `Result: Succeeded`; log confirms all 4 new .cpps compiled (unity-excluded), 0 errors.
  Log: scratchpad `build_p1_try1.log` (session-local).
- Confirmed: zero CkEcs references in the new code (`rg --no-ignore` exit 1).
- Inferred (unconfirmed until Phase 5 autotests): recycle reset + delegate preservation behave as
  specified at runtime — compile-only evidence so far.
- Design detail decided in-code: participant bind idempotency implemented as TSet bind-key ledgers
  inside the struct (survives recycle since the property is skipped); Unbind/RemoveAll clears ledger
  entries for the object.

### 2026-07-11 — campaign start: research, forks resolved, docs authored
- Ran: `git diff --stat dev...origin/feature/pool-module` → 32 files / +4353 (port source mapped).
- Confirmed: dev contains NO CkPool (branch-only) — campaign is a port, not a refactor.
- Confirmed: `Request_CreateNewObject` family at `CkCore/Public/CkCore/Object/CkObject_Utils.h:141-181`;
  property-copy machinery already present (`Request_CopyAllProperties` :237, `Request_ResetAllPropertiesToDefault` :245).
- Confirmed: EntityScript surfaces — instancing enum `CkEntityScript.h:35`, `_Script` strong pin
  `CkEntityScript_Fragment.h:70`, spawn switch `_Processor.cpp:92-122` (calls `Request_CreateNewObject`
  at :113 with archetype), EndPlay `_Processor.cpp:469-484`, snapshot re-mint `_Fragment.cpp:49`.
- Confirmed (subagent sweep, spot-checkable): 17 `TStrongObjectPtr` members across 13 structs.
  Conversion candidates: `CkUnrealComponent_Fragment.h:52`, `CkPmg_Fragment.h:55,:247`,
  `CkAudioTrack_Fragment.h:41`, `CkWorldSpaceWidget_Fragment.h:55,:57`. Must-stay-strong:
  `CkVfxCue_Fragment.h:39` (Niagara factory), `CkRenderTarget_Fragment.h:77,:471,:588` (engine
  factory), `CkPmg_Fragment_TextShapes.h:25`, `CkIskmProxy_Fragment.h:192`,
  `CkWorldSpaceWidget_Fragment.h:54` (asset/archetype pins), `CkReplicatedObjects_Fragment_Params.h:137`
  (actor-subobject entangled), `CkDynamic_ScriptQueryProcessor.h:75` (processor member, not fragment),
  `CkEcsWorld_Subsystem.h:192` (already subsystem-owned).
  `_Script` weak-conversion deref sites: Processor .cpp :275,295,328,352,361,363,374,409,411,436,455,476;
  Utils .cpp :83-87,115; Fragment .cpp :96. Corroboration: `FRequest_EntityScript_Replicate._Script`
  is ALREADY weak (`_Fragment.h:101`).
- Confirmed: CkTests companion `origin/feature/pool-receiver-autotests` = 9 autotests (mostly
  EntityPool-shaped → die with decision 2; receiver round-trip/veto subjects reusable).
- User resolved 4 forks (recorded in PROMPT.md decisions 1-4). Note: decision 3 (replicated
  poolable ALLOWED) overrode the reject-in-v1 recommendation — Phase 4 item 6 is the safety net.
- Created branch `feature/object-pooling-core` off origin/dev b3894d535. Worktree left
  `feature/save-load-improvements` (clean; switch back anytime — sibling save/load campaign
  touches `CkEntityScript_Fragment` snapshot path, ordering risk noted in PROMPT reading list).
- Inferred (unconfirmed): `_WrapperWidget`'s `AddToViewport` may hold a secondary viewport ref —
  verify at Phase 3 item 5 before relying on unpin order.

## Open items
| Item | Status | Next step |
|---|---|---|
| Phase 4 — CkTests pooling suite (round-trip, delegates, GC, replicated-poolable) | pending | new CkTests branch; first runtime proof of the recycle path |
| Phase 5 — CkGameplayDebugger pools inspector | pending | after Phase 4 |
| Phase 6 — docs + AS verification | pending | after Phases 4-5 |
| `_SnapshotLoadPin` removal | owned by save/load campaign | coordinate at merge of feature/save-load-improvements |
