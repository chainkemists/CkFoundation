# PHASE 5 — Decommission Model A (gate-not-delete), docs, final acceptance

> **SUPERSEDED — 2026-07-13.** This doc encodes the retired "gate-not-delete" strategy
> (`#if CK_WITH_FIDELITY_ORACLE`). Adam's close-out directive replaced it with FULL DELETION of Model A;
> the locked design is **FINALIZE.md** (obj-2 + the "Fable F3 execution ruling"), executed as F3
> clusters 2–4 (CkF `1226006d8`, `85b5d7319`, `1ddd20da6`). `CK_WITH_FIDELITY_ORACLE`,
> `CK_REGISTER_SNAPSHOTABLE`, the fragment registry, archives, TagRegistry/TagDriver, the fidelity oracle,
> and both Oracle.* tests NO LONGER EXIST (0 references in Source). The Track-B clusters 1a–6 logged in
> PROGRESS.md §"Phase-5 progress" were executed against this doc pre-supersession and remain historically
> accurate. Nothing below is executable; retained for history. Acceptance protocol: the amended VALIDATION.md.

Run VALIDATION.md's full protocol at the end of this phase — it is the campaign's definition of done.

## Entry criteria
- Phases 0–4B done per PROGRESS; OracleParity green with only declared-transient annotations; all patterns green.

## Steps

### 5.1 Move Model-A capture machinery under the oracle gate
- Wrap `CK_REGISTER_SNAPSHOTABLE`'s emitted registrar body in `#if CK_WITH_FIDELITY_ORACLE` INSIDE the macro
  definition (`CkSnapshot_FragmentRegistry.h:138-152`) — one edit, all ~119 call sites become test-only with zero
  per-site churn. The `static_assert` stays UNGATED (serialize-path validation still catches authoring errors in
  all configs).
- Model-A `Run_Capture`/`Run_Restore_Registry`/`Run_Restore` + Archive Writer/Reader + TagRegistry/TagDriver:
  compile under the same gate (`#if` at file scope in their .cpp/.h bodies, stubs elsewhere ONLY if a shipping
  reference survives — expected: none after 5.2).
- The registry-level CkTests (`Core.RoundTrip`, `DynamicFragment.*`, `Audit.*`, `LifecycleStrip` rewrite,
  RoundTrip-per-feature) keep running — they exercise oracle-gated code and remain the deep-diff regression net.

### 5.2 Delete the dead shipping-path machinery
In this order, compiling between clusters:
1. `UCk_Snapshot_SaveGame::_SnapshotBytes` (Model-A section) + the dual-write in `Request_Save` (v3-only saves now).
2. `FTag_Snapshot_JustRestored` + `CkSnapshot_RestoreMarker.h` + everything still keying on it — all inert since
   Phase 3B (v3 loads never stamp it): DELETE `FProcessor_Persistence_ReDriveOnRestore` +
   `FFragment_Persistence_ReDrivePending` AND the six Phase-1-deferred restore processors + their done-tags
   (Team, Player, Inventory Spatial + DataOnly, RenderTarget, 2dGridOccupancy — the [B1] deferral lands here),
   plus CkGrid's `FProcessor_2dGridSystem_RestoreRecompose` if nothing else references it (verify by grep first).
   (`Produce` stays everywhere — save capture + oracle use it.)
3. The Camera adopt-or-add blocks (`CkCamera_Utils.cpp:336-410` — restore `AddFloat/AddRange/AddVector/AddRotator/
   AddInt` to plain Adds; the restored-record collision they defended against cannot occur under v3), the Transform
   `_Previous` re-seed branch (`CkTransform_Utils.cpp:97-108` — revert to the pre-`15434d8ef` shape), and
   `FSnapshotPolicy_*` classifications (`CkSnapshot_Policy.h` + every holder/record template param — the policy
   becomes meaningless when capture is oracle-only; REMOVE the required param, keep a dated comment in the policy
   header explaining the retirement). If removing the template param fans out too widely (>~40 files), leave the
   params in place and gate only the marker's effect — record which path you took in PROGRESS.
4. Stale docs: update `CkEcs/CLAUDE.md` replication section (dispatcher group moved, fire-gating), root
   `CLAUDE.md` macro table row for `CK_REGISTER_SNAPSHOTABLE` ("oracle-only since Phase 5"), spec Status header →
   IMPLEMENTED.

### 5.3 Final gate
Run VALIDATION.md top to bottom. Then:
```bash
rg --no-ignore -n "ReplicateOnRestore|RestoreReplicated|JustRestored|Reconstitution" Plugins/CkFoundation/Source
```
(Amended: dropped `IsSnapshotRespawnable` per [P3B-D1] — it stays as the v3 `FFragment_ActorSpawnIntent` opt-in.)
Expected: zero hits outside `#if CK_WITH_FIDELITY_ORACLE` regions, campaign docs, and
`CkEcs/Snapshot/CkSnapshot_RestoreMarker.h` (the `FTag_Snapshot_JustRestored` symbol is retained there for cross-repo BB
consumers per [P5-D3] — see its retirement comment). Anything else → unfinished deletion → fix before committing.

Commits: one per 5.2 cluster + `docs: retire Model A from shipping path; snapshot doctrine update`.

## Exit criteria
- VALIDATION.md checklist fully green and pasted into PROGRESS.md.
- Shipping-config build compiles: `CkAuto\UnrealToolbox.exe --build --target Game --config Shipping` (proves the
  oracle gate actually excludes; expected clean).

## Fences
- Do NOT delete Model-A code the oracle/registry tests still call — GATE it. "Delete list" ≠ delete files.
- Do NOT bump plugin versioning files unless a `CLAUDE.md` Versioning section exists for CkFoundation (check; the
  per-plugin rule currently applies to GitLink only).
- The `CkCamera` revert (5.2.3) must be verified by `Ck.Snapshot.Parity.Attributes_MPReload` + a v3 save/load of a
  camera-owning pawn in the OracleParity fixture — if camera attributes diff post-revert, the revert is premature:
  STOP → Blockers (means some v3 path still restores camera children — investigate, don't re-add the hack).
