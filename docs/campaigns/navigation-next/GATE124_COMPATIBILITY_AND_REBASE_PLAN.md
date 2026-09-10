# Gate124 - BusterBlock compatibility entry and preservation plan

Date: 2026-09-10. Status: source/ancestry audit complete; baseline 22/27, exit 1; **preservation approved by the user; execution underway**. No compatibility implementation, candidate installation, rebase, commit or publication has occurred.

Authority: `CONTINUATION_PROMPT_BusterBlockCompatibilityAndABReadiness.md`. This checkpoint follows its changed ordering. The seven approved gyms stay closed. All deferred acceptance obligations remain in that handoff and VALIDATION.

## Verified checkout and ancestry

`GATE124_ENTRY_EVIDENCE.json` records all eight root/Foundation/Tests/Toolbox checkout identities, statuses, dirty-file SHA256 values, local dev refs and refreshed origin/dev refs. No local dev ref was updated. Only origin/dev tracking refs were fetched, with no tags or recursive submodule update.

| Existing checkout | HEAD | Refreshed origin/dev | Behind / ahead |
|---|---|---|---|
| CkPlugins_3 | fd9f6abf7d0c9d4de95d1f98b714d11c48ca7101 | 477122a36b89ccaa185365f0cfeeaf4a3b5a7fe0 | 2 / 60 |
| CkPlugins_3 Foundation | ea6f8bf0b714bd4a398022c87f72a8e3f2d06ccb | 54e9e6943cd075025673a600d7279a3e412b2fa4 | 35 / 66 |
| CkPlugins_3 Tests | cfd3ea1704ba45f5ec1feca48b4ac46ac541dc35 | 68533b5e6416f901deffbecf7cec880084dc407f | 14 / 77 |
| BusterBlock | eab2de2ab8ab83bee512e605cc7e4dca518a86c1 | a16b2bce594dc125787b8b702ce7bd1543076a23 | 5 / 5 |
| BusterBlock Foundation | b46cc762bc6d5a0452ad2470eba760b85955848d | 54e9e6943cd075025673a600d7279a3e412b2fa4 | 1 / 0 |
| BusterBlock Tests | c3b7c8ffe3ec2628f27ccecc1b1ea04909070ab2 | 68533b5e6416f901deffbecf7cec880084dc407f | 1 / 0 |

Both game plugin pins are ancestors of refreshed dev, and neither is an ancestor of the committed navigation candidate. Foundation's merge base is `fdfb9f406c07c14f38e3f1bbf6838c2724d3daf0`; Tests' is `658ae56756eaa4306eacc3493cd25a4719a846c1`. A direct candidate replacement would omit current game changes; rebasing must reconcile them first.

All initial identities match the authoritative handoff, including Toolbox pins `5532971dbbb821bf8e5420e350a61afb79d0eb3a` (host) and `676b70c457500d6f9f139042743c6eea3aadd068` (game). All 13 protected hashes, 21 presentation hashes and 26 Gate123 source/tool/binary/log hashes match: **60/60**. Gate123 map MD5 also matches `3969ce9a7287f6b9eb43b07283e692c5`; it is not included in that SHA256 count. The offline validator rerun passed 21 tests and accepted four eligible historical records, both commands exit 0. This verifies artifact identity and validator behavior, not game compatibility or new runtime evidence.

## Concrete preservation request

The handoff explicitly says: "If safe in-place rebase requires a commit/preservation decision, get approval rather than bypassing dirty ownership." It also provides no blanket commit authority. The user explicitly approved the following bounded preservation plan on 2026-09-10. Operations remain pending unless recorded as executed in PROGRESS:

1. Make local logical commits for the existing owned candidate, using exact path/hunk staging and preserving the original feature tips with local backup refs. No push. Exclude all `CONTINUATION_PROMPT*.md` files and all unrelated protected dirt.
   - Foundation: one commit for the six changed native path-timing/diagnostic files in `Source/CkGroundNav/.../Path/` and `Source/CkNavigation/.../Nav/`; one documentation commit for the owned campaign ledger, guide, contracts and evidence (including this checkpoint).
   - Tests: one approved-presentation commit for the 21 files in `GATE119_PRESENTATION_ARTIFACTS.md`; one benchmark/diagnostic commit for the remaining owned map, runtime actor/config, editor fixture, module dependency, diagnostic test and Python validator changes in `GATE123_LOCAL_BENCHMARK_EVIDENCE.json`. Keep implementation, tests and required dependencies together. Review the complete staged diffs before committing.
2. Preserve the exact bytes of the three unrelated tracked files in a narrowly scoped temporary preservation operation: root `Config/DefaultGameplayTags.ini`, root `Script/Generated/CkPlugins_EntitySpawnParams.as`, and Foundation `Source/CkNavigation/Public/CkNavigation/Settings/CkNav_ProjectSettings.h`. Use path-scoped stashes only under this explicit approval, supplemented by SHA256/byte-preserving copies of these files inside the existing checkout's Saved directory. Never use a blanket stash. The settings header is mixed-line-ending dirt with no semantic diff; Git normalization must not lose those bytes. Keep the preservation records until exact restoration is verified. All unrelated untracked prompts and eight root Jolt assets remain where they are.
3. Rebase the candidate Foundation and Tests feature branches onto their refreshed origin/dev tips in place. Preserve both navigation semantics and incoming game fixes in every conflict. Rebase the selected root feature history in place after its tracked dirt is protected; reconcile root gitlinks to the integrated candidate. These are local checkpoint commits/refs only; no dev movement or publication. If untouched ownership has changed or a protected file now needs semantic conflict resolution, stop for a new ownership decision.
4. Restore the three protected files to their exact original bytes and verify all 13 unrelated hashes. The 21 presentation files may need a narrow integration update to the generated wrapper: `Script/Generated/CkTests_AutoTestActors.as` also changed upstream. Keep its new presentation class and all incoming classes, regenerate via the normal generator, and record a new identity; never replace the generated file wholesale with the old snapshot. Review coupled `Source/CkTests/CkTests.Build.cs` additions together.
5. In the existing clean BusterBlock checkout, create `feature/navigation-ab-readiness` in place and rebase its current root history onto refreshed dev while retaining the NPC voice/CPU work. Import the **integrated local feature commits only** from the two existing host plugin stores into the two existing game plugin stores (local fetch, no clone/init/copy/alternate checkout); select corresponding feature branches there. Apply paired game compatibility changes before building against the candidate. No other checkout is used.

This approval does not authorize discards, deleting build artifacts, force pushes, PRs, merges to dev, cook/package, or unrelated commits. Future implementation commits/publication remain separately scoped. Current game source and plugin pins remain unchanged until this preparation is resolved.

## Rebase review hotspots

Path overlap is a review warning, not proof that a textual conflict will occur. The committed candidate and refreshed dev both touch Foundation's uplugin, Crowd nav projection/status processors and headers, Crowd avoidance-volume processor, Jolt settings, extraction, static-world subsystem/header, subsystem implementation and world processor header. The incoming changes include same-frame SceneNode propagation, teardown admission, sparse script queries, transform follow, diagnostics gating and Jolt broadphase/contact work. Do not choose either side wholesale.

Tests has committed and dirty overlap in its generated AutoTest actor file and `Source/CkTests/CkTests.Build.cs`. The latest dev also changes the authored TestGyms level; that upstream edit does not reopen the seven user-approved gyms. Post-rebase source/binary identities must be recorded and affected gates rerun; Gate123 hashes will then be historical provenance.

## Smallest compatible implementation contract

See `GATE124_COMPATIBILITY_SOURCE_AUDIT.md` for the eight filters, callers, dimensions and source evidence. The neighboring implementation to reuse is the native NavSurface filter/area registrars and the existing MatchedBenchmarkActor readiness/reporting pattern; do not copy its fixture dimensions into game config.

1. Pin game navigation defaults explicitly to Recast and shadow Off; retain current Recast data/settings and direct sidewalk extraction. Retain the legacy UE filter classes for any direct consumers. Migrate the eight Ck mappings losslessly to neutral definitions, with the native area policies and Recast class/area registrations needed by each provider. The game constructors currently add exclusions, not custom cost overrides: preserve the underlying default area costs too.
2. Close named-filter failure paths: bad/missing configured definitions must reject the operation instead of returning the default unrestricted query. Empty filter tags retain their intentional default meaning. Required registration/configuration must be atomic; diagnoses and explicit failure branches must share a side-effect-safe validity value and remain effective if ensures compile out. Tests must exercise malformed assets/tags/areas, no downstream query and no partial publication.
3. Bridge the two dynamic game policies before admitting GroundNav: AccessZone Restricted markup and Entryway CustomerShoppingBoundary markup. Their existing `UCk_Utils_NavAreaMarkup_UE` objects write only into the Recast octree. Neutral `Request_AreaMarkup` is a separate ECS path: preserve the exact shape/transform, semantic tag, completion/failure, lifetime ownership and removal. Entryway's outside-half slab, ingress staging region, scale, padding and teardown/rearm behavior are part of the policy. An absent field or uninstalled markup makes the trial ineligible. Do not add a second unowned mirror.
4. Add per-process/run selection and an isolated authored game scenario using startup selection before the first gameplay route. Suggested interface to implement: requested provider, run ID, scenario ID and seed. These flags/scripts **do not exist yet**. Production without an explicit trial request remains Recast/off. The first scope is **Crowd route solving**; direct `utils_nav::Try_ProjectOntoNavmesh` queries and the sidewalk detector deliberately retain Recast. Label both dependencies in reports; do not claim every game navigation operation switched providers.
5. The run controller owns bounded readiness and workload admission: report requested/effective provider, provider health/settled state, field/profile identity, all eight filter identities, installed markup state, Recast backing, map/scenario/seed, agent and mesh dimensions, query/scheduler settings, source/binary/build/machine identity, completion/eligibility and rejection reason. Validate data before dispatch; never label a fallback as the requested provider. Keep package-data eligibility separate from local live-field eligibility.
6. Run the same authored workload in separate Recast -> GroundNav -> Recast processes. Every run starts from the same fixture state and records its effective provider. Trial failure exits with an explicit reason and releases run-owned state; it cannot silently enter the normal game or hang. No per-NPC provider choice or mid-route switching is promised.

Crowd's GroundNav result is installed into the shared CkNav path slot, so existing path/status consumers do not need a wholesale rewrite. External install resets the Recast-specific diagnostics, however; reports and NPC failure diagnostics must not read zero/default values as measured GroundNav projection/timing evidence. Source anchors: `CkCrowdAgent_HandleRequests_Processor.cpp:431`, `CkCrowdAgent_OnGroundNavPathResolved_Processor.cpp:354`, `CkNav_Algorithm.cpp:260` under the candidate Foundation source tree.

## Verification and remaining decisions

- Baseline: current game pins, unchanged source, Development Editor receipt, `--test-pattern=NpcAI --no-live --parallel=1 --serial-lane=*`. Exact results and diagnostics belong in PROGRESS and the baseline evidence record. The run completed 27 tests: 22 passed, 5 failed, 0 skipped/contaminated, exit 1, 3m 37s, one editor boot. See `GATE124_RECAST_BASELINE_EVIDENCE.json` for named results and identities. All five failed rows include a source-control missing-ref diagnostic; LivenessWatchdogAccrual also fails its live-queue exemption assertion. This is a red focused baseline, not full-suite or eight-filter route proof.
- After preparation/rebase: incremental Toolbox build and affected native/navigation/game checks with a freshly stated boot budget. No build while source is changing. No automatic gym rerun.
- Compatibility acceptance: all eight actual production filters resolve and enforce their exclusions/costs; bad mappings reject without executing an unrestricted route; AccessZone/Entryway lifecycle and all strict variants are exercised through real production composition; existing Recast sidewalk/queue paths retain their contract. Source-only tests do not prove this.
- A/B readiness acceptance: executable selection/reporting and explicit rejection, one authored shared game fixture, matched endpoints/settings/seed, Recast -> GroundNav -> Recast evidence. Controls ready, local behavior accepted, package accepted and performance measured are four separate statuses.
- The proposed scope retains Recast projections. A fully provider-neutral projection migration is a distinct architecture decision; do not silently include it. Dynamic area ownership is required for this compatibility slice, not optional polish.
- Build-machine work remains a documented, unlaunched runbook: DryRun/manifests then real cook/write, Development/Test execution and Shipping compile/package/provider/load checks, with exact artifact/source identity. Local tools may prepare inputs; no local cook/package. Formal performance needs at least three eligible alternating pairs on one artifact, equivalent behavior, raw samples and provider traces. It remains deferred.

## Deferred and unrelated obligations retained

- [ ] Outside-PIE editor snap on flat/ramp/overlapping floors and explicit off-surface rejection; responsiveness and wrong-floor checks.
- [ ] Editor undo/redo restores positions, routes and connections.
- [ ] LiveExtract large-scene timing, field identity/epoch, localized preview updates and undo/redo resweep.
- [ ] Real authored World Partition/data-layer load/deactivate/reactivate/replace/unload/reload with idle, installed and in-flight routes.
- [ ] Real commandlet DryRun and build-machine cook/manifests/index/tiles/all-profile selector/fingerprint/loaded-from-cook checks; no transactional disk rollback claim.
- [ ] Development/Test packaged gameplay and Shipping compile/package/provider/load checks, including appropriate production-map behavior; functional compatibility remains necessary independently of performance.
- [ ] Matched performance with at least three eligible alternating pairs, matched provenance/settings/seed, raw samples and Insights traces; unequal behavior is inconclusive.
- [ ] Later one-production-map adoption and exercised rollback; no global GroundNav opt-in now.
- [ ] Named Gate102 Recast Queue front-slot debt (240 polls / 6.08 seconds); diagnose if relevant/reproduced, never increase the timeout silently.
- [ ] Historical Gate112 eight SceneNodeTween failures, ScriptProcessor_PumpStopsAfterMarkerDrain and VisualLod_RenderBandProfileLifecycle; not asserted current without rerun and not absorbed into this task.
- [ ] Current BusterBlock NPC voice/transform-follow and CPU/lifetime/contact-churn work; historical CPU handoff is not navigation authority. Preserve all unrelated tracked/untracked/ignored work, including the 13 protected files and approved presentation slice.
