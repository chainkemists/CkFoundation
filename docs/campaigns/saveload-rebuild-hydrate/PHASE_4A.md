# PHASE 4A — StateMachine redrive-as-hydration + N1 closure (spawner control-state)

CTO note N1 (review addendum) is the mission: a RuntimeSpawned subordinate is duplicate-safe only once its
SPAWNER's SM control state resumes from hydrated state instead of re-entering a pre-spawn state after gate-open.
Read N1 in full + `FProcessor_Sm_RestoreRedrive` (`CkStateMachine_Processor.cpp:1011-1235`) + the SM rep handlers
(`CkStateMachine_Replication.cpp:295,314`) before writing anything.

## Entry criteria
- Phase 3B done; `oracle-allowlist-p3.txt` exists with its `# Phase-4-pending` lines; patterns green mod allowlist.

## Steps

### 4A.1 SM hydration payload
The SM already has Save-shaped payloads: `FCk_RepData_StateMachine_WithHistory` / `_NoHistory`
(`CkStateMachine_RepData.h:15,48`) and client Apply handlers that replay to the target state
(`Sm_DispatchWithHistory` / synthesized transition via `Sm_EnqueueOrStash`). Migrate the registrar to
`RegisterLazyTyped` + add `Produce` (emit current run-state exactly as the authority's replicate pass builds the
payload) + flip `Transport = NetAndSave`.
**The load-path difference vs net:** on a loading AUTHORITY the Apply must drive the SM to the saved state WITHOUT
re-executing entry-side effects that spawn subordinates — this is exactly what `FProcessor_Sm_RestoreRedrive`
already knows how to do (its replay path with authority gating at `:1134`, sub-machines at `:1027`). Port that
replay into the SAVE-transport Apply branch: handler checks "am I applying from hydration on authority"
(`FFragment_PendingHydration`-sourced applies pass a flag — extend the sibling dispatcher to mark the handle or
pass context via a scoped struct; pick the mechanism `FProcessor_Hydration_Dispatch` can provide WITHOUT changing
the net Apply signature: recommended = a thread-local-free scoped guard object in the dispatcher,
`FCk_HydrationApplyScope`, queryable statically — design fixed, implement as stated). Net-transport Apply behavior
must remain byte-identical (pins + SM net suite prove it).
Then DELETE `FProcessor_Sm_RestoreRedrive` + `FTag_Sm_RestoreRedriven` + the `JustRestored` gate at
`CkStateMachine_Processor.cpp:972`.

### 4A.2 Close the N1 allowlist
Remove every `# Phase-4-pending` line from `oracle-allowlist-p3.txt` (copy it to `oracle-allowlist-p4.txt`, empty
of pending lines; update the OracleParity test to use the p4 file).

### 4A.3 New test
`"Ck.Snapshot.Rebuild.SpawnerResumesPastSpawnDecision"` — fixture: SM-driven script whose state-entry task spawns a
RuntimeSpawned subordinate, then transitions to a holding state. Save AFTER the spawn. Load. Assert: exactly ONE
subordinate exists post-load AND the spawner SM sits in the holding state (did not re-enter the spawning state).
This is the N1 exit criterion in executable form.

### 4A.4 Gate + commit
```powershell
CkAuto\UnrealToolbox.exe --build --test --test-pattern "Ck.Snapshot" --discover-fresh --output CkAuto\logs\p4a.log
CkAuto\UnrealToolbox.exe --test --test-pattern "Ck.StateMachine" --output CkAuto\logs\p4a-sm.log
CkAuto\UnrealToolbox.exe --test --test-pattern "Net" --output CkAuto\logs\p4a-net.log
```
Expected: OracleParity green with ZERO allowlist entries; `Ck.Snapshot.Parity.StateMachine*` green; the whole
`Ck.StateMachine` family delta-zero (this family has a long flake history — see `ck-failure-archaeology` §6; a
single red: re-run it alone, then the group, before classifying flake-vs-regression, and record which).
Commits: `feat(CkStateMachine): SM redrive as Save-transport hydration; delete RestoreRedrive` ;
(CkTests) `test(CkSnapshot): spawner-resume N1 exit criterion`.

## Exit criteria
- `rg -l "RestoreRedrive|RestoreRedriven" Source` → 0. Oracle allowlist empty of pending lines.
- `Ck.Snapshot.Rebuild.SpawnerResumesPastSpawnDecision` green by name.

## Fences
- Do NOT weaken net-path SM Apply behavior — the hydration branch is ADDITIVE, gated by the apply-scope.
- Do NOT chase unrelated SM flakes (archaeology §6); classify + record, only fix if YOUR diff caused it (A/B stash).
