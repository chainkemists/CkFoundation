# Maintainer items from the Save/Load core-tenets campaign

> Written 2026-08-21 at campaign close, by the campaign orchestrator, at the user's ruling
> (`POST-D4`). These are the five findings the campaign surfaced that are **framework questions, not
> game-side defects** — each was deliberately left open because answering it is the maintainer's
> call, not the campaign's. Nothing here blocks the BusterBlock delivery; all five were reachable
> from evidence the campaign already had, and each carries its repro or its file:line.
>
> Campaign record (gitignored, BusterBlock-side): `docs/campaigns/saveload-core-tenets/PROGRESS.md`.
> Delivery review package: `docs/superpowers/reviews/2026-08-21-saveload-core-tenets-delivery-review-package.md`.

Ownership at a glance:

| # | Item | Owner | Kind |
|---|---|---|---|
| 1 | Setup runs pre-hydration on the ESCALATED load path | CkSnapshot | **Correctness hole** |
| 2 | `Get_PlacementForOccupant` back-ref dead post-load | CkGrid (2dGridOccupancy) | Correctness / documented limit |
| 3 | Test-sweep universe shrinks silently | **UnrealToolbox (CkAuto)** — not CkF | Tooling / evidence integrity |
| 4 | AngelScript has no save-exclusion API | CkSnapshot + AS bindings | Missing API |
| 5 | `_MarkedDirtyBy` gates on registry-wide presence | CkEcs scheduler | Design question |
| 6 | `*Snapshot*` serial-lane default costs 14 extra editor boots | **UnrealToolbox (CkAuto)** — not CkF | Tooling / cost |
| 7 | Doctrine names the wrong detector for durable-handle-to-unpersisted | CkSnapshot docs | Doc accuracy |

---

## 1. GW-49 — Setup observes construct defaults on the ESCALATED load path

**This is the one with a correctness consequence.** The campaign's C2 contract says no processor
observes a half-hydrated entity: the order is construct → hydrate(Durable) → setup/live. The
quarantine that enforces it is stamped when an entity's hydration **begins**.

That is sufficient on the quiesced path and insufficient on the escalated one. Every BusterBlock
store load escalates — the kernel logs

```
rebuild kernel quiesced with [13]/[40] mapped — escalating
```

— and Full-scope ticks under `Escalated` run Setup on entities whose payloads are **mapped but not
yet enqueued**. Those entities have not begun hydrating, so they carry no quarantine stamp, so
Setup runs and reads construct defaults. That is precisely the failure the contract exists to
prevent, on the path production actually takes.

The ordering test does not catch it because it never escalates:
`Ck.Snapshot.Ordering.SetupRunsOncePostHydration` runs the quiesced path only.

**Proposed fix.** Stamp the quarantine on every **MAPPED-but-unhydrated** entity before the first
Full-scope tick, not at hydration start. Add a forced-escalation variant of the ordering test so
the escalated path is covered by construction.

**Acceptance.** The forced-escalation variant is green, and BusterBlock loads show zero
Setup-before-hydration.

**Repro.** Recoverable with `git -C Plugins/CkTests revert baf3bbb4` — that commit neutralised the
test that exposed it; reverting it puts the discriminating case back.

**Provenance.** Batch 3 finding, ruled `G2-D32`, deferred at `G2-D42a`. A fork was recorded rather
than resolved because the fix changes when the quarantine is stamped for every load, and the
campaign had no mandate to re-time the framework's load kernel.

---

## 2. GW-59 — `Get_PlacementForOccupant` is dead post-load when the occupant never remaps

`Source/CkGrid/Public/CkGrid/2dGridSystem/Occupancy/Ck2dGridOccupancy_Utils.cpp:60-72` writes the occupant→placement back-reference **only when the
occupant handle is valid**. The load path passes possibly-invalid occupants by design, so a
placement restored while its occupant has not yet remapped never gets its back-ref, and
`Get_PlacementForOccupant` returns nothing for the rest of the session. The placement itself is
fine; only the reverse lookup is dead.

**Two acceptable outcomes**, maintainer's choice:
- fix at occupant remap/adopt — write the back-ref when the occupant becomes valid, not only at
  placement time; or
- record it as a documented limitation, so callers know the reverse lookup is not load-safe.

**Acceptance.** Either the chain is proven live post-load (a parity transform assert), or the limit
is written down where a caller will find it.

**Provenance.** Review pass 2 `[M-8]`, `G2-D42a`.

---

## 3. GW-62 — a test sweep's universe can shrink silently, making Totals non-comparable

**Owner note: this is an UnrealToolbox item (CkAuto), included here only so the five live in one
place.** It is listed with the framework items because its consequence is framework-wide: it
degrades the evidence every other item is argued from.

Two distinct manifestations, same class:

1. **Serial-lane abort.** `no progress across 2 consecutive spawns — aborting with N tests never
   run` cost 12 `Ck.Snapshot.Parity.*` tests from `Final-CkSweep.log`. All 12 were green elsewhere
   on the same artifact — so the sweep reported a smaller, quieter universe rather than a failure.
2. **Selection collapse** (`G2-D49`). A net-pinned batch selection collapsed 21 batches → 1, taking
   the run list from 2551 → 1680. **Discovery was identical in both runs (4595/4596)** — so this is
   a *selection* problem, not a scheduler one.

The common root: **the toolbox prints neither the effective pattern nor a selected-of-discovered
count**, so a run that quietly tests a third of what the previous run tested looks exactly like a
run that tested everything. Test records from different runs are not comparable, and nothing says so.

This campaign hit the consequence directly and had to declare one of its own records *composite*
(BB.* from one run, Ck.* from another) to stay honest.

**Proposed fix**, cheapest first:
- print `selected N of M discovered` and the effective pattern in the summary line — this alone
  makes the failure visible;
- fail loudly (non-zero, named in the summary) when a lane aborts with tests never run, instead of
  shrinking `Total`;
- optionally, retry the aborted batch.

**Acceptance.** Sweep totals are stable across runs, or the "compare Total across records" check is
documented as a required step.

**Provenance.** B6 records `G2-D47`, amended `G2-D49`, characterised in the Batch 7 return
(quoted in `G2-D50`).

---

## 4. GW-64 — AngelScript cannot declare a fragment or tag save-transient

`utils_snapshot.as` exposes no way to mark an AS-added tag or fragment as **excluded from capture**.
C++ has the notion; AngelScript does not. The consequence is that probe and infrastructure tags
living on a persisted subtree are captured, and every save emits a capture-AUDIT warning for them.

**Nine distinct offenders, not four** — measured 2026-08-21 from a full `--test-pattern Snapshot`
run (34 capture-AUDIT lines):

| offender label | occurrences |
|---|---|
| `Npc.Probe.CollisionPill` | 6 |
| `Timer: No Name Specified` | 5 |
| `Npc.Tourist.LifetimeTimer` | 4 |
| `Npc.Tourist.FlyerRecipient` | 4 |
| `NO NAME` | 4 |
| `Npc.Customer.FlyerRecipient` | 2 |
| `Default__Bb_ItemOverflowDriver_EntityScript` | 2 |
| `Default__Bb_EventFeed_EntityScript` | 2 |
| `Default__Bb_AutoSaveDriver_EntityScript` | 2 |

Producers: `Ck_SaveData_EntityTags` (16) · `Ck_SaveData_Timer` (10) · `Ck_SaveData_DynamicFragments` (4)
· `Ck_RepData_Velocity` (4). The original list named only the last three plus one unlabeled Timer; **the
four `Npc.*` construct-spawned children are new**, and each drops a payload on every save. All nine are
the same shape — infrastructure riding on a persisted entity — and one API silences the family. The campaign accepted the 3→4 delta as informational (`G2-D50`) rather than
labelling them individually, because labelling would have contradicted the manifest `Reset` decision.

**Acceptance.** AngelScript can declare an exclusion, or the limitation is documented so authors
stop reading these warnings as defects.

---

## 5. GW-73 — is registry-wide presence the intended granularity for `_MarkedDirtyBy`?

**A question, not a defect.** Both implementations agree, so this is about intent rather than a
divergence:

| Path | Site |
|---|---|
| C++ | `CkProcessorTraits.inl.h:223-233` |
| AngelScript | `CkDynamic_ScriptProcessor_Host.cpp:109-130` |

Both install an `_IsDirtyChecker` with the same semantics — C++ calls
`InRegistry.Has_AnyLiveEntityWith<DirtyFragment>()`, AngelScript calls
`Has_AnyLiveEntityWith_Fragment(TransientHandle, Struct)` against the same registry pool. Either
way the dirty gate asks whether the marker exists **anywhere in the registry**, not whether it
exists on anything this processor would visit.

The campaign's docs dry-run flagged this as a falsified claim; investigation showed the claim was
merely *imprecise*. The gate is real, but for a common marker in a populated world, registry-wide
presence is trivially true, so the gate is open essentially always. The documentation now states the
measured mechanism honestly (`528c6e26f`) — no doc fix is owed.

**The question:** is registry-wide the intended granularity, or should processors keyed on common
markers get a truer gate (per-visited-set, or per-archetype)? If registry-wide is intended, this
item closes with a one-line confirmation.

**Provenance.** Dry-run Q1, ruled `G4-D3`; mechanism grounded by the drafter against the real hunk.

---

## 6. Serial-lane pinning is by NAME, and it costs 14 editor boots per save/load run

**Owner: UnrealToolbox (CkAuto).** Same family as item 3 — name-matching that silently costs time.

`SerialLanePatterns{"Net", "*Snapshot*"}` (`include/utb/Settings.hpp:80`) pins any test whose **name**
contains "Snapshot" to the serial chain, where it is pre-batched at `kMaxNetTestsPerBatch = 12`
(`src/TestRunner.cpp:451`). That batching exists for a real reason — net tests have a measured hang
ceiling of ~18 and each wedge costs the 180 s idle watchdog — but it is applied by name, not by
behaviour.

Measured on BusterBlock, 2026-08-21, same artifact, `--test-pattern Snapshot` (177 tests):

| invocation | editor boots | wall | verdict |
|---|---|---|---|
| default (pinned) | **15** | 29m 47s | 177/176/1 |
| `--serial-lane Net --parallel 1` | **1** | **16m 22s** | 177/176/1 (identical) |

One spawn, zero respawns — **the hang ceiling never bound for this set.** Same total, same pass count,
same single failing name. The 15 boots were pure cost: 14 wasted boots and ~13.5 min per run.

Contributing detail: **96 of the 177 tests never start PIE at all** (classified by log-region
attribution on `LogPlayLevel`/`StartPIE`/`PIEReady`/`CreatePIEGameInstance`), yet all 177 inherit a
containment rule measured for multi-PIE net tests. `GitLink.LfsClientLifetime.SnapshotKeepsObjectAlive`
— a C++ smart-pointer lifetime test — is pinned to the multi-PIE serial chain because its name contains
"Snapshot".

**Proposed fix.** Pin by **capability** (does the test start PIE / request clients) rather than by name.
That makes it automatically correct for every project instead of each one hand-tuning
`tests.serialLanePatterns`.

**Caveat worth carrying:** unpinning ALONE is worse, not better — without `--parallel 1` the scheduler
shards into N lanes and each lane boots its own editor (measured: 8 boots / 5m 34s for a 41-test subset
that costs 1 boot / 1m 04s at `--parallel 1`). Minimum-boots and minimum-wall-clock are different
objectives.

---

## 7. The doctrine names the wrong detector for durable-handle-to-unpersisted-target

Small, but it sent a campaign down a wrong path for an afternoon.

`CkSnapshot/Claude.md` (and the root `CLAUDE.md` section derived from it) says the C1 structural
rule's prohibited shape — a `Durable` fragment holding a handle to a non-persisted target — is caught
by *"the capture-time AUDIT ... it names the offender on every save"*.

It is not. The `v3 capture AUDIT:` lines fire only when a **skipped entity carries a hydration
payload**; a payload-free `Probe`/`Interactable`/`Timer` handle audits nothing. Verified: a full
Snapshot run emitted 34 AUDIT lines and none named the offending fragments.

The **actual** detector is a different, better line, and it is field-path precise:

```
v3 capture: durable payload [Ck_SaveData_DynamicFragments] on entity [..] holds a handle at
[_Fragments[1]<Bb_Fragment_SideHustleLeveling_State>.Buckets[1].Instances[0]] to entity [..], which
this save does NOT persist. It will load back as a tombstone — the feature comes back structurally
complete and functionally dead. Either persist the target, or make ...
```

**Ask:** correct the doc to name this line rather than the AUDIT. The detector itself needs no change —
it is doing exactly the right thing, and it caught two live instances (a `SideHustleLeveling_State`
handle mirror, and `2dGridSystem` at `Placements[0]._Occupant` — the same chain as item 2).
