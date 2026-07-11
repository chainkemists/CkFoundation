# PHASE 3B — Load side: rebuild orchestration, reconciliation, retire reconstitution suppression

The heart of the migration. Spec §4.2/§4.3/§4.4 + CTO note N1 are the authority. Re-read them first.

## Entry criteria
- Phase 3A done per PROGRESS; saves now carry both Model-A and v3 bytes; all patterns green at 3A counts.

## Steps

### 3B.1 Load state machine rework (`CkSnapshot_Subsystem`)
Replace `ELoadPhase{...Restoring, RespawningActors}` tail with the v3 pipeline (TearingDown/travel/AwaitingWorld
phases keep their existing shapes and frame caps):
`Idle → TearingDown → AwaitingWorld → Rebuilding → Hydrating → Reconciling → Settling → Idle(done)`
- On world-ready (where the code currently escalates to Full at `:620`): `Set_IsLoadGateActive(true)` on the new
  world's EcsWorld subsystem instead of any reconstitution phase.
- **Rebuilding:** read the v3 entity table in file order (topology-sorted at capture):
  - RuntimeSpawned → `UCk_Utils_EntityScript_UE::Request_SpawnEntity` with recipe class + remapped params, lifetime
    owner resolved through the saved-id→handle map (owners appear earlier by ordering; missing owner → loud ensure,
    skip entry, LoadReport). Entries with `ActorSpawnIntent` ride the existing `FTag_ActorRespawn_Pending` +
    `FProcessor_ActorRespawn` path (kernel — it runs under the gate).
  - EngineOwned → rendezvous: poll for the keyed entity (SaveKey resolver for level actors — rehydrate the resolver
    map from live world-side SaveKey fragments, NOT from restored ones; player key via PlayerState). Not found yet →
    keep polling this phase (frame cap 600, same idiom as `DoIs_NewWorldReady`).
  - ConstructSpawned → resolve AFTER its owner reports constructed: look up (owner handle → labeled child via the
    record/label utils). Found → map saved-id → child handle. Not found (content changed) → orphan-report line.
  - Phase exits when every entry is mapped/reported AND all spawned scripts finished construction (poll like
    `DoIs_RespawnComplete`, `:537-563` — iterate, never `view.empty()`).
- **Hydrating:** for each mapped entity: write its payload list into `FFragment_PendingHydration` (+
  `FTag_Hydration_PendingApply`). Payload handle members remap through the completed saved-id map (a v3
  `FSnapshotContext` mode wrapping `TMap<uint64, FCk_Handle>` — add it beside the loader-backed mode in
  `CkSnapshot_Context.h`). Phase exits when no entity carries `FTag_Hydration_PendingApply` (dispatcher drains
  under the gate) or the dispatcher's timeout has loudly dropped stragglers (they are in the LoadReport).
- **Reconciling:** spec §4.2 subtractive pass, framework-side here (not a processor): for each owner in the saved
  table, enumerate its live labeled ConstructSpawned children (live tag + label + record walk); any NOT in the
  saved child set → `Request_DestroyEntity`. NOTE: the destruction pipeline is GATED (Phase 2.2 rationale) — these
  destroys PARK and complete on the first normal frame after gate-open, with feature EndPlay processors live. The
  reconcile phase only needs the requests QUEUED; do not wait for completion here.
- **Settling:** `Request_PumpToQuiescence(LoadKernel)` (from the FTSTicker — outside any scheduler tick, the
  re-entrancy skip at `:250-255` cannot bite) → `Set_IsLoadGateActive(false)` → `DoFinish_Load` (existing report
  + signals). The first normal frame drains all accumulated NeedsSetup/requests via the ordinary pump.
- `Request_Load` reads v3 bytes when present (header sniff), else falls back to the Model-A path UNCHANGED (old
  saves keep loading until Phase 5 removes the fallback — actually fork ruling 5 says hard-break: v3-only, reject
  v2 with `Failed_IncompatibleSave`. DO THAT — delete the fallback read, keep Model A code compiled for the oracle).

### 3B.2 Retire reconstitution suppression
Delete: `ECk_ReconstitutionPhase` + accessors + `_ReconstitutionPhase` (`CkEcsWorld_Subsystem.h/.cpp`),
`DoIs_WorldReconstituting` + its two call sites (`CkEntityScript_Utils.cpp:42-71,147,196`),
`Get_IsSnapshotRespawnable` (`CkEntityScript.h/.cpp`), the `OnPostWorldInitialization` watch +
`DoSet_ReconstitutionFlag` + quiescence-frame countdown in `CkSnapshot_Subsystem`. The GameMode default pawn is now
EngineOwned-adopted; nothing needs suppressing. Verification: `rg --no-ignore -n "Reconstitution|IsSnapshotRespawnable" Source`
→ zero hits outside campaign docs/history.

### 3B.3 Tests (CkTests) + gate
Existing suite is the main gate: all `Ck.Snapshot.Parity.*_MPReload` + `M2a/M2b*` orchestration specs must pass
**through the v3 path** (they exercise Request_Save/Request_Load end-to-end; they now run rebuild+hydrate).
EXPECTED CASUALTIES (rewrite, don't delete): tests asserting Model-A mechanics on the LOAD path —
`Ck.Snapshot.LifecycleStrip` (no lifecycle strip in v3 — rewrite to assert no `FTag_DestroyEntity_*` survives a v3
load), `Ck.Snapshot.M2b2a.ReplicatedRespawn` (respawn semantics changed — rewrite assertions to end-state parity).
Registry-level tests (`Core.RoundTrip`, `DynamicFragment.*`, `Audit.*`) exercise Model-A internals directly — they
stay green untouched (Model A code still compiles). Any OTHER existing test red → STOP → Blockers.
New tests:
- `"Ck.Snapshot.Rebuild.NoDuplicateGrants"` — script whose Construct grants a labeled child; save; load; assert
  exactly one child.
- `"Ck.Snapshot.Rebuild.LostGrantStaysLost"` — grant then destroy the child pre-save; load; tick ≥2 normal frames
  AFTER load-complete (reconcile destroys park until gate-open — Phase 2.2), then assert absent.
- `"Ck.Snapshot.Rebuild.OrphanHydrationLoud"` — save with a labeled child, load with a fixture script variant that
  no longer creates it (test-only class switch); assert LoadReport orphan line + no crash.
- `"Ck.Snapshot.Rebuild.OracleParity"` — representative fixture world: Tier-1+Tier-2 oracle across a full v3
  save→load; allowlist = `docs/campaigns/saveload-rebuild-hydrate/oracle-allowlist-p3.txt` (CREATE it; N1: expected
  duplicate lines for driver/SM-spawned subordinates go here, each with a `# Phase-4-pending:` comment). Assert
  zero NON-annotated diffs.
- `"Ck.Snapshot.V3.InstancedStructDiskSmoke"` — FInstancedStruct param round-trip on 5.7.4 (CTO suggestion 7).
- Record (not assert) load wall-time from the orchestration spec log → PROGRESS (CTO suggestion 6 baseline).
```powershell
CkAuto\UnrealToolbox.exe --build --test --test-pattern "Ck.Snapshot" --discover-fresh --output CkAuto\logs\p3b.log
CkAuto\UnrealToolbox.exe --test --test-pattern "Net" --output CkAuto\logs\p3b-net.log
CkAuto\UnrealToolbox.exe --test --test-pattern "Ck.Attribute.Net" --output CkAuto\logs\p3b-attrnet.log
```
**Known interaction to expect (from the [B1] ruling):** the six Phase-1-deferred features' `Apply` handlers are
client-shaped (stamp-a-sync-fragment idiom; their sync processors may be ClientOnly), and under v3 their
authoritative state partially rebuilds from recipes anyway (items are first-class recipes). If a deferred feature's
parity spec reds here with hydration applied-but-not-drained on the authority, that is THIS known shape: annotate
it against the feature and record it in Blockers as a Phase-4B work item (RenderTarget already is one) — do not
invent an authority-side sync drain ad hoc.

**Decision gate:** every red must be either (a) an expected-casualty you rewrote, or (b) in the oracle allowlist.
A red Parity spec = a hydration coverage/ordering bug — diagnose via the oracle diff lines for that feature before
touching code; if the diff implicates a feature not in this phase's scope → allowlist ONLY if it matches N1's
driver-spawned-subordinate shape, otherwise STOP → Blockers.

Commits: `feat(CkSnapshot): v3 rebuild+hydrate load pipeline (gate, reconcile, settle)`;
`refactor(CkEcs,CkEcsExt): retire reconstitution suppression machinery`; (CkTests) one commit per test cluster.

## Exit criteria
- All patterns green modulo the annotated allowlist; allowlist file exists, every line `# Phase-4-pending`-tagged.
- `rg "Reconstitution" Source` → 0 hits. PROGRESS: load-time baseline number recorded.

## Fences
- Do NOT delete Model-A capture/restore code (oracle + registry tests use it until Phase 5).
- Do NOT "fix" an N1 duplicate by suppressing the spawner — that is Phase 4A's hydration job. Allowlist + annotate.
- Do NOT build spawn-params forward-ref fixup (capture already ensures against it).
- The reconcile destroy must go through `Request_DestroyEntity` — never direct registry destruction.
