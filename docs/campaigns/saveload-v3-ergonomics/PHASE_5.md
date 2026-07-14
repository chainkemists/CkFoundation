# PHASE 5 — Relocate persistence machinery to `CkEcs/Persistence/` (header split, build 5)

**Adam-approved 2026-07-14 as the FINAL phase** — deliberately last so it sits at the tip of the commit
stack (cheap to drop in review) and so Phase 2's type renames + entry points already exist (this phase is
file/include mechanics ONLY). **Purely mechanical — zero behavior change, zero type renames.** If any step
forces a semantic choice → STOP + Blocker.

This is a **header SPLIT, not a file move**: the current header mixes transport-neutral persistence
machinery with net-only wire plumbing. The neutral half moves; the wire half stays.

## Entry criteria

Phases 1–4 committed (or 1–3 if Adam cut Phase 4 — this phase does not depend on Phase 4); trees clean;
gates at the PROGRESS-recorded baseline. Phase 2's renames are in (registry is
`FCk_PersistenceHandlerRegistry`, slots `NetApply`/`NetRemove`/`HydrationApply`/`Produce`, named
`Register_*` forms in use).

## Step 1 — Create the new files under `Source/CkEcs/Public/CkEcs/Persistence/`

Symbol manifest (carve OUT of `Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.{h,inl.h,cpp}`
and `…_Processor.{h,cpp}` — move code verbatim, update only includes and file-top comments):

| New file | Symbols moved into it |
|---|---|
| `CkPersistenceHandlerRegistry.h` | `ECk_Persistence_ApplyResult`; `FCk_PersistenceHandlerRegistry` + nested `FHandler` + `FTypeResolver` + the `FApplyFn`/`FRemoveFn`/`FProduceFn` aliases |
| `CkPersistenceHandlerRegistry.inl.h` | `RegisterLazyTyped<T>` body + the four `Register_*` bodies |
| `CkPersistenceHandlerRegistry.cpp` | registry static storage + `Register`/`RegisterLazy`/`ResolvePending`/`RegisterFallback`/`Get_SaveHandlerTypes`/`Find`/`Resolve` bodies (incl. the registration-time ensure) |
| `CkPersistenceHydration.h` | `ck::FFragment_PendingHydration`; `ck::FTag_Hydration_PendingApply` |
| `CkPersistenceHydration_Processor.h` | `ck::FProcessor_Hydration_Dispatch` declaration; `ck::persistence_apply::ApplyOne` + `EApplyOutcome` |
| `CkPersistenceHydration_Processor.cpp` | their bodies + the `CK_REGISTER_PROCESSOR(ck::FProcessor_Hydration_Dispatch)` line |

## Step 2 — What STAYS in `Net/ReplicatedFragmentContainer/` (wire-only)

- `CkReplicatedFragmentContainer.h`: `FCk_ReplicatedFragmentEntry`, `FCk_ReplicatedFragmentArray`,
  `ck::FTag_RepFragments_PendingApply`, `ck::TFragment_ContainerEntryRef<T>`. It now `#include`s
  `CkEcs/Persistence/CkPersistenceHandlerRegistry.h` (its FastArray callbacks call `Resolve`).
- `CkReplicatedFragmentContainer.cpp`: the FastArray replication callbacks (Resolve checks at the lines
  formerly 162/192/218).
- `CkReplicatedFragmentContainer_Processor.{h,cpp}`: `FProcessor_ReplicatedFragments_Dispatch` ONLY (its
  `CK_REGISTER_PROCESSOR` stays here).
- Delete the old `.inl.h` entirely (its whole content moved). Add a one-line file-top comment in the
  container header noting the split and pointing at `CkEcs/Persistence/`.

Dependency direction: **Net includes Persistence, never the reverse** — with ONE known seam:
`FProcessor_Hydration_Dispatch` declares `RunAfter = TDepList<FProcessor_ReplicatedFragments_Dispatch>`.
Try a forward declaration first (`namespace ck { class FProcessor_ReplicatedFragments_Dispatch; }` in
`CkPersistenceHydration_Processor.h`); if `TDepList`/the scheduler requires the complete type, include the
Net processor header there and note it in the file-top comment ("scheduling-order dependency only"). Both
outcomes are acceptable — record which in PROGRESS.md. Anything beyond that needing a Persistence→Net
include → STOP + Blocker.

## Step 3 — Update the includers (~47 files, 56 lines; enumerate fresh)

```powershell
rg --no-ignore -n "ReplicatedFragmentContainer" Source/ Plugins/CkTests/Source 2>$null
```

Mapping rule per includer (mechanical — decide by which symbols the file actually uses):
- **The 24 registrar files** (list in PROMPT.md inventory) → swap the container `.h` + `.inl.h` pair for
  `CkEcs/Persistence/CkPersistenceHandlerRegistry.h` + `.inl.h`. Save-only registrars (Timer, EntityTag,
  Dynamic SaveData, EntityScript SaveFields, the two Refills) end with NO `Net/` include — that is the
  payoff; verify it explicitly on those six.
- **`CkSnapshot_CaptureV3.cpp`** → Persistence registry header only.
- **`CkSnapshot_Subsystem.cpp`** → `CkPersistenceHydration.h` (PendingHydration enqueue at ~:918) +
  Persistence registry if it names the registry (it doesn't today — verify).
- **`CkNet_Utils.h`** → keeps the Net container include (entry/array/driver types for
  `TryUpdateContainerFragment`) AND adds the Persistence registry include (for Phase 3's `TryProduce`).
- **Driver files** (`CkEntityReplicationDriver_Fragment.{h,cpp}`, `_Utils.cpp`) → keep Net container;
  `_Fragment.cpp` adds Persistence registry (the `Resolve` check at ~:104).
- **Attribute shared headers** (`CkAttribute_RestorePersistence.h`, `CkAttribute_RefillPersistence.h`) →
  swap to Persistence registry includes.
- Everything else the grep finds → same rule: registry/hydration symbols → Persistence path; entry/array/
  container-ref symbols → Net path; both → both.

## Step 4 — Build + gate (the phase's ONLY build)

1. Build → exit 0 (wide recompile — the registry header is in 47 files' include chains).
2. `--test --test-pattern "Ck.Snapshot"` and `"Ck.Net"` → **delta-zero vs the recorded baseline, by
   name**. This phase cannot change behavior; any red is yours — fix the include/move mechanics or revert.
   (No `--discover-fresh` — no tests added/removed.)
3. Grep gates:
   - `rg --no-ignore -n "FCk_PersistenceHandlerRegistry|FFragment_PendingHydration" Source/CkEcs/Public/CkEcs/Net/` →
     only the container header's single registry include + callback call sites (no DECLARATIONS remain under Net/).
   - `rg --no-ignore -n "ReplicatedFragmentContainer.inl" Source/ Plugins/CkTests/Source` → **0**.
   - The six save-only registrar files: `rg --no-ignore -n "CkEcs/Net/" <each>` → **0**.

## Commit

`refactor(CkEcs): relocate persistence machinery to CkEcs/Persistence/ — header split; registry/hydration out of Net/, FastArray wire plumbing stays (tip-of-stack, droppable in review)`

## Fences

- ZERO type/symbol renames in this phase (Phase 2 owned those). Move verbatim.
- Do NOT move `FCk_ReplicatedFragmentEntry`/`Array`, `FTag_RepFragments_PendingApply`,
  `TFragment_ContainerEntryRef`, or `FProcessor_ReplicatedFragments_Dispatch` — they are wire plumbing.
- Do NOT create a forwarding/alias header in the old location — one name per thing; the includer sweep IS
  the migration.
- Persistence → Net includes: only the RunAfter seam from Step 2, nothing else.
- No anonymous namespaces in the new .cpps (unity builds) — the moved code already complies; keep it so.
- If UHT complains about `FFragment_PendingHydration`'s generated-body plumbing after the move
  (`CK_GENERATED_BODY` fragments are not USTRUCTs — no `.generated.h` involved; planner expects NO UHT
  interaction), anything unexpected → STOP + Blocker rather than adding reflection includes.
