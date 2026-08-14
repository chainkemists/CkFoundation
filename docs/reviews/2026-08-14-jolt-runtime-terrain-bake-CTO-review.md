# Jolt runtime terrain bake (dynamic mesh + heightfield) — CTO Plan Review

> **Workflow:** Review the brief below, then fill in the **CTO Review Response** section at the
> bottom of this file. Commit your changes — the plan author / their assistant will pick up your
> notes from there. The plan itself is NOT restated here; it is linked and is the load-bearing
> document.

---

## Reviewer brief

### Your role

Senior reviewer / architect for the CkFoundation plugin suite. You are expected to **read code in
the repo, not just the plan in isolation** — the plan's whole value proposition is that a
weaker-model executor can follow it without design judgment, so your job is to catch what would
make that go wrong: architectural mistakes, convention/idiom mismatches, steps that are
ambiguous or unsafe, missing test coverage, and risks sized wrong. Green-light it or list
specific blockers.

### What's being built

CkJolt's static world can bake level geometry (and Landscape heightfields) into Jolt bodies, but
runtime-generated geometry on `UDynamicMeshComponent` currently produces **zero bodies silently**
(the `ExtractComponent` dispatch chain ends at Brush with no terminal else). The plan adds, in
four independently-shippable phases: (1) an explicit DynamicMesh tri-mesh dispatch branch plus a
loud terminal; (2) an updatable heightfield shape core (deformation envelope, pure region-plan
function, expand-and-overlay `SetHeights`); (3) a public `JoltHeightField` feature (typesafe
handle, synchronous bake/remove, **deferred** region-update request drained by a processor);
(4) validation/docs. Scope is framework-only (CkFoundation code + CkTests tests); no game-side
consumers.

This is a **planning-package review** (ck-plan-handoff style): the plan will be executed by an
Opus/Sonnet-class session with zero memory of the planning session, in a DIFFERENT checkout
(`D:\Repo\Venus`) that has **no UnrealToolbox** — builds via the AngelScript-fork UBT directly,
tests via headless `UnrealEditor-Cmd`.

### Environment

- UnrealEngine-Angelscript **5.7.4** (Hazelight fork), EnTT 3.16.0, vendored **Jolt 5.2.1**.
- `GeometryFramework` was verified to live at `Engine/Source/Runtime/GeometryFramework` in this
  engine tree (an engine Runtime *module*, not a plugin) — this materially changed one decision
  (see locked decision 2).

### Plan location

Start at [PROMPT.md](../campaigns/jolt-runtime-terrain-bake/PROMPT.md) (problem, decision record
D1–D6, file inventory, verification story), then the phase files in the same directory:
[PHASE_1](../campaigns/jolt-runtime-terrain-bake/PHASE_1.md) ·
[PHASE_2](../campaigns/jolt-runtime-terrain-bake/PHASE_2.md) ·
[PHASE_3](../campaigns/jolt-runtime-terrain-bake/PHASE_3.md) ·
[VALIDATION](../campaigns/jolt-runtime-terrain-bake/VALIDATION.md) ·
[PROGRESS](../campaigns/jolt-runtime-terrain-bake/PROGRESS.md).

### Critical context — read before reviewing

- `Source/CkJolt/Claude.md` — the static-world contract, the ECS-first attribution entity, the
  processor `RunAfter FProcessor_JoltWorld_WaitForAsync` edge rule, the bake-filter/policy split.
- Root `CLAUDE.md` — `CK_ENSURE_IF_NOT` discipline (recovery in the ensure body), the
  request-completion delegate contract, "no silent fallbacks", comment audit.
- `Source/CLAUDE.md` — tier table (CkJolt is T4), `Request_*` takes the request STRUCT rule.
- Sibling code the new work is supposed to **mirror** (the plan cites these as molds):
  - `Source/CkJolt/Public/CkJolt/StaticWorld/CkJoltStaticWorld_Subsystem.cpp` —
    `Request_BakeComponent` (replace-on-rebake + `_ManualComponentEntities` tracking) and
    `DoCreate_BodiesFromExtracted` (body creation + user-data stamping).
  - `Source/CkJolt/Public/CkJolt/StaticWorld/CkJoltBakeExtraction.cpp` — the dispatch chain, the
    SplineMesh cache-bypass precedent, `CreateHeightFieldShape` row-flip math.
  - `Source/CkJolt/Body/CkJoltBody_Processor.cpp` — the HandleRequests processor mold (group +
    RunAfter edge).
  - CkTests `Source/CkTests/Private/UnitTests/CkJolt/Test_JoltBake_HeightField_KnownHeightsZUp.cpp`
    and `Test_JoltBody_Lifecycle.spec.cpp` — the two test harness molds.

### Design decisions already locked in (do NOT relitigate unless you see a real problem)

1. **D1:** both capabilities ship; tri-mesh dispatch first (small, independently shippable),
   heightfield surface second — the heightfield is the only path delivering cheap region updates.
2. **D2:** explicit `UDynamicMeshComponent` branch with a `GeometryFramework` engine-module dep;
   the generic "any primitive with a `GetBodySetup()`" terminal branch was killed (unaudited
   level-sweep blast radius; quiet absorption of unknown component semantics). The branch
   bypasses the guid-keyed shape cache (fresh `BodySetupGuid` per recook — verified in engine
   source).
3. **D2b:** terminal-else loudness is policy-split — `CK_TRIGGER_ENSURE` under `ExplicitActor`,
   Verbose log under `LevelSweep`. Accepted consequence: CkUnrealComponent `Automatic` hosting an
   unsupported-class primitive will newly ensure (judged correct-loud; opt-outs exist; rollback
   named).
4. **D3:** heightfield handle = attribution entity carrying BOTH the existing
   `FFragment_JoltStaticActor_Current` (teardown/attribution unchanged) and new heightfield
   fragments. Bake/remove synchronous (mirrors `Request_BakeComponent`); **UpdateRegion deferred**
   via ECS request + processor with the WaitForAsync edge (thread-safety vs the async physics
   step — `SetHeights` is documented unsafe against in-flight queries). Block alignment =
   expand-and-overlay via `GetHeights`, never reject. Envelope validated loudly by us (Jolt's
   `SetHeights` **silently clamps** out-of-range values); `NotifyShapeChanged` after every edit.
   Optional `_SourceComponent` keys replace-on-rebake + a reciprocal one-representation-per-surface
   guard against `Request_BakeComponent`.
5. **D4:** vertical geometry (overhangs/walls) is a documented heightfield limitation pushed to
   the tri-mesh path — NO auto-generated companion bodies (approximated collision is banned in
   this module).
6. **D5:** async-cook ordering is the caller's contract; the loud ensure stays; no retry
   machinery (a bake after an async-cook edit reads the stale previous BodySetup, undetectably —
   verified in engine source, `bUseAsyncCooking` defaults false).
7. **D6:** Chaos coexistence is a confirmed non-conflict (XOR rule governs JoltBody composition,
   not static bakes); the plan fences the executor from "fixing" it.

### What I specifically want you to scrutinize

#### A. Architecture / decomposition

- Reusing the `JoltStaticActor` attribution fragment on the heightfield entity (D3) vs a fully
  separate feature: does piggybacking on `Request_RemoveBodiesForEntity` /
  `FProcessor_JoltStaticActor_EndPlay` create a coupling you'd regret (e.g. when static-world
  Phase-2 channel queries or cooked-data paths evolve)?
- The synchronous-bake / deferred-update split (D3): is it defensible that `Request_BakeHeightField`
  mutates the Jolt world synchronously (as `Request_BakeComponent` already does) while only
  UpdateRegion defers? Or should bake defer too for step-safety symmetry? (The plan's position:
  body add/remove already happens synchronously all over the existing subsystem; only the
  in-place shape mutation is newly hazardous.)
- Is `FCk_Fragment_JoltHeightField_ParamsData` carrying `TArray<float> _WorldHeights` by value
  through a UFUNCTION acceptable at expected grid sizes, or does the plan need a
  move/shared-storage note for large grids (e.g. 513×513 ≈ 1 MB per bake call, plus BP/AS copy
  semantics)?

#### B. Convention compliance

- Do the pre-designed structs/UFUNCTION signatures in PHASE_3 match house style exactly
  (CK_PROPERTY vs CK_PROPERTY_GET choices, CK_DEFINE_CONSTRUCTORS essentials, request-struct
  rule, delegate-last with AutoCreateRefTerm and no C++ default, concrete return type on its own
  line)?
- PHASE_2 deliberately makes `ApplyHeightFieldRegionUpdate` / `ComputeHeightFieldRegionPlan`
  report rather than ensure, with the loud boundary in the Phase-3 processor/utils (argued as the
  Cast/CastChecked split). Does that read as compliant with non-negotiable #3 to you, or should
  the helpers ensure directly?
- The plan tells the executor to mimic `DoCreate_ComponentEntity` — which currently uses
  `ck::StaticCast<FCk_Handle_JoltStaticActor>` internally, while the root doctrine bans
  `ck::StaticCast` at call sites (subsystem-internal composition is arguably framework plumbing).
  Should the plan instead direct the new code to use `CastChecked` and leave the existing site
  alone, or follow the mold verbatim?

#### C. Version-specific API specifics

- Engine claims were verified against the local 5.7 fork: `UDynamicMeshComponent::GetBodySetup()`
  const overload (no create-on-demand), `RebuildPhysicsData` guid churn + async-queue swap,
  `SetComplexAsSimpleCollisionEnabled`, `ADynamicMeshActor`. Anything you know of in the 5.7.x
  tail (or the Venus checkout's engine) that invalidates these?
- Vendored Jolt 5.2.1 `HeightFieldShape::SetHeights/GetHeights` block-alignment + silent clamp +
  `mMinHeightValue/mMaxHeightValue` were verified. The odd-N sample-count rounding behavior is
  the one Jolt claim the plan pins by test rather than by reading (PHASE_2 risk) — acceptable?

#### D. Test coverage

- 9 new tests across 3 phases (3 dispatch, 4 shape-core, 2 world-level spec), red-first, plus the
  existing `Ck.Jolt.Bake.HeightField.KnownHeightsZUp` as the refactor pin. Gaps you'd insist on
  (e.g. a re-bake-replaces-bodies count test for heightfields, AS-side execution coverage,
  multi-update-same-frame batching)?
- "No regressions" is defined as: session-1 recorded baseline of the target checkout's `Ck.Jolt`
  suite (23 tests on the planning checkout), same verdicts + only the 9 new greens. Sufficient,
  or do you want the wider `Ck.` suite gated too?

#### E. Risks called out — sized correctly?

- D2b's new ensure firing in existing content that explicitly bakes unsupported-class primitives
  (one-line rollback named). Correct-loud, or too hot for a framework used by live games?
- The row-flip-composed-wrong risk (mirrored region updates) is called the highest risk and is
  pinned by asymmetric-surface tests with off-center rects. Convinced?
- PHASE_3 flags `AddExpectedError` × `CK_ENSURE` fire-once interplay as unverified. Is there a
  known house pattern the plan should cite instead of "mimic the existing ensure-expecting test"?

#### F. Forward-compat with downstream / deferred work

- Does the heightfield surface's shape leave room for: cooked/persisted heightfields later (it is
  runtime-only by requirement — is anything here painting that door shut?), the Phase-2
  channel-filtered query API, save/load (v3 rebuild+hydrate) if a game ever wants craters
  persisted, and multiplayer (the update request is local-machine; is that stated loudly enough)?
- The plan deliberately does NOT extend `UShapeComponent` extraction despite discovering shape
  components silently don't bake today (out of scope; would change cooked hashes). Should that be
  spun off as a tracked follow-up rather than a fence-note only?

### Output format — fill in the CTO Review Response section below

Be direct. If the plan is good, say so and green-light it — don't manufacture issues to look
thorough. Specific blockers tied to a phase/step/file, not vague concerns.

---

## CTO Review Response

### Verdict

`GREEN-LIGHT` — flipped from `CHANGES REQUESTED` at re-review (see addendum below; original
verdict text preserved for the record).

Original verdict: `CHANGES REQUESTED` — the architecture is sound and both decisions the author
flagged for scrutiny (D3 attribution reuse; sync-bake / deferred-update split) are **signed off
as designed**. The four blockers are all small plan-text defects, dangerous only because the
executor profile is zero-context and forbidden to redesign: each one either fails to compile as
pre-designed, or specifies a behavior a literal executor would ship as a bug. All four are
~minutes of plan edits; no re-review needed once applied.

### Re-review addendum (2026-08-14, same reviewer)

Verified the implementer's plan edits against every sign-off condition:

- **Blocker 1 (stale-entry liveness):** applied — the "Stale-entry rule" block
  (PHASE_3.md § subsystem additions), LIVE-qualified guards in BOTH directions of the reciprocal
  check, and the "each behind a liveness check" exit criterion. The pinning test case from
  sign-off condition 1 was missing; **added by the reviewer during re-review** (one spec-comment
  step at the end of `CrossRepresentationGuard`: destroy the attribution entity directly →
  re-bake the component → succeeds, no expected error).
- **Blocker 2 (Deinitialize reachability):** applied — `_HeightFieldEntities` flat drain list
  alongside the keyed `_ManualHeightFields`, the Deinitialize paragraph extending the existing
  drain in-style, and the deinit-reachability exit criterion.
- **Blocker 3 (macro placement):** applied — `CK_REQUEST_DEFINE_DEBUG_NAME` moved inside the
  struct after `CK_GENERATED_BODY` with a why-comment. (The `public:` specifier before
  `CK_GENERATED_BODY` was not added; USTRUCT default access is public, so this is cosmetic
  mold-mismatch only — not held against the green light.)
- **Blocker 4 (cast fence):** applied — and correctly adapted: the create path converts to the
  NEW feature's handle, so `UCk_Utils_JoltHeightField_UE::CastChecked` (not JoltStaticActor's) is
  the right target; existing `StaticCast` sites fenced as leave-untouched.
- **Non-blocking 1 and 2 also applied** (fire-once risk rewritten as resolved with the
  `Occurrences=-1` pattern in PHASE_3 + VALIDATION; D2b re-ranked with the QueryOnly-trigger
  vector in PROMPT + PHASE_1, `UShapeComponent` extraction spun off as a tracked follow-up
  outside the campaign). Non-blocking 3–6 (`_BodyId` invalid-constant default, re-bake-replaces +
  same-frame-batch spec additions, `_WorldHeights` copy note, local-machine doc sentence) were
  not applied — they remain optional; none gates execution.

**GREEN-LIGHT. The package is ready for the executor.**

### Blocking issues

1. **PHASE_3 § subsystem additions — the reciprocal guard and the replace path must tolerate
   stale map entries.** Entity-side destruction (the `FProcessor_JoltStaticActor_EndPlay` funnel)
   does NOT remove `_ManualComponentEntities` / `_ManualHeightFields` entries; the existing
   `Request_BakeComponent` replace path guards with `ck::IsValid(*ExistingEntity)`
   (`CkJoltStaticWorld_Subsystem.cpp:311-322`) precisely for this. As specified — "if
   `_SourceComponent` set and present in `_ManualComponentEntities` → CK_ENSURE" — destroying a
   heightfield's handle entity directly and then tri-mesh-baking the same component false-fires
   the reciprocal ensure (and mirror-image for the other direction). Spec the guard as: entry
   found AND `ck::IsValid(entity)` → ensure; found-but-dead → prune the entry and proceed. Pin it:
   extend `CrossRepresentationGuard` with — destroy the heightfield's entity via
   `Request_DestroyEntity`, tick, `Request_BakeComponent` the same component → succeeds, no
   expected error.

2. **PHASE_3 — subsystem `Deinitialize` teardown is unspecified.** `Deinitialize`
   (`CkJoltStaticWorld_Subsystem.cpp:90-143`) drains `_LevelBodies` + `_ManualActorEntities` +
   `_ManualComponentEntities` (remove bodies + destroy attribution entities "while the Jolt world
   and the ECS registry are both still alive") and `Empty()`s each container. The plan adds
   `_ManualHeightFields` but never says to mirror that drain — and **handle-only (unkeyed)
   heightfields are in NO container at all**, so their bodies and entities are unreachable at
   Deinitialize, breaking the module's free-everything-remaining invariant. Fix: track every
   heightfield attribution entity for teardown (e.g. a flat
   `TArray<FCk_Handle_JoltHeightField> _HeightFieldEntities`; `_ManualHeightFields` stays the
   replace/reciprocal key only) and add both to the Deinitialize drain + `Empty()` list.

3. **PHASE_3 § pre-designed shapes — `CK_REQUEST_DEFINE_DEBUG_NAME` placed outside the struct
   will not compile.** The macro expands to a `protected:` member-function override
   (`CkEcs/Request/CkRequest_Data.h:135-137`); PHASE_3.md places it after the closing `};`. Move
   it inside the struct, right after `CK_GENERATED_BODY`, and add the `public:` access specifier
   before `CK_GENERATED_BODY` — match `FCk_Request_JoltBody_Teleport`
   (`CkJoltBody_Fragment_Data.h:446-474`) exactly. Under the plan's own stop-don't-improvise rule
   this typo costs the executor a blocker-stop.

4. **PHASE_3 § fences — the cast instruction self-contradicts.** "Do NOT use `ck::StaticCast`…
   Inside the subsystem's create path, mirror whatever `DoCreate_ComponentEntity` does in the
   TARGET checkout" — but the mold uses `ck::StaticCast<FCk_Handle_JoltStaticActor>`
   (`CkJoltStaticWorld_Subsystem.cpp:732, 754`), so a literal executor can obey either sentence.
   Replace with an unambiguous directive: new code uses
   `UCk_Utils_JoltStaticActor_UE::CastChecked` (the feature was just composed — the guaranteed
   case; `CK_DEFINE_CPP_CASTCHECKED_TYPESAFE` is on the utils, `CkJoltStaticActor_Utils.h:25`);
   leave the two existing `StaticCast` sites untouched.

### Non-blocking suggestions

1. **The PHASE_3 `AddExpectedError` × fire-once risk is resolved in-code — replace the risk
   paragraph with the answer.** Repeat suppression is explicitly DISABLED under automation:
   `CkEnsure.cpp:212-217` (`NOT Record.IsFirstOccurrence && NOT GIsAutomationTesting` — the
   comment names AddExpectedError as the reason). The house pattern, used by 10+ CkJolt tests, is
   `AddExpectedError(TEXT("substr"), EAutomationExpectedErrorFlags::Contains, /*Occurrences=*/-1)`.
   Drop the "make each format string include a distinct value" fallback — its premise is wrong
   (the latch signature is per-site, not per-message, and it is inert during automation anyway).

2. **D2b sizing refinement.** The likeliest real-content fire is not CkUnrealComponent
   `Automatic` but `Request_BakeActor` on an actor carrying a collision-ENABLED shape component:
   `Get_ComponentSkipReason` rejects only `NoCollision` (`CkJoltBakeExtraction.cpp:183`), so
   QueryOnly trigger spheres/capsules — common on gameplay actors — reach the terminal else under
   ExplicitActor while the actor's supported components bake fine. Still correct-loud (one fire
   per site per session outside automation; rollback stands), but per brief §F2: **yes, spin the
   UShapeComponent-extraction gap off as a tracked follow-up** rather than a fence-note — the new
   loudness will generate user pressure toward exactly that feature.

3. **`_BodyId = 0` default in `FFragment_JoltHeightField_Current` is a false sentinel.** 0 is a
   legal Jolt body id (index 0, sequence 0); Jolt's invalid is `JPH::BodyID::cInvalidBodyID`
   (0xffffffff). The fragment header may include Jolt — default to the invalid constant.

4. **Feature-spec additions (cheap, pin real contracts):** (a) re-bake-replaces for keyed
   heightfields — bake with `_SourceComponent`, re-bake, EXPECT count stays baseline+1 and the
   ray reads the new heights (pins the `_ManualHeightFields` remove+destroy dance); (b) two
   disjoint `Request_UpdateRegion` in one frame → both applied (pins the natural per-frame
   batching claim).

5. **`TArray<float> _WorldHeights` by value: acceptable, note it.** The UFUNCTION takes the
   params/request by `const&`, so C++ and AS don't copy; BP copies at the node boundary as BP
   always does (~1 MB at 513², bake-frequency, not per-frame). One doc-comment sentence on the
   params struct suffices; no API surgery.

6. **Multiplayer loudness (brief §F):** add one sentence to `Request_UpdateRegion`'s doc comment
   — the edit and its completion are LOCAL-machine; nothing replicates. The general completion
   contract says this, but this is a surface where a consumer will assume terrain deformation
   replicates.

7. **PHASE_2 report-not-ensure helpers: compliant, no change.** Same shape as the Cast/CastChecked
   split — the only callers (Phase-3 processor + bake) diagnose loudly, PHASE_2's tests probe
   rejection without expected-error scaffolding, and the creation-side public boundary DOES
   ensure (envelope `Min < Max`, sample-count). Non-negotiable #3 is satisfied at the boundary
   that owns the caller relationship.

8. **Bake/Remove without completion delegates: accepted as fenced.** Strictly,
   `Request_RemoveHeightField` is a handle-taking immediate mutator, which the completion
   contract's synchronous-fire clause covers — but consistency with the delegate-less
   `Request_BakeActor/Component/RemoveComponent` surface is the better call here, and the fence
   argues it explicitly. No change.

### Convention compliance spot-checks performed

Live-codebase files opened during this review (planning-checkout tree, CkFoundation @ dev):

- `Source/CkJolt/Public/CkJolt/StaticWorld/CkJoltBakeExtraction.cpp` — full `ExtractComponent`
  dispatch chain (confirmed: ends at Brush with no terminal else, :839-863), skip gate
  (`Get_ComponentSkipReason` :167-208), `CreateHeightFieldShape` row-flip/offset/scale
  (:639-683), SplineMesh cache-bypass precedent (:733-747), `EmitBody`/public-member
  `FCk_Jolt_ExtractedBody` precedent.
- `Source/CkJolt/Public/CkJolt/StaticWorld/CkJoltStaticWorld_Subsystem.cpp` —
  `Request_BakeComponent` replace-on-rebake + liveness guard (:287-336), `Request_RemoveComponent`
  / `Request_RemoveBodiesForEntity` idempotent funnel (:338-389), `Deinitialize` drain (:90-143),
  `DoCreate_ComponentEntity`'s `ck::StaticCast` (:732, :754); `_Manual*Entities` map types
  (`CkJoltStaticWorld_Subsystem.h:239-241`).
- `Source/CkJolt/Public/CkJolt/StaticWorld/CkJoltStaticActor_Fragment.h` — fragment shape +
  friend list; `CkJoltStaticActor_Utils.h` — `CK_DEFINE_CPP_CASTCHECKED_TYPESAFE` present.
- `Source/CkJolt/Public/CkJolt/Body/CkJoltBody_Processor.h/.cpp` — the HandleRequests mold:
  `RunAfter TDepList<…, FProcessor_JoltWorld_WaitForAsync>` (:101-102), `CK_REGISTER_PROCESSOR`
  block, `ForEachRequest` + `MakeCompletionGuard` drain (:426-432), and the
  `FProcessor_JoltBody_CancelPendingRequests` teardown twin the plan tells the executor to find.
- `Source/CkJolt/Public/CkJolt/World/CkJoltWorld.h` — `_TempAllocator` is private with no
  accessor (:198); the pre-approved one-liner is genuinely needed.
- `Source/CkJolt/Public/CkJolt/Body/CkJoltBody_Fragment_Data.h` — `FCk_Request_JoltBody_Teleport`
  as the request-struct mold (macro placement, property split, constructors).
- Vendored Jolt 5.2.1 `HeightFieldShape.h` — silent clamp on `SetHeights` (:224), block-aligned
  rect contract (:209-228), `GetMinHeightValue/GetMaxHeightValue` (:203-205), `cNoCollisionValue`
  (:23), `mMinHeightValue/mMaxHeightValue` (:76-79), `GetSampleCount` rounds up to a block
  multiple (:131); `BodyInterface.h` — `NotifyShapeChanged` (:174).
- `Source/CkCore/Public/CkCore/Ensure/CkEnsure.h/.cpp` — `CK_TRIGGER_ENSURE` exists (:91), the
  automation-mode repeat-suppression bypass (:212-217).
- CkTests `Source/CkTests/Private/UnitTests/CkJolt/` — all three cited molds exist on disk; the
  `AddExpectedError(…, Contains, -1)` pattern across BoxConvexRadius / Lifecycle /
  OwnershipExclusivity / BoxOccupancy / GeometryParitySampler.
- `Source/CkJolt/Claude.md`, root `CLAUDE.md`, `Source/CLAUDE.md` — static-world contract,
  WaitForAsync edge rule, ensure/request/comment doctrine.

### Design / architecture observations

- **D3 attribution-fragment reuse: signed off** (the brief's question A1). The teardown funnel is
  fragment-scoped, not source-scoped — `Request_RemoveBodiesForEntity` checks only
  `Has<FFragment_JoltStaticActor_Current>` and drains `_BodyIds` (verified :376-389) — so the
  heightfield entity rides it with zero changes. Component bakes already set the "attribution
  entity without a meaningful source actor" precedent, and `Get_SourceActor` is already
  documented may-be-null. Channel-query forward-compat falls out for free: the heightfield body's
  signature comes from `TryDerive_SignatureFromProfile` like every other body, so the Phase-2
  channel-filtered query API will see it with no special-casing. The alternative (a fully
  separate feature with its own removal funnel + EndPlay processor) duplicates the idempotence
  machinery for no isolation gain and forfeits free ray-hit attribution. The residual coupling —
  future code iterating JoltStaticActor entities and assuming an actor — is already broken by
  component bakes today, not newly by this.
- **Sync-bake / deferred-UpdateRegion split: signed off** (question A2). Synchronous body
  add/remove mid-frame is the module's existing, exercised contract
  (`Request_BakeComponent`/`DoBatchAdd_Bodies`); the only newly-hazardous operation is the
  in-place `SetHeights` (documented race vs parallel queries, `HeightFieldShape.h:219`), and
  deferring exactly that behind the `WaitForAsync` edge is the established house pattern (the
  JoltBody mold carries the same edge for the same reason). Deferring bake too would buy no
  additional safety and would cost the synchronous handle return that mirrors the neighboring
  surface. Whether synchronous body-add is fully safe against an in-flight async step is a
  pre-existing module-wide question — inherited, not introduced.
- **The PHASE_2 grid mapping is correct.** I re-derived it against the creation code
  (`r = N-1-y`, `-(N-1)*sy` offset, `mScale.Y = 1` — `CkJoltBakeExtraction.cpp:654-671`) and
  hand-checked all four RegionPlan test expectations — full surface, (2,2,2,2)→(2,4,2,2),
  (1,1,3,3)→(0,4,4,4), odd-N aligned top edge 10 — all land. The asymmetric `h(x,y) = 10x + 100y`
  surface plus the off-center rect genuinely distinguishes the double-flip/zero-flip failure mode;
  the highest-risk call is well pinned.
- **Every vendored-Jolt claim the plan makes checked out verbatim** (silent clamp, block
  alignment, envelope settings, no-collision sentinel, sample-count rounding-up,
  `NotifyShapeChanged` signature incl. the prev-COM + activation params PHASE_3's processor step
  names). The one deliberately test-pinned claim (odd-N padding-row `SetHeights` behavior) is the
  right thing to pin by test — the header does not document it.
- **Engine-side 5.7 claims were NOT independently re-verified by this review**
  (`GetBodySetup()` const overload, `BodySetupGuid` churn, async-cook queue swap): the planner
  verified them against the engine tree and the phase entry criteria force the executor to
  re-verify against the target engine — that covers the Venus drift risk. Inference, flagged as
  such.
- **Verification story: right-shaped for a no-toolbox checkout.** Recorded session-1 baseline,
  report parsing over exit codes, the Ck.Jolt-scoped no-regression definition — honest and
  sufficient, since nothing outside CkJolt consumes the new symbols and the plan explicitly
  forbids claiming wider than measured. The 9-test set plus the KnownHeightsZUp refactor pin is
  good coverage once non-blocking #4's two additions land.
- **D1/D2/D2b/D4/D5/D6 all stand as argued.** D2's cache-bypass reasoning matches the in-file
  SplineMesh precedent; D4's no-companion-bodies stance is consistent with the module's
  no-approximated-collision doctrine (`MakeScaleValid` rejection); D6's fence correctly heads off
  the one scope-creep an executor would be tempted into.

### Sign-off conditions

The four blocking items, all plan-text edits to PHASE_3.md (+ one test-case line in its spec):

1. Reciprocal-guard/replace spec gains the liveness rule (found-but-dead entry → prune, proceed)
   + the stale-entry test case in `CrossRepresentationGuard`.
2. Deinitialize drain specified: all heightfield attribution entities tracked (flat array) and
   drained + `Empty()`'d alongside the existing three containers.
3. `CK_REQUEST_DEFINE_DEBUG_NAME` moved inside the request struct after `CK_GENERATED_BODY`,
   with the `public:` specifier, matching `FCk_Request_JoltBody_Teleport`.
4. The cast fence reworded to: use `UCk_Utils_JoltStaticActor_UE::CastChecked` in the new create
   path; do not copy the mold's `ck::StaticCast`; leave the existing sites alone.

Apply as written → GREEN-LIGHT, no re-review needed.

---

### Reviewer

- **Name:** Claude (Fable 5) — senior-reviewer session for Adam
- **Date:** 2026-08-14
