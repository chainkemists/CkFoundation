# Gate124 BusterBlock per-run A/B operator contract

Status: Gate126 focused NpcAI is 27/27 after the liveness fixture correction. The Gate124 isolated Recast/GroundNav/Recast-rollback A/B passes remain historical evidence after the EQS rebuild; no fresh A/B result is claimed. Production retains Recast with shadow Off. New tracked dev tips still require a controlled preservation/rebase checkpoint, and broader editor/streaming/package/performance acceptance remains deferred. Current evidence: `GATE126_VALIDATION_EVIDENCE.json`.

## Local command surface

Use the existing BusterBlock checkout. The launcher never builds, cooks or packages. Use a compatible built Editor and the authored-map validation from `BusterBlock.UnitTests.Navigation`; `BusterBlock.UnitTests.Navigation.NavABFixture.AuthoredMap` creates a missing fixture map and validates an existing one without replacing dirty work.

```powershell
Set-Location D:\Repos\BusterBlock
# Inspect a run without launching anything.
.\Tools\navigation\Invoke-NavigationAB.ps1 -Provider Recast -RunId gate124-preview -Seed 124 -DryRun

# Separate processes, same fixture and seed; use fresh run IDs for every attempt.
.\Tools\navigation\Invoke-NavigationAB.ps1 -Provider Recast -RunId gate124-recast-<fresh-id> -Seed 124
.\Tools\navigation\Invoke-NavigationAB.ps1 -Provider GroundNav -RunId gate124-groundnav-<fresh-id> -Seed 124
.\Tools\navigation\Invoke-NavigationAB.ps1 -Provider Recast -RunId gate124-recast-rollback-<fresh-id> -Seed 124
```

The launcher invokes the vendored Toolbox `--gauntlet=NavigationAB`, one fresh Editor `-game` process per call. It injects `BB_NAV_AB_PROVIDER`, `BB_NAV_AB_RUN_ID`, `BB_NAV_AB_SCENARIO=AccessAndShoppingV1`, and `BB_NAV_AB_SEED` into that process, then restores the calling shell's environment. Missing values together mean ordinary production; partial/malformed options reject. Seed is provenance for this deterministic fixed workload; it does not currently randomize endpoints.

Startup selection runs after the ECS world subsystem initializes and before actor initialization. Only `/Game/Tests/Navigation/BusterBlockNavigationAB` in a standalone world may admit the trial. There is no live switching or mixed cohort support. The runtime actor remains idle without an explicit run request. The Gauntlet adapter admits exactly one authored scenario actor.

## What the isolated scenario proves when green

The authored open/tall three-lane fixture retains the game's effective supported-agent Recast profile 35/144 and default-resolution cell height 1, measured in Gate124c. The ini nominal radius is 45 and resolution heights are 10/1/10; these declarations do not imply a live radius of 45. GroundNav uses the game's Crowd capsule 42/192. This existing profile difference is reported; this fixture cannot prove low-clearance equivalence. The scenario creates a live local GroundNav field, waits for publication, then owns restricted, shopping-boundary and standing-crowd markups through the game compatibility bridge. Recast backing is retained for both providers.

Before policy checks, each lane must have a complete unfiltered control route. All eight actual game filters are then checked against all three lanes (24 policy checks), followed by a positive Employee Crowd crossing. Missing provider/data cannot count as policy rejection. Completion requires the Crowd arrival location, provider/shadow identity, markup removal settlement while the field exists, and owned-entity retirement. Reported effective provider comes from the runtime world.

This is native filter/markup and Crowd-route compatibility coverage. Production AccessZone/Entryway AS lifecycle and the existing NpcAI/sidewalk/queue checks are distinct gates. The game continues to use direct Recast projections and sidewalk extraction. The isolated native fixture does not exercise the complete customer ingress/shopping/checkout/egress state machine or settle the named queue debt.

## Evidence and eligibility

Keep `Saved/Logs/NavigationAB-<run-id>.log`, the unique `*.identity.json` envelope and archived `Saved/Logs/Gauntlet/.../NavigationAB_r*.log` runtime logs. The envelope captures source commits plus dirty/untracked file hashes, relevant DLL/receipt/map hashes, machine, seed, requested provider and before/after identity. The runtime `[BBNavAB-REPORT]` records actual effective provider, policy outcomes, field input fingerprint/epoch, readiness, cleanup, scheduler settings and failure reason.

GateL historically built the prior Development Editor in 14.65 seconds. The controlled `gate124-recast-c`, `gate124-groundnav-c`, and `gate124-recast-rollback-c` manifests each have Toolbox exit 0, unchanged/matching identity, 24/24 policy checks, Crowd arrival, and cleanup/markup-retirement settlement. All three reports measure effective Recast 35/144 with default cell height 1; the fixture uses markup half-Y 300. The earlier half-Y 325 trials remain failure evidence: their painted regions extended 25 units beyond wall centers and did not isolate policy lanes. Half-Y 300 is a fixture isolation correction, not production overlap-parity evidence. `GATE124_FINAL_CHECKPOINT.json` is the authoritative final identity/evidence index: it records all eight repository heads matching the post-rebase checkpoint and local-dev refs matching entry, protected files 13/13, mirrored framework/test files 14/14, six ancestry checks, and source/artifact hashes unchanged across the three final trials. The host binaries were not rebuilt after mirroring, which is an explicit evidence limit. All three final raw logs contain the startup FText self-test error but no ensure or AngelScript error.

The launcher rejects reused evidence IDs, wrong build configuration, missing artifacts, changed source/binary/map identity, missing/duplicate report, mismatched identity/provider or failed runtime verdict. Toolbox exit 0 alone is insufficient. Inspect fresh raw startup/test diagnostics too; expected test ensures must remain distinguished from unexpected diagnostics. A failed provider or behavior check is ineligible, never a fallback win. Historically, Gate125e focused `NpcAI` was **26/27**, one boot, 5m26s, with all 27 rows and 0 skipped/contaminated. Its sole failure was the unchanged-game `LivenessWatchdogAccrual` live-line exemption (1.066707 s; baseline about 1.066671 s). All five new Gate124j/Gate125c names, plus `BelowFloorRecovery` and `SidewalkOwnedRoute`, now pass; no new name failed versus entry. `SidewalkOwnedRoute` was an inconclusive pre-fix observation and production tuning did not change. `GATE125_VALIDATION_EVIDENCE.json` preserves that historical result. Gate126 corrects the fixture and passes 27/27; `GATE126_VALIDATION_EVIDENCE.json` records the current focused gate. This focused result does not make the isolated A/B slice broader readiness; the Gate124 A/B manifests are historical after the EQS rebuild and no fresh A/B run is claimed.

**Current rebase debt:** BusterBlock tracked `origin/dev` advanced independently after Gate124 to root `c314b1ed7d81b9f7461afb7900e85dc8f425de0d`, Foundation `369d6766d6fd62537a61886f391cb5aa2b5660fc`, and Tests `b8087d6eff8fdaa3e93452630db16568e2268d98`; none is an ancestor of the current candidate HEAD. Host tracked refs are unchanged. The candidate’s local-dev refs, working HEADs, index gitlinks, protected 13 files, and final-run identity remain unchanged, so Gate125e measures the current local candidate only. The six prior ancestry checks cover Gate124’s fetched dev/entry pins, not these new BusterBlock tips. Before any further rebase, establish a controlled preservation checkpoint for the uncommitted implementation in the shared checkout. Gate125 did not attempt a rebase or make additional commits.

## Additional verification limits

The per-run contract freezes filter policy as well as provider selection. Admission/compiler rejection is tested, but changing a named filter asset while an already-installed route is active does not independently invalidate that route: GroundNav invalidation is driven by field publication epochs. Live policy mutation needs its own invalidation design and production-path coverage before it is supported. Do not describe this slice as live filter switching or complete rejection of stale installed policies.

GroundNav markup failure callbacks implement rollback in source, but the current public readiness result can fail before the callback drains. The successful `-c` cleanup proves the admitted happy path only; it does not directly prove async GroundNav rejection rollback. This remains an explicit verification gap.
## Deferred build-machine acceptance (not launched)

1. Freeze the eventual feature commit and game/plugin gitlinks, resolve dirty identity, and archive the exact source/config/profile/filter/markup contract. Do not advance dev to make the build machine consume it.
2. On the build machine, run the established GroundNav commandlet DryRun, inspect all profile manifests/index/tiles, then perform the real write/cook. Verify selector, identity/fingerprint and loaded-from-cook behavior. Local live-field success is not cooked-field evidence. Sequential file-save failure is not transactional rollback; repair I/O and recook.
3. Build/package Development and Test and execute the isolated compatibility scenario with both providers and rollback. Verify production Recast startup, eight filters, AccessZone/Entryway lifecycle, direct sidewalk/projection dependencies and representative gameplay. Confirm Shipping compile/package/provider-selection/load separately. The current Editor-only AS Gauntlet adapter is not a packaged runner; a packaged controller and cooked-field admission path must be supplied and gated before package acceptance can be claimed.
4. After functional behavior is accepted, collect at least three eligible alternating pairs on one Development-game artifact, with identical scenario/settings/seed, raw samples, machine/build/map identity and provider Insights traces. The current fixed-workload correctness report is deliberately not performance eligible. Different behavior makes a pair inconclusive.
5. Migrate one real production map and exercise rollback only after the separate isolated and required package gates and a later adoption decision. No global GroundNav opt-in is part of this slice.

## Obligations retained in the handoff

- Outside-PIE node snap on flat/ramp/overlapping floors, explicit off-surface rejection, responsiveness and wrong-floor checks.
- Editor undo/redo restoration of positions, routes and connections.
- LiveExtract large-geometry update timing, field identity/epoch, localized preview and undo/redo resweep.
- Real authored World Partition/data-layer load/deactivate/reactivate/replace/unload/reload with idle, installed and in-flight routes.
- Real commandlet/cooked manifests/index/tiles/profile selector and loaded-from-cook proof.
- Development/Test packaged gameplay and Shipping compile/package/provider/load proof; no local cook/package.
- Matched build-machine performance, at least three eligible alternating pairs, raw samples and Insights.
- Later one-production-map adoption and rollback.
- Gate102 Recast Queue front-slot debt (240 polls / 6.08 seconds); no further arbitrary timeout increase.
- Historical Gate112 failures: eight SceneNodeTween rows, ScriptProcessor_PumpStopsAfterMarkerDrain and VisualLod_RenderBandProfileLifecycle; historical until freshly rerun, unrelated fixes excluded.
- Preserved BusterBlock NPC voice/transform-follow, CPU/lifetime/contact-churn work, all 13 protected files and approved presentation work. Seven approved gyms stay closed. Historical CPU handoffs do not authorize restarting unrelated work.
