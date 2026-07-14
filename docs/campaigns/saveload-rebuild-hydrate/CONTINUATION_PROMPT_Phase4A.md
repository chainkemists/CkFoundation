# Continuation — CkSnapshot rebuild+hydrate, Phase 4A (then 4B → 5)

**One-line:** Phase 3B is DONE + COMMITTED (v3 rebuild+hydrate load pipeline live; gate-3 Ck.Snapshot 52/43/9, all 9
fails are verified Phase-4 casualties). Phase 4A gives CkStateMachine a Save-transport hydration path (so SM state
restores) AND closes CTO note N1 (the boot-infra-vs-gameplay spawner discriminator). You are **Opus**; stay Opus for
implementation; route design forks through a **Fable** agent (`Agent`, `model:"fable"`, read-only) and VERIFY every
ruling against the cited code yourself before implementing.

## 0. READ FIRST (in order)
1. `PROGRESS.md` — §Status board (3B DONE row), **§Phase-3B DONE** (gate-3 verdict + the 9-casualty categorization +
   the M2a misdiagnosis correction), §Decisions [P3B-D6]/[P3B-M2a], and the Unattended execution protocol.
2. `PHASE_4A.md` (the phase) + `docs/specs/2026-07-10-CkSnapshot-rebuild-hydrate-design.md` §4.2 (CTO note N1 is the
   authority) + `PHASE_4B.md`/`PHASE_5.md`/`VALIDATION.md` at each later phase start.
3. `*.md` is gitignored — campaign docs are force-added (`git add -f`).

## 1. Repo state (verify at start)
- CkFoundation `feature/save-load-improvements` HEAD = the 3B docs commit (the one that added this file), on top of:
  - `78fcdaa8e` refactor(CkEcs,CkEcsExt): retire reconstitution suppression machinery
  - `36bcdec5d` feat(CkSnapshot): v3 rebuild+hydrate load pipeline
- CkTests `dev` HEAD = `ce32c65` test(CkSnapshot): v3 InstancedStruct disk smoke + M2a respawn opt-in.
- Tree CLEAN after the 3B commits. NOTHING pushed. Editor CLOSED.
- **FIRST: capture the Net baseline** (3B never ran it — it's provably inert for non-load paths, but 4A touches
  CkStateMachine replication and needs the diff base):
  `CkAuto\UnrealToolbox.exe --test --test-pattern "Net" --discover-fresh --output CkAuto\logs\p4a-net-baseline.log`.
  Expect: framework `Ck.*.Net` delta-zero + the recorded kiosk-trio env-red + the `Ck.StateMachine.Net.OwningClientAuth_
  SubSm_AuthorityGatedTask` flake (ignorable). A NEW `Ck.*.Net` red = investigate before 4A code.

## 2. Phase 4A work (PHASE_4A.md is the spec — this is the digest)
**4A.1 — SM redrive-as-hydration.** CkStateMachine already has Save-shaped payloads (`FCk_RepData_StateMachine_
WithHistory`/`_NoHistory`, `CkStateMachine_RepData.h:15,48`) + client Apply handlers that replay to the target state.
Migrate the SM registrar to `RegisterLazyTyped` + add `Produce` (emit current run-state) + flip `Transport = NetAndSave`.
**Load-path difference vs net:** on a loading AUTHORITY the Apply must drive the SM to the saved state WITHOUT
re-executing entry-side effects that spawn subordinates — that is what `FProcessor_Sm_RestoreRedrive`
(`CkStateMachine_Processor.cpp:1011-1235`, authority gate `:1134`, sub-machines `:1027`) already does. Port that replay
into the SAVE-transport Apply branch, gated by a hydration-apply scope. **Design already fixed (PHASE_4A §4A.1):** a
scoped guard object `FCk_HydrationApplyScope` the `FProcessor_Hydration_Dispatch` sets, queryable statically, WITHOUT
changing the net Apply signature — net-transport Apply must stay byte-identical (SM net suite + pins prove it). Then
DELETE `FProcessor_Sm_RestoreRedrive` + `FTag_Sm_RestoreRedriven` + the `JustRestored` gate at `:972`.
**Route to Fable:** the exact apply-scope mechanism + how the redrive replay maps into the Apply branch (verify against
`CkStateMachine_Processor.cpp:1011-1235` + `CkStateMachine_Replication.cpp:295,314` before writing).

**4A.2 — Close the N1 allowlist.** Remove `# Phase-4-pending` lines from `oracle-allowlist-p3.txt` (copy to
`oracle-allowlist-p4.txt` with none pending); point the OracleParity test at the p4 file. NOTE: p3 currently has NO
active entries (framework gate has no BB driver world). N1's real substance is the DISCRIMINATOR: a gameplay
RuntimeSpawned entity spawned under the transient by an SM task (the NON-bridged path) must respawn while true
boot-infra stays skipped. In 3B, the boot-infra skip ([P3B-D5], `CkSnapshot_Subsystem.cpp:686`) skips ALL
transient-owned non-bridged RuntimeSpawned entities. 4A must add the gameplay-vs-boot discriminator so gameplay
top-level entities respawn. **This is the N1 core — route the discriminator design to Fable** (candidates: a persisted
"gameplay-spawned" marker on the recipe vs. boot-infra recreated by GameMode; the spawner's hydrated SM state gating the
re-spawn). Do NOT "fix" an N1 duplicate by suppressing the spawner — hydration-cover the spawner's control state.

**4A.3 — New test** `Ck.Snapshot.Rebuild.SpawnerResumesPastSpawnDecision`: SM-driven script whose state-entry task
spawns a RuntimeSpawned subordinate then transitions to a holding state; save AFTER the spawn; load; assert exactly ONE
subordinate AND the spawner SM sits in the holding state (did not re-enter the spawning state). This is the N1 exit
criterion executable.

**4A.4 — Gate + commit.**
```
CkAuto\UnrealToolbox.exe --build --test --test-pattern "Ck.Snapshot" --discover-fresh --output CkAuto\logs\p4a.log
CkAuto\UnrealToolbox.exe --test --test-pattern "Ck.StateMachine" --output CkAuto\logs\p4a-sm.log
CkAuto\UnrealToolbox.exe --test --test-pattern "Net" --output CkAuto\logs\p4a-net.log
```
Expected: `Parity.StateMachine*` GREEN (10→7 casualties); OracleParity green with ZERO allowlist entries; the whole
`Ck.StateMachine` family delta-zero (long flake history — `ck-failure-archaeology` §6; re-run a lone red alone then the
group before classifying). Commits: `feat(CkStateMachine): SM redrive as Save-transport hydration; delete RestoreRedrive`;
(CkTests) `test(CkSnapshot): spawner-resume N1 exit criterion`. Stage by name, no push, no Co-Authored-By.

## 3. Locked (don't re-litigate)
- Net-path SM Apply stays byte-identical; the hydration branch is ADDITIVE, gated by the apply-scope. Do NOT weaken it.
- Do NOT chase unrelated SM flakes (archaeology §6) — classify + record; only fix if YOUR diff caused it (A/B stash).
- The 9 remaining 3B casualties are per-feature payload work: 4A closes the 2 SM parity reds; 4B closes the other 7
  (Attributes/AnimPlan empty-seed Produce; TagSet/Grid/Inventory×2/RenderTarget client-shaped Apply — for the
  client-shaped set, PHASE_4B re-authors the Apply to a hydration-scope authority write, same pattern as 4A.1's SM
  branch; do NOT invent an ad-hoc authority sync drain).
- Model A stays compiled until Phase 5.

## 4. After 4A: 4B (coverage sweep + AS smoke) then 5 (decommission Model A under `CK_WITH_FIDELITY_ORACLE` — gate, don't
delete; VALIDATION.md is the definition of done). Read each PHASE_N.md + VALIDATION.md at that phase's start.
