# Plan and Gates

## Gate 0 - Baseline and mechanism proof

Entry: current dirty boundary recorded; prior build and focused-test logs available.

Work:

- Preserve the prior Frame502 trace and follow-up screenshot as anecdotal red baselines.
- Add nested scopes and work counters to distinguish wrapper dispatch, entity visits, requests, renderer submissions, and component writes.
- Capture matched idle and active editor workloads before behavior changes.

Exit: each named hotspot has a measured work discriminator; no optimization is justified only by inclusive parent time.

## Gate 1 - Empty custom-processor work

Entry: counters identify empty wrappers versus active producers.

Work:

- Add a side-effect-safe empty-storage early-out to transform request handling.
- Preserve copy-then-reset, cancellation, request completion, and re-entrant next-pass behavior.
- Apply the same pattern to ISM proxy transform only if its dirty view is proven empty while wrapper cost remains material.
- Do not dirty-gate time-driven ISKM crowd advance; add explicit editor-preview activity policy only with behavior coverage.

Exit: focused request/interpolation/destruction tests pass; idle traces show no request scan or cleanup work.

## Gate 2 - Visualizer foundation and CkPmg backend

Entry: module placement and dependency direction are verified.

Work:

- Add `CkEntityVisualizer` as a runtime composition module with editor-only integration processors.
- Define public descriptors and lifecycle/visibility/backend policy without exposing backend fragments to callers.
- Implement durable CkPmg children attached through SceneNode.
- Make finite-duration PMG ticking tag-gated so persistent visualizer shapes never enter it.
- Add event-driven editor selection invalidation.

Exit: setup, move, descriptor change, selection toggle, and teardown update exactly affected PMG visuals; stationary visuals require no redraw pass.

## Gate 3 - Transform and probe migration

Entry: CkPmg backend lifecycle is green.

Work:

- Move transform preview ownership out of CkEcsExt's direct debug-draw processor.
- Represent transform axes as three durable arrow children; do not use the stock pivot copy-only setup.
- Map probe box/sphere/capsule/cylinder descriptors from CkSpatialQuery.
- Retire the four zero-duration probe DrawDebug processors.

Exit: visual parity and compatibility policies are proven in editor gyms; old per-frame preview scopes no longer execute.

## Gate 4 - Shared CkIsm backend

Entry: source descriptors and lifecycle are backend-independent.

Work:

- Add shared transform-axis and common-shape meshes/material behavior suitable for custom instance data.
- Batch by world and render descriptor, never by selection owner.
- Verify transient-factory multi-world semantics or own an explicitly world-keyed visualizer cache.
- Add explicit backend policy and an evidence-based Auto threshold.

Exit: deterministic 1,000 and 10,000 visualizer stations have exact live counts, correct dirty updates, zero teardown orphans, and visibly correct real-RHI output.

## Gate 5 - Remaining editor hotspots

Entry: retained visuals are no longer dominating the frame.

Work:

- Diagnose ISKM crowd advance by controller/activity counts and animation versus cosmetic sub-scopes.
- Diagnose ISM proxy transform by dirty entity and touched-component counts.
- Diagnose state-machine first-sync as one-frame admission work, not stationary churn.
- Finish UnrealComponent owner-to-component invalidation only if compare-only visits remain material.

Exit: every remaining material scope has either a justified event/dirty gate or evidence that it represents active required work.

## Gate 6 - Verification and review

Entry: implementation and focused correctness gates are green.

Work:

- Build through detached UnrealToolbox with the explicit project path.
- Run focused automation and inspect fresh logs for ensures and script errors.
- Run real-RHI visual captures and matched N>=3 before/after traces for off, selection 1, selection 10, preview-all, 1k, and 10k workloads.
- Perform adversarial architecture, lifecycle, multi-world, and performance review.

Exit: success criteria in `PROMPT.md` are evidence-backed; unresolved risks are explicit and do not get reported as complete.
