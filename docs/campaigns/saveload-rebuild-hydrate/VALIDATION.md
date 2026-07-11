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

Expected, against the Phase-0 baseline table in PROGRESS.md:
- [ ] Dev editor build clean; Shipping game build clean (oracle gate excludes correctly).
- [ ] `Ck.Snapshot.*`: every Phase-0 baseline test green (or its documented Phase-3B rewrite green), PLUS all
      campaign-added tests green BY NAME: Oracle.StructuralBaseline, Oracle.ProduceDiffBaseline,
      LoadGate.GatedSkipsKernelTicks, Physics.Net.Velocity_ApplyAfterLateSetup, V3.CaptureClassification,
      V3.RecipeParamsHandleRemap, V3.InstancedStructDiskSmoke, Rebuild.NoDuplicateGrants,
      Rebuild.LostGrantStaysLost, Rebuild.OrphanHydrationLoud, Rebuild.OracleParity,
      Rebuild.SpawnerResumesPastSpawnDecision, AS.SaveGameFields_RoundTrip, AS.NonSaveGameField_Drops.
- [ ] The three pins green: `Ck.Attribute.Net.Values_AppliedBefore_OnReplicationComplete`,
      `Float_InitialBakedValue_Replicates`, `Float_PreComposition_StashedValue_Applies`.
- [ ] Full-suite run: failing set == Phase-0 baseline failing NAMES exactly (the ~9 known pre-existing BB reds are
      acceptable ONLY as name-matches).
- [ ] No `Angelscript: Error` naming campaign files in any log.

## 2. Oracle acceptance
- [ ] `Rebuild.OracleParity`: zero unexplained diffs; `oracle-allowlist-p4.txt` empty of pending lines;
      `oracle-declared-transient.txt` — every line carries a reason string.
- [ ] Deliberate-failure probe (proves the oracle can see): temporarily comment out one feature's `Produce`
      (e.g. Velocity), run OracleParity → MUST go red with a line naming that payload type. Restore, re-run green.
      Record both runs in PROGRESS. A oracle that cannot fail is not an oracle.

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
4. Watch the log during one load for: orphan-hydration lines (expect none on an unmodified map), audit warnings
   (expect none), `Pump limit reached` warnings (expect none — would indicate a settle-pump feedback loop).

## 5. Definition of done
All boxes above checked, PROGRESS.md carries the final gate table + the load-time measurement vs the Phase-3B
baseline, spec Status flipped to IMPLEMENTED, and the campaign branch handed to Adam for merge decision (NEVER
push/merge from an executor session).
