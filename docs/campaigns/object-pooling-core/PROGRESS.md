# Object pooling core — PROGRESS.md (living log)

## Current state  <!-- supersedes everything below; update at EVERY phase and session end -->
**As of 2026-07-11 (branch `feature/object-pooling-core`):** Phases 1-4 ✅; Phase 5 debugger module
built + linked (tab is [EDITOR-VERIFY]). Latest user-driven refinements: TryReleaseToPool made a
benign no-op for untracked objects + all 6 release-gates removed. Fresh full regression suite
RUNNING on the grace/gate binary. Then: commit everything (3 repos), then Phase 6 (docs/AS verify).
Prior refinements this session: veto removed, per-use params removed, Get_ScriptInstance not added,
subsystem methods renamed public, "vend"→"tracked/pinned".
**Baseline being diffed against:** 1048 total / 1040 pass / 8 fail (names in PHASE_2.md exit
criteria; all pre-existing BB game tests). Reproduced identically across Phase-1/2/3 binaries.
**Next action:** confirm full-suite diff (expect 1052 total = +4 pooling, 1044 pass, same 8 fail);
then commit Phase 4 (CkFoundation API-refinement + CkTests suite as coordinated commits).
**Blocked on:** nothing.
**Uncommitted:** CkFoundation working tree has Phase-3-followup API refinements (veto removal,
renames, per-use trim, GetScriptInstance removal) NOT yet committed — all since commit f4585bf3d.
CkTests branch `feature/object-pooling-autotests` has the 4 tests + subjects + generated wrapper,
uncommitted.

## Decision log
| Date | Decision | Why | Revisit when |
|---|---|---|---|
| 2026-07-11 | All 12 locked decisions — see PROMPT.md table (single source; not duplicated here) | user forks resolved via 4 questions + 2 flagged unilateral calls (enum additive, actors excluded) | per-row notes in PROMPT.md |

## Dated entries (append-only, newest first)

### 2026-07-11 — Phase 5 debugger module + TryReleaseToPool grace / gate removal (user-driven)
- User Q1: the `Get_IsPoolTrackedObject` release-gate is redundant. Made subsystem `TryReleaseToPool`
  a BENIGN no-op (returns Failed, no ensure) for untracked objects → removed ALL 6 release-gates
  (EntityScript EndPlay + CkUnrealComponent + CkAudio + CkPmg ×2 + CkUI ×2). Predicate stays public
  (GC autotest uses it), just not as a gate. `TryReleaseToPool` now safe to call unconditionally.
- User Q2: "release then destroy" clarified in every teardown comment — these are all
  DestroyOnRelease (force-new) components, so release = UNPIN (the subsystem hold exists only so the
  fragment can be weak; a registered ownerless component has no other GC root); DestroyComponent
  does the real UE teardown; unpin-before-destroy because destroy can garbage-mark the object.
- Intermediate suite (rename/veto binary + 4 pooling tests): **1052 total, 8 failed — same 8
  pre-existing BB game tests; all 4 pooling green** (suite_p4_full). No regressions from
  renames/veto.
- Phase 5 debugger: new `CkObjectPoolingDebugger` UncookedOnly module in CkGameplayDebugger
  (branch `feature/object-pooling-inspector` off 37e4066): 7 files mimicking the lean CkInputDebugger
  tab. Per-pool table + summary, world selector, refresh gate. Console `ck.ObjectPoolingDebugger`.
- Ran: build (build_p5_try1) green, 0 errors — validated grace/gate CkFoundation edits AND the new
  debugger module cross-repo (`BusterBlockEditor-CkObjectPoolingDebugger.dll` linked). Fresh full
  regression suite RUNNING (suite_p5_full) since grace/gate touches every teardown path.
- Inferred (pending): grace/gate regression diff (expect same 1052/1044/8); debugger tab live
  behavior is [EDITOR-VERIFY] (agents can't open Slate).

### 2026-07-11 — Phase 4 pooling suite GREEN + user-driven API refinements
- User directives this session (all applied): (a) drop per-use `FInstancedStruct` from the acquire
  chain — participant delegates payload-free; (b) remove the `_CanBePooled` veto entirely — no
  per-instance opt-out, "never recycle" = force-new policy; (c) don't expose `Get_ScriptInstance`;
  (d) rename subsystem `DoRequest_Acquire`→`AcquireFromPool`, `DoTryReleaseToPool`→`TryReleaseToPool`,
  both public (dropped the friend); (e) explain + eliminate "vend" → "tracked/pinned/hand out"
  repo-wide. Also: `Get_IsPoolVendedObject`→`Get_IsPoolTrackedObject`, `_VendedUnique`→`_PinnedUnique`,
  `Get_NumVendedUnique`→`Get_NumPinnedUnique`, `Get_IsVendedObject`→`Get_IsTrackedObject`.
- Added tooling surface: `UCk_Utils_Object_UE::Get_ObjectPoolStats(WCO, Class, Archetype)` (BP/AS;
  no WorldContext meta — it conflicted with the ScriptMixin=UObject arg0-receiver binding and
  produced a broken generated wrapper; diagnosed via ck-angelscript-interop skill catalog item 6).
- CkTests branch `feature/object-pooling-autotests` off ac4f00d: 4 AS autotests + subjects
  (`Script/CkObjectPooling/`). Observe via public pool-stats + direct plain-object API only.
- Ran: rebuilds (build_p4_try2/try3/try4 all green, 0 errors). Targeted pooling tests:
  `--test-pattern ObjectPooling --discover-fresh` → **4/4 Success, 0 AS errors, EXIT 0**
  (tests_pooling_v3). AS gotchas hit + fixed: full-class `UCk_Utils_Object_UE::` static spelling
  fails for UObject-mixin functions (use `utils_object::`); setter chain on an AS temporary is
  rejected (declare-then-set).
- Confirmed: zero "vend" tokens remain in the pooling code + all consumers (rg exit 1).
- Inferred (pending): full-suite regression diff RUNNING (suite_p4_full).

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
