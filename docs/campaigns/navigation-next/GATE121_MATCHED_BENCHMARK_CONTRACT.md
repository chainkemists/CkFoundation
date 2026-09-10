# Gate121 — local matched benchmark contract

2026-09-10. Supersedes the old showcase-first blocking state: the CTO approved all seven gyms and requested no further checks. Build-machine performance collection moves to BusterBlock integration by explicit user instruction; it is deferred, not accepted or replaced by editor timing.

## G121-D2 — one authored scene, two real query services

The benchmark owns `/CkTests/GroundNavBenchmark/Maps/GroundNavMatchedBenchmark`. Author a static 3600x2400 floor, top Z=0, and four static 150x150x300 pillars at (+/-350,+/-350). Both systems consume these exact tagged actors: authored Recast bounds/data and Jolt-backed GroundNav bake. Agent radius 42, full height 192 (GroundNav capsule half-height 96). No changes to approved gyms, shared AutoTests geometry or project provider defaults. Create the new map only when absent; validate an existing map without silently overwriting it.

| Workload | Production service | Evidence |
|---|---|---|
| QueryBurst128 / Recast | deferred `UCk_Utils_Nav_UE::Request_FindPath` | terminal nav result and algorithm diagnostics |
| QueryBurst128 / GroundNav | deferred `UCk_Utils_GroundNavPath_UE::Request_FindPath` | terminal GroundNav result and sliced-search diagnostics |
| CrowdConvergence240 / either | ordinary Crowd `Request_MoveTo`, selected world provider | Walking/reached eligibility, goal failures and off-surface observations |

The low-level Nav request processor is Recast-specific. Changing the world provider does **not** make that API a GroundNav query; sending both legs there would produce a mislabeled comparison. The query-only benchmark explicitly dispatches the real native queued services and normalizes results. No Crowd motion or dynamic crowd filtering is mixed into the query burst. The provider-neutral synchronous facade remains the endpoint/readiness oracle, not the asynchronous throughput workload.

## G121-D3 — diagnostic scope and eligibility

Recast generation and selector dimensions must both be 42/192. Pin all Recast resolution cell heights to 1 uu (the engine default is 10), retaining the same raw Z=0 workload and 2 uu projection tolerance rather than moving one provider's endpoints. Wait for Recast build completion and validate the complete map before its first save. This fixture-specific authoring does not change project defaults.

The host's supported agent is 35/144. A benchmark-owned `UNavigationSystemModuleConfig` subclass is therefore persisted on this map's WorldSettings and installs 42/192 on the new world navigation-system instance before registration. Recast generation settings alone do not override supported-agent registration. Never modify the navigation-system CDO or host INI to make this map pass.

Provider-work duration includes actual begin/search work, excludes inter-frame queue/park waiting and result installation. Add explicit availability, since zero milliseconds can be measured and must not mean missing. GroundNav currently accumulates ContinueSearch but omits Begin/BeginRepair; include those calls in the existing accumulator. Do not add unused facade diagnostics or change Crowd installation solely for this harness. Readiness retries that have no built field remain unmeasured.

QueryBurst128 uses fixed elliptic endpoints (radii 1400/900) and antipodal goals, same raw positions for both providers; prove every endpoint projects under both before issuing. Exactly 128 Ready, zero Partial/Failed, finite available search diagnostics and complete per-frame/individual terminal accounting are required. Record observed issue-to-terminal latency as **end-to-end**, never label it pure queue wait. Record both providers' budgets; different scheduling budgets must remain visible.

CrowdConvergence240 uses six staggered rings of forty agents at radii 750..1000, outside all four pillars, and antipodal goals. Start the 3-second warmup plus 6-second sample only after all agents are Walking or reached. This gates initial readiness only: legitimate PathPending replans remain in the continuous sample. Report raw frame durations, average/p95/max/FPS, reached count, sticky goal failures and off-surface counts, unique replanned agents and the terminal movement-state mix. An uninitialized terminal movement state is ineligible. Differences in completion mix remain visible, not disguised as equivalent behavior. No timing pass threshold or performance winner.

The runtime actor rejects reentrancy, validates its authored fixture before work, owns only its created entities/Jolt admissions, and restores provider/cvars on success, failure and EndPlay. At the crowd sample boundary it snapshots outcomes, requests Stop through the normal Crowd API and waits for Idle before validating captured positions; post-sample validation must not extend the live workload. Normal completion waits for actual entity retirement (including pending-kill entities), cleanup and provider settle. Four native editor automation rows exercise both modes/providers on the saved map, without AS wrapper/populator changes. Missing data or failure means ineligible with an explicit reason, never successful partial setup.

## Report and checks

One `[NAV-BENCHMARK]` JSON record per run, schema 1, `purpose=local-correctness`, includes fixture path/hash/version, environment/config/machine, capsule, real provider/mode, readiness/restoration, raw query samples and histogram or raw frame durations, derived metrics and eligibility/reason. `Plugins/CkTests/Tools/nav_benchmark_report.py` independently validates type/finite-data/count/histogram/percentile/eligibility consistency and retains input-log SHA256 identity. It makes no cross-provider performance claim.

Implementation exemplars: `CkAutoTest_NavSurface_RecastBudgets_PathThroughput.as` (drain observations), `CkAutoTest_Crowd_SteeringPerf.as` (crowd composition and named eligibility), `CkGroundNav_StreamingAcceptanceActor` (native runtime bridge), `CkGroundNavStreamingAcceptance_EditorUtils` (scoped author/save), native PIE latent tests (real-world exercise).

Local exit: incremental Toolbox build, all new contract/runtime rows pass on final source, fresh diagnostics inspected, report validator consumes real outputs, and original dirty work remains preserved. Planned boot count will be stated before launch after final test registration is known. Human snap/undo/LiveExtract, actual WP/cooked-manifest/package acceptance remain separate; no local cook/package, Git delivery, or BusterBlock checkout changes.
