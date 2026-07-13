# VALIDATION.md — campaign acceptance protocol

Run at Phase 5 end (and any time a "done" claim is made about the campaign). Route the final classification
through `ck-change-control` (this is a framework-invariant-touching change: CkEcs core + scheduler + replication).

## 1. Headless gates (all from BB root; read verdicts from the log files)

```powershell
CkAuto\UnrealToolbox.exe --build --target Editor --config Development --output CkAuto\logs\val-build.log
CkAuto\UnrealToolbox.exe --test --test-pattern "Ck.Snapshot" --discover-fresh --output CkAuto\logs\val-snapshot.log
CkAuto\UnrealToolbox.exe --test --test-pattern "Net" --output CkAuto\logs\val-net.log
CkAuto\UnrealToolbox.exe --test --test-pattern "Ck.Attribute.Net" --output CkAuto\logs\val-attrnet.log
CkAuto\UnrealToolbox.exe --test --test-pattern "Ck.StateMachine" --output CkAuto\logs\val-sm.log
CkAuto\UnrealToolbox.exe --test --output CkAuto\logs\val-full.log
CkAuto\UnrealToolbox.exe --build --target Game --config Shipping --output CkAuto\logs\val-shipping.log
```

Expected, against the re-baselined gate table in PROGRESS.md (§Phase F3 cluster 4):
- [ ] Dev editor build clean; Shipping Game build clean — Model-A machinery is DELETED, not gated. Proof:
      (a) Shipping link succeeds with zero snapshot-machinery symbols (F3 cluster-4 record: 538s, only
      pre-existing JPH LNK4217 warnings); (b) FINALIZE.md's exit greps = 0 over Source/
      (`CK_WITH_FIDELITY_ORACLE`, `CK_REGISTER_SNAPSHOTABLE`, `SerializeSnapshot`, `Register_SnapshotableTag`,
      word-bounded per the "Fable F3 execution ruling" list; carve-outs: docs/campaigns/*, CkSnapshot_Policy.h
      until Option A, comment-only mentions slated for the cluster-6 sweep).
- [ ] `Ck.Snapshot.*`: every surviving campaign test green BY NAME: V3.CaptureClassification,
      V3.RecipeParamsHandleRemap, V3.InstancedStructDiskSmoke, LoadGate.GatedSkipsKernelTicks,
      AS.SaveGameFields_RoundTrip, AS.NonSaveGameField_Drops, Meta.RepDataRestoreCoverage,
      M2a.LoadOrchestration, M2b.LevelReload, M2b.OpenLevelSpike, M2b2a.ReplicatedRespawn,
      M2b2b.MPServerTravel, M2b2b.ServerTravelSpike, and Parity.{Acceleration, AnimPlan, Attributes,
      InventoryDataOnly, InventorySpatial, RenderTarget, StateMachine, StateMachineNoHistory, TagSet,
      TeamPlayer}_MPReload.
      (Amended 2026-07-13: dropped Oracle.* — deleted with Model A per FINALIZE.md obj-2; dropped the five
      Rebuild.* — never written (four recorded as optional follow-ups in FINALIZE.md; SpawnerResumes is
      [N1-A]-deferred); dropped Physics.Net.Velocity_ApplyAfterLateSetup — never written, subsumed per
      [P2-D3] by Parity.Acceleration_MPReload + the three Ck.Attribute.Net pins.)
- [ ] Allowlisted reds — acceptable ONLY as exact name-matches; ANY other red = FAIL:
      1. `Bb.Snapshot.FixtureReconstruct`  2. `Bb.Snapshot.PlayerRestore` — pre-existing BusterBlock-project
         casualties (not CkFoundation; the v3 rebuild finds 0 live entities to rendezvous against post-travel —
         the unbuilt BB-driver-world / OracleParity prereq — needs a BB-project GameMode; tracked BB-side).
      (FIXED 2026-07-13, no longer allowlisted: `Ck.Snapshot.Parity.GridPlacements_MPReload` — the "engine-death"
       was stale; a missing authority-side 2dGridOccupancy hydration branch, fixed CkF `ecd6f4019` → GREEN, so
       grid save/load now HAS passing coverage. See PROGRESS.md §Red/flake fixes.)
- [ ] The three pins green: `Ck.Attribute.Net.Values_AppliedBefore_OnReplicationComplete`,
      `Float_InitialBakedValue_Replicates`, `Float_PreComposition_StashedValue_Applies`.
- [ ] Full-suite run: failing set == the current re-baselined failing-name table in PROGRESS.md (§Red/flake
      fixes) — i.e. the 2 allowlisted `Bb.Snapshot` reds above (+ the kiosk env-trio in the wider BB suite).
      `Ck.Snapshot` is now 27/2 and `Ck.*.Net` is delta-ZERO (103/103/0 — the SM.Net flake was fixed
      2026-07-13, CkTests `3fdd6de`).
- [ ] No `Angelscript: Error` naming campaign files in any log.

## 2. Fidelity acceptance (v3 — post-oracle)
The fidelity oracle + Model-A registry round-trip net were DELETED (FINALIZE.md obj-2). The fidelity bar:
- [ ] Parity.*_MPReload family green (per-feature value fidelity across save + seamless travel) — enforced by §1.
- [ ] `Meta.RepDataRestoreCoverage` green — the coverage ratchet (every RepData type declares its restore
      disposition); this is the surviving structural net.
- [ ] The fidelity-gap register is CURRENT, not closed: FINALIZE.md obj-4 (G1 transform, G2 dynamic
      fragments, G3–G5, G17 item stack-count) + [F3-D1] coverage losses (grid round-trip, Timer-resume,
      MontagePlayer state) + [F3-D1b] the un-ported Model-A test migrations (the `V3.HandleWalk.*` suite,
      `V3.ProduceSensitivity`, `Parity.AttributeModifier_MPReload` — FINALIZE "cluster 0" port-before-delete,
      NOT authored before the purge). All are Adam product/scope decisions. Acceptance = the register is
      accurate; it does NOT mean the gaps are closed.
      (The old "deliberate-failure probe" retires with the oracle; its successor `V3.ProduceSensitivity` is a
      recorded follow-up, not an acceptance criterion.)

## 3. Three-environment checks (framework non-negotiable #4)
- [ ] C++: covered by the suites above.
- [ ] AngelScript: the two AS tests + `rg` the AS generated bindings for the new public surface
      (`Get_IsLoadGateActive` etc. if exposed — only UFUNCTION surfaces need AS verification; enumerate what you
      exposed and check each per `ck-angelscript-interop`).
- [ ] Blueprint: `Request_Save`/`Request_Load`/`Get_SaveSlotHeader` UFUNCTION surfaces unchanged in signature
      (BP-facing API diff = none expected; `rg -n "UFUNCTION" Source/CkSnapshot/Public/CkSnapshot/Subsystem/CkSnapshot_Subsystem.h`
      and compare against `bc484d645`).

## 4. [EDITOR-VERIFY] — human steps (cannot be automated; hand to Adam)
1. PIE a BB gameplay map → play ~2 min (move, interact, spawn something) → console `Ck_Snapshot_Save` (or the
   BP/subsystem call) → keep playing, change state → `Ck_Snapshot_Load` → world returns to saved state: position,
   camera feel (tuner attributes), interactables still interactable, no ISKM ghost bodies, no duplicate NPCs/pawns.
2. Listen-server PIE (2 players) → save on server → load → client receives the rebuilt world; second client's pawn
   re-adopted (no duplicate), values (health-style attributes) match server.
3. Save in editor session A, close editor, reopen, load in session B (cold-boot rendezvous of level-placed
   SaveKey actors).
4. Watch the log during one load for (expect none on an unmodified map): v3 capture-audit warnings
   (unlabeled-ConstructSpawned payload-drop), `_PayloadsDropped` ensures, orphan-hydration lines, and
   `Pump limit reached` warnings (would indicate a settle-pump feedback loop). The Model-A registration audit
   is gone — these are the v3-path watches.

## 5. Definition of done
All boxes above checked, PROGRESS.md carries the final gate table (the 29-test re-baselined suite; 3 allowlisted
reds BY NAME) + the load-time measurement vs the Phase-3B baseline, spec Status flipped to IMPLEMENTED, and the
campaign branch handed to Adam for merge decision (NEVER push/merge from an executor session).
