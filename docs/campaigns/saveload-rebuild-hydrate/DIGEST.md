# DIGEST — CkSnapshot save/load: the unified **v3 rebuild+hydrate** solution vs the retired **Model A**

*Capstone comparison, 2026-07-13. Full detail lives in FINALIZE.md (locked design) + PROGRESS.md (execution log). This is the one-page "what changed and why."*

## TL;DR

The campaign replaced a two-pipeline design — a "serialize the world's bytes" path (**Model A**) running *alongside* a rebuild path — with a **single** pipeline that **re-runs the world's construction, then replays each feature's state through the same handlers the network already uses**. Model A's machinery is **deleted, not gated**: the fragment registry, the entt archives, the tag driver, the fidelity oracle, and the per-fragment `SerializeSnapshot` surface are gone. Shipping-Game compiles with zero snapshot-machinery symbols.

## The two designs, side by side

| | **Model A (retired)** | **v3 rebuild+hydrate (survivor)** |
|---|---|---|
| **Capture** | Each fragment implements `SerializeSnapshot`; a global registry (`CK_REGISTER_SNAPSHOTABLE`) enumerates snapshotable fragments; an entt archive (`FSnapshotArchive_Writer`) serializes them to bytes; tags via `TagRegistry`/`TagDriver`; a **fidelity oracle** cross-checks byte-for-byte. | `CaptureV3` walks the world; per entity it records a **provenance** (EngineOwned / ConstructSpawned / RuntimeSpawned / DefinitionBuilt) + a **spawn recipe** (how to re-create it) + per-feature **Produce payloads** (the *same* `Produce` the replication handler registry uses; `Get_SaveHandlerTypes` selects the Save-transport subset). |
| **Load** | Deserialize the archive back into the *existing* world; re-drive restored entities via a `JustRestored` marker → `ReDriveOnRestore` processors. | **Tear down** the gameplay world → **rebuild** entities from recipes (context-owner-first) → **hydrate** each feature by feeding its payload through the *same `Apply` handler the wire uses* (`FCk_HydrationApplyScope`). Handle fields remap through `FSnapshotContext::Snapshot_Handle` (saved-id → live-handle map), re-homed onto the live registry. |
| **Mental model** | *Photograph the world's bytes; paste them back.* | *Re-run construction; replay each feature's state down the net path.* |

## Why the unified design wins (the core insight)

1. **Save/load and networking are the same problem** — reconstructing an entity's state on a fresh instance. Model A ran a *separate* serialization path (`SerializeSnapshot`) parallel to the replication path (`Produce`/`Apply`). v3 collapses them: the save file drives the **exact** handlers the wire drives. One code path, exercised by *both* the Net suite and the Snapshot suite.
2. **No dual-registration tax.** Model A required every persistable fragment to *both* `CK_REGISTER_SNAPSHOTABLE` *and* implement `SerializeSnapshot` *and* carry a Policy classification. v3: a feature persists by opting its existing net handler into `ECk_PersistenceTransport::Save`. Zero new per-fragment surface.
3. **Rebuild gets identity & lifetime right.** Pasting raw bytes into a live — or seamlessly-traveled — world leaves entity IDs, ownership, and actor bridges stale (an `FCk_Handle` restored as raw bytes can even resolve to the *wrong* entity). v3 rebuilds under the real lifetime/context-owner graph and re-homes handles onto the live world — correct across OpenLevel **and** seamless server travel (the `M2b`/`M2b2b` gates).
4. **Shipping ships less code.** The machinery is *deleted*, not `#if`-gated. Smaller binary, no dead-but-compiled path, no oracle-gate maintenance.

## What it cost — the honest ledger (all recorded for Adam, none silently accepted)

- **Coverage losses (Adam-gated):** `[F3-D1]` the Grid / Timer / MontagePlayer **registry round-trip** tests were compile-coupled to Model A → deleted with it; `[F3-D1b]` the planned *port-before-delete* migrate set (`V3.HandleWalk.*` handle-remap incl. the tombstone-incident case, `V3.ProduceSensitivity`, `Parity.AttributeModifier_MPReload` — the **only** modifier-across-save/load coverage) was never authored before the purge.
- **Open fidelity gaps (obj-4):** G1 transform, G2 dynamic fragments, G17 item stack-count, … — features whose `Produce`/`Apply` does not yet round-trip full state. A living register, not closed.
- **`GridPlacements_MPReload` — since FIXED (2026-07-13, post-F4):** the long-assumed "engine death" was stale (the teardown-drain mitigated it). The real gap was a missing authority-side 2dGridOccupancy hydration branch (the same [INV-A]/[F1-D6] pattern); adding it made both the server + client asserts green, so grid save/load now HAS passing coverage. The remaining reds are the 2 pre-existing `Bb.Snapshot` project casualties (blocked on the unbuilt BB-driver-world prereq).
- **The oracle's gap-detection retired.** Model A's oracle could flag "feature X's `Produce` disagrees with its captured bytes." v3 has no exhaustive structural cross-check; the per-feature `Parity.*_MPReload` gates + the `Meta.RepDataRestoreCoverage` ratchet (every RepData type must declare its restore disposition) are the replacement — targeted, not total.

## By the numbers

- **Deleted:** 16 machinery files (CkEcs) · 21 Model-A-coupled tests (19 registry + 2 oracle) · the per-fragment `SerializeSnapshot` surface across 41 feature fragments · the `CK_REGISTER_SNAPSHOTABLE` registrar on all ~229 ECS tags · the fidelity oracle + `CK_WITH_FIDELITY_ORACLE` gate (now 0 references repo-wide).
- **Snapshot suite:** 54 → **29** tests (the drop *is* the Model-A test deletions), **27 green + 2 reds** (the 2 pre-existing `Bb.Snapshot` project casualties; GridPlacements was fixed post-F4).
- **Gates:** Dev delta-zero · Net **103/103/0** (the `StateMachine.Net` flake was fixed post-F4) · **Shipping-Game compile Succeeded**, zero snapshot symbols in link diagnostics.

## Status for the reviewer

**79 commits on `feature/save-load-improvements`, unpushed.** Class-4 (framework core: CkEcs snapshot + scheduler + replication) → **Adam review mandatory before any push.**

- **F1** — fixed the multi-day `[INV-A]` inventory-restore bug (items + occupants now round-trip through the shared pipeline).
- **F2** — deleted the dead `SeedContainer` path + hardened live-load footguns.
- **F3** — the Model-A purge (clusters 2–6: entry points → per-fragment sweep → CkEcs teardown → docs/acceptance → comment sweep).
- **F4** — hygiene: 2 `ck::algo` conversions + campaign-scaffolding comment sweep.

Every deferral and gap is an **Adam-decision item in FINALIZE.md**, not a silent acceptance: the coverage losses (`[F3-D1]`/`[F3-D1b]`), the obj-4 fidelity gaps (incl. the grid occupant-handle fidelity surfaced by the GridPlacements fix — same [INV-A] provenance family), the deferred `ck::algo` remainder (`[F4-D1]`, invalidated by F1's rewrite), and the policy-macro **Option A** retirement (`[F3-D2]`, the one remaining coherent Model-A-vocabulary surface left inert on purpose). Post-F4, Adam asked to fix the reds: GridPlacements + the `StateMachine.Net` flake are now GREEN; the 2 `Bb.Snapshot` reds stay (BB-project, blocked on the unbuilt BB-driver-world).
