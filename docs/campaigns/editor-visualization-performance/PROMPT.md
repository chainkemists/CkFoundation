# Editor Visualization Performance Campaign

## Freshness

- Started: 2026-08-04
- Baseline CkFoundation HEAD: `7598146517eb911ac148352d2970ed7ba4e1a4d6`
- Death condition: replace this prompt if the ECS scheduler contract, editor-selection-owner contract, CkPmg retained-shape contract, or CkIsmRenderer proxy contract changes materially.

## Mission

Make a stationary non-PIE editor world cheap. Editor visualization must be retained and event-driven, not reconstructed by processors every frame. Common high-count visualizers must have a shared CkIsm-backed path, including transform axes, while lower-count rich shapes may use CkPmg.

## Success criteria

1. Transform and probe previews remain visible while stationary without executing per-entity draw submission every frame.
2. Selection, transform, descriptor, visibility, and lifetime changes update exactly the affected visuals.
3. `FProcessor_Transform_HandleRequests` and other custom wrappers do negligible work when their dirty/request views are empty, without changing deferred request ordering or completion semantics.
4. A reusable visualizer feature supports transform axes, box, sphere, capsule, and cylinder descriptors with CkPmg and CkIsm backends selected by explicit policy.
5. CkIsm visualizers batch thousands of non-clickable common visuals across owners; editor selection of the source is a visibility policy, not an ISM batch key.
6. Existing transform, probe, UnrealComponent, PMG, ISM/ISKM, state-machine, and teardown behavior remains green.
7. Performance claims use at least three matched before and after editor traces per workload and report scope self time, inclusive time, invocation count, and associated work counters.

## Locked constraints

- `CkEcsExt` cannot depend on CkPmg or CkIsmRenderer because both already depend on CkEcsExt.
- The renderer-composition layer belongs above CkEcsExt, CkPmg, and CkIsmRenderer. The working module name is `CkEntityVisualizer`.
- Persistent UE debug lines are not an entity-owned renderer: they lack safe per-entity update/removal and global flush is not acceptable.
- CkPmg visuals use durable mesh sections and dirty transform propagation. Persistent visuals must not enter duration-expiry work.
- CkIsm batching must be world-correct and must not key normal debug instances by selection owner.
- Ownerless editor entities retain an explicit compatibility policy.
- Request handling remains deferred, copy-then-reset, re-entrancy-safe, and exactly once.
- Existing unrelated dirty files are out of scope and must be preserved.

## Non-goals

- Making pooled ISM instances individually clickable. Source selection drives visibility; a clickable proxy is a separate low-count concern.
- Transform-dirty-gating time-driven skeletal animation.
- Claiming a performance win from a single frame or screenshot.
- Broad scheduler redesign before local counters prove it is necessary.

## Prior evidence

- `20260804_135003_Frame502.json`: one 90.372 ms frame; transform preview self time 23.915 ms over 2,890 entities; sphere preview self time 6.793 ms over 91 entities.
- Follow-up screenshot: scheduler 14.8 ms inclusive, transform preview 14.8 ms, transform requests 3.20 ms, ISKM crowd 3.02 ms, ISM proxy transform 3.00 ms, UnrealComponent push 2.46 ms, state-machine first sync 1.96 ms. It lacks self/count metadata and is diagnostic only.
- Existing selection-filter and compare-before-write edits are useful but do not satisfy the retained/event-driven mission.

## Reading list

- `Source/CLAUDE.md`
- `Source/EDITOR_MODULES.md`
- `Source/CkEcsExt/Claude.md`
- `Source/CkPmg/Claude.md`
- `Source/CkIsmRenderer/Claude.md`
- `Source/CkSpatialQuery/Claude.md`
- `docs/campaigns/iskm-editor-preview/PROMPT.md`
- `docs/campaigns/iskm-editor-preview/PROGRESS.md`

## Things ruled out

- Keeping `DrawDebug*` with `Duration=0` and only filtering selection: stationary selected entities still redraw every frame.
- Setting a long debug-line duration: movement duplicates geometry and teardown cannot remove one entity's primitives safely.
- Putting CkPmg/CkIsm calls in CkEcsExt: circular dependency.
- Treating CkPmg as the thousands-scale batching answer: it is retained and dirty-driven, but each shape owns procedural mesh state.
- Splitting pooled ISM renderer components by selection owner: it destroys cross-owner batching.
