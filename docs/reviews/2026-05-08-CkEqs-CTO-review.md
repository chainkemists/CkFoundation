# CkEqs — CTO Review

> **Workflow:** Review the brief below, then fill in the **CTO Review Response** section at the bottom of this file. Commit your changes — the plan author / their assistant will pick up your notes from there.

---

## Reviewer brief

You are reviewing an implementation prompt for a **new CkFoundation module, `CkEqs`** — an ECS-native Environmental Query System. It generates spatial candidate sets (locations or entity handles), runs filter/score tests over them, and writes results to ECS fragments for consumers (notably GOAP) to read. **It is not** a wrapper around UE's `UEnvQueryManager` — it is a from-scratch, EnTT-backed reimplementation that uses UE EQS source as a *math reference only* (`NormalizeItemScores`, generator distribution math, etc.).

The prompt has already been through one Tech-Director pass earlier in the same session — what you're reviewing is the revised v2, not a first draft. Your job is to verify the v2 lands.

### Pre-flight: name-collision call-out

There is **already** a class named `UCk_Utils_Eqs_UE` in `Plugins/CkFoundation/Source/CkAi/Public/CkAi/EQS/CkEqs_Utils.h` — a thin BP wrapper around `UEnvQueryInstanceBlueprintWrapper`. The new plan declares a class with the **same name** in a new `CkEqs` module. UE/UHT will not allow two `UCLASS`-marked classes to share a name even across modules.

This is a v1 blocker. Possible resolutions (please pick one in your review):

1. Rename the existing CkAi class (e.g. `UCk_Utils_UeEqs_UE` to clarify it wraps UE-native EnvQuery).
2. Rename the new class (e.g. `UCk_Utils_EqsQuery_UE`).
3. Move/fold the existing CkAi helper into the new `CkEqs` module as a deprecation path — replacing UE-EQS callers with the ECS-native pipeline over time.

The plan currently does not address this. Please call it explicitly so the senior programmer doesn't discover it at compile time.

### Your role

Senior reviewer / architect. You're the last set of eyes before implementation begins. Specifically:

1. Catch architectural issues that would be expensive to fix mid-implementation.
2. Catch convention/idiom mismatches against the existing CkFoundation codebase that would cause review churn later.
3. Adjudicate the **name-collision question** above (blocking).
4. Verify the v1 scope is right-sized given the GOAP consumer context.
5. Either green-light (with optional non-blocking suggestions) or list specific blocking concerns.

You're expected to **read code in the repo** — don't review the plan in isolation. Spot-check the patterns it claims to follow against `CkRelationship/Team` (full module shape), `CkSpatialQuery/Probe` (request/signal/factory patterns), and `CkInteraction/InteractionResolver` (immediate vs deferred utility split — `Request_RunQuery_Immediate` mirrors `ResolveBestInteractTargets_Immediate`).

### What's being built

A new module, `CkEqs`, providing ECS-native environmental queries. Per query (which is itself a child entity owned by the querier):

- **Generators** produce a set of `FCk_Eqs_Candidate` (location and/or entity handle): SimpleGrid, Grid (with ground projection), Donut, Cone, EntitiesWithTag.
- **Tests** filter and score those candidates: Distance, Dot, Trace (LOS via CkSpatialQuery), GameplayTag (CkEntityTag-backed), Overlap (sphere overlap via CkSpatialQuery shape cast).
- **Finalize** sorts/truncates per `RunMode` (SingleBest / AllMatching / AllMatchingSorted / RandomBest5Pct / RandomBest25Pct).
- **Results** land on `FFragment_EqsQuery_Results` (waypoints + best location + best entity + has-results), and `OnEqsQueryComplete` fires.

Consumers (GOAP actions, AS scripts) bind a delegate at request time via `CK_SIGNAL_BIND_REQUEST_FULFILLED` — they never need to hold the query handle. v1 is single-frame synchronous evaluation. GOAP integration is intentionally deferred (EQS knows nothing about `FWorldState`).

### Sibling plan context

CkNavigation was the sibling plan revised in the same session; **CkNavigation has since shipped** as a full module (with its own Gate plan), so the "future CkEqs path-cost test consumes CkNavigation's path API" hook in the plan is now realistic future work, not theoretical. Useful framing for the reviewer: when CkEqs ships, the natural next module-pair task is a `Path` test type that calls into CkNavigation.

### Reference plugins / modules to spot-check

- **`Plugins/CkFoundation/Source/CkRelationship/Public/CkRelationship/Team/*`** — full module-shape reference (Module, Log, Fragment, Fragment_Data, Processor, Utils). Plan mirrors directory layout.
- **`Plugins/CkFoundation/Source/CkSpatialQuery/Public/CkSpatialQuery/Probe/*`** — request/signal/factory patterns + `CK_REGISTER_PROCESSOR_WITH_FACTORY` lambda capturing `TWeakPtr<JPH::PhysicsSystem>` from registry context. CkEqs reuses Jolt for trace/overlap tests via the same factory.
- **`Plugins/CkFoundation/Source/CkInteraction/Public/CkInteraction/InteractionResolver/CkInteractionResolver_Utils.h`** — `ResolveBestInteractTargets_Immediate` is the precedent for `Request_RunQuery_Immediate` (synchronous evaluation bypassing the deferred processor pipeline).
- **`Plugins/CkFoundation/Source/CkEntityTag/Public/CkEntityTag/CkEntityTag_Utils.h`** — `ForEach_Entity_UsingGameplayTag(InAnyHandle, FGameplayTag) -> TArray<FCk_Handle>` is what the `EntitiesWithTag` generator and `GameplayTag` test depend on. Plan adds `CkEntityTag` to Build.cs as a result.
- **`Plugins/CkFoundation/Source/CkAi/Public/CkAi/EQS/CkEqs_Utils.{h,cpp}`** — the existing UE-EQS wrapper. Source of the name-collision flagged above. Read this to make the rename/replace/fold decision.
- **`Plugins/CkFoundation/Source/CkSpatialQuery/Public/CkSpatialQuery/Probe/CkProbeTrace_Utils.h`** — confirms the synchronous trace API (`Request_SingleLineTrace`, `Request_SingleShapeTrace`, `Request_MultiShapeTrace`) and `FCk_Probe_RayCast_Settings` / `FCk_ShapeCast_Settings` shapes used by `Trace` and `Overlap` tests.
- **`Plugins/CkFoundation/Source/CkEcs/Public/CkEcs/Scheduler/CkProcessorGroups.h`** — group ordering. Plan places all five EQS processors in `FGroup_Gameplay`; reviewer should validate the stale-transform tradeoff.

### Plan location

Read in full: [`ckeqs_prompt.md`](../../Source/ckeqs_prompt.md)

It's ~1000 lines. Key sections:
- The five-processor pipeline (HandleRequests → Generate → Test → Finalize → Cleanup), each with explicit `RunAfter` declarations (added in v2 — v1 was missing the HandleRequests→Generate dependency).
- The **per-test three-pass algorithm contract** (Raw → Min/Max → Score+Filter) that pins how UE's `NormalizeItemScores` translates into this codebase.
- `CkEqs_Algorithm.{h,cpp}` — the static-helper file that lets `Request_RunQuery_Immediate` and the processor pipeline share identical evaluation code (replaces the v1 hack of "manually invoke `ForEachEntity`").
- "Summary of Changes From the Previous Plan" at the bottom — 20 enumerated revisions vs. v1.

### Critical context — read before reviewing

- `Plugins/CkFoundation/CLAUDE.md` — top-level conventions, naming, ECS patterns.
- `Plugins/CkFoundation/Source/CLAUDE.md` — extended C++ rules.
- `Plugins/CkFoundation/Source/CkEcs/CLAUDE.md` — handle/processor/signal fundamentals.
- `Plugins/CkFoundation/Source/CkEcs/Public/CkEcs/Request/CkRequest_Data.h` — confirms `FCk_Request_Base` (BP-exposed) is the right base, not `ck::FRequest_Base` (C++-only).
- `Plugins/CkFoundation/Source/CkEcs/Public/CkEcs/Signal/CkSignal_Macros.h` — `CK_SIGNAL_BIND`, `CK_SIGNAL_BIND_PROMISE`, `CK_SIGNAL_BIND_REQUEST_FULFILLED`.
- `Plugins/CkFoundation/Source/CkEntityTag/CLAUDE.md` — how the EntityTag fragment works (used for both `EntitiesWithTag` generator and `GameplayTag` test).

### Design decisions already locked in (do NOT relitigate unless you see a real problem)

These were debated and settled by the v1→v2 Tech-Director pass:

1. **Query is a child entity owned by the querier.** Each `Request_RunQuery` spawns a dedicated entity with `FFragment_EqsQuery_Params`, `FFragment_EqsQuery_State`, and `FTag_EqsQuery_Pending`. Lifetime ties to the querier's context-ownership chain. Mirrors CkTargeting's TargetPoint pattern.
2. **No `ProcessorInjector/` subdirectory.** Processors register inline at the top of `CkEqs_Processor.cpp` via `CK_REGISTER_PROCESSOR` / `CK_REGISTER_PROCESSOR_WITH_FACTORY`.
3. **Tag-state pipeline:** `Pending → InProgress → Complete`, with `Failed` (terminal) and `AutoDestroy` (cleanup) tags. Each processor consumes one tag and emits the next.
4. **`CkEqs_Algorithm.{h,cpp}` exists** — `DoGenerate`, `DoRunTests`, `DoFinalize` as static helpers. Both processors and `Request_RunQuery_Immediate` call them. v1 plan's "manually invoke ForEachEntity" hack is gone.
5. **Delegate-at-request pattern** — `FCk_Request_Eqs_RunQuery` carries an optional `OnComplete` delegate; `HandleRequests` binds it via `CK_SIGNAL_BIND_REQUEST_FULFILLED` on entity creation. Callers never need the query handle. (Different from CkNavigation, where the consumer holds the agent handle and uses two-step BindTo.)
6. **`Request_RunQuery` returns `void`.** Old plan returned an invalid handle and told callers to "poll or BindTo separately" — broken for Blueprint/AS users. v2 fix: delegate-at-request.
7. **Per-test three-pass algorithm** (Raw → Min/Max → Score+Filter) is explicit and contractual. Do not collapse passes. Min/Max is per-test, not carried across tests. Matches UE EQS exactly.
8. **Score accumulator initial value `1.0f`.** Each test multiplies in: `_Score *= NormalizedScore * Weight`. Filter failures set `_Passed = false` but do **not** short-circuit (later tests still contribute score, used for debug; Finalize drops failed candidates).
9. **`FCk_Request_Base`** (BP-exposed) for the request struct.
10. **Signal name: `OnEqsQueryComplete`** (verb-first, matches `OnProbeBeginOverlap`).
11. **`EntitiesWithTag` and `GameplayTag` test use `UCk_Utils_EntityTag_UE::ForEach_Entity_UsingGameplayTag`.** `CkEntityTag` is a Build.cs dep.
12. **Group:** all five processors in `FGroup_Gameplay`. Stale-transform tradeoff documented (queries against moving actors read last-frame positions; acceptable for GOAP cadence, callers needing same-frame can use `Request_RunQuery_Immediate` from a `FGroup_PostTransform` processor).
13. **GOAP integration is one-directional and deferred.** EQS writes to fragments; GOAP reads. EQS knows nothing about `FWorldState`, `FKeyRegistry`, or any GOAP type.
14. **No replication.** Query entities are local-only.
15. **No async multi-frame spreading in v1.** `_MaxCandidatesPerFrame` field exists on Params (ignored by v1) so v2 doesn't break Blueprint layout.
16. **Nav-cost generators/tests are explicitly out of scope.** Any nav-dependent code path triggers `CK_TRIGGER_ENSURE`. (CkNavigation has now shipped, so a `Path` test is realistic future work but still not v1.)
17. **CkGameplayDebugger integration is a required follow-up** (candidate spheres color-lerped by score, best pick highlighted, per-test breakdown). Not v1, but cannot ship v1 without it on the board.
18. **Engine-source usage is math-only.** Read UE's `NormalizeItemScores`, `UEnvQueryGenerator_Donut`, `UEnvQueryGenerator_Cone` for the formulas; do **not** depend on UE EQS at runtime. (Note: per session-memory rule, don't grep `Engine/Source/...` blindly. The plan tells the senior programmer the specific files; reviewer can confirm the file paths are still right in our UE version.)

### What I specifically want you to scrutinize

#### A. Architecture / decomposition

- **Name-collision adjudication** (see "Pre-flight" above). Pick a resolution.
- **Child-entity-per-query pattern:** correct shape, or should queries be in-place fragments on the querier with state-machine progression? CkTargeting uses child entities, so this matches precedent — but is the precedent right for EQS specifically (where queries are typically one-shot, not persistent)?
- **Five-processor decomposition** (HandleRequests, Generate, Test, Finalize, Cleanup): right granularity? In particular, should Generate and Test be merged into a single processor that runs both phases in one entity tick? They share an `FGroup_Gameplay` slot anyway.
- **`CkEqs_Algorithm.{h,cpp}` shared between processors and `Request_RunQuery_Immediate`:** clean abstraction, or does it create maintenance pressure (every new test type touches both the Algorithm helper and the processor's match clause)?
- **The `_Querier` and `_Context` fields on `FCk_Eqs_QueryParams`:** is `FCk_Handle` the right type, or should they be typed handles tied to specific feature traits (e.g. `FCk_Handle_Transform` to enforce that the querier has a transform)?

#### B. CkFoundation convention compliance

- File layout matches `CkRelationship/Team`?
- No `ProcessorInjector/` subdir? Processors register inline at top of Processor.cpp via `CK_REGISTER_PROCESSOR` / `CK_REGISTER_PROCESSOR_WITH_FACTORY`?
- `CK_DEFINE_CUSTOM_FORMATTER_ENUM` on every enum (inline, not just in a checklist)?
- `CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE` shape correct against `OnProbeBeginOverlap` precedent?
- Request struct uses `FCk_Request_Base` + `CK_REQUEST_DEFINE_DEBUG_NAME` + `CK_DEFINE_CONSTRUCTORS` with essential params only?
- Friend-class declarations on `FCk_Eqs_Candidate` / `FCk_Eqs_QueryResults` granted to `FCk_Eqs_Algorithm` + `UCk_Utils_Eqs_UE` (mirrors CkProbe's pattern)?
- Read/write expressed via `const FFragment&` vs `FFragment&` on `ForEachEntity` params (NOT `TReadOnly<>`/`TReadWrite<>` template wrappers)?
- `CK_SIGNAL_BIND_REQUEST_FULFILLED` used correctly in `HandleRequests` (auto-unbind after first fire, ignore in-flight payloads)?
- `CopyAndRemove` / `Update_Requests` pattern used correctly to drain `FFragment_EqsQuery_Requests`?

#### C. Algorithm correctness

- **Three-pass per-test contract:** does the plan's wording actually pin it correctly? The "Phase 3: don't short-circuit on filter failure" rule is subtle — verify the plan is unambiguous.
- **`NormalizeAndScore` port from UE EQS:** plan says read `EnvQueryTest.cpp::NormalizeItemScores` and translate equation-by-equation (Linear, Square, InverseLinear, Sine, Constant). Spot-check that the equations the plan lists match UE's actual formulas (Sine is `Sin(Value * PI * 0.5f)` not `Sin(Value * PI)`, etc.).
- **Donut/Cone generator math:** plan delegates to UE source. Are there gotchas in `UEnvQueryGenerator_Donut`'s arc-sweep math worth pre-flagging (e.g. ring-r=0 handling, single-point-per-ring degenerate case)?
- **`Overlap` test using zero-length shape cast** (`StartPos == EndPos == CandidateLocation`): plan flags this as needing verification at research time. Is zero-length-cast-as-overlap actually how Jolt's `Request_MultiShapeTrace` behaves, or do we need a real overlap API? (CkSpatialQuery's `Request_BeginOverlap` is persistent-probe-flavored, not one-shot.)
- **`_BestEntity` selection for location-based generators:** the first candidate's `_EntityHandle` will always be invalid for SimpleGrid/Grid/Donut/Cone. Plan documents this but does it consistently — all `Get_BestEntity` callers should be safe against invalid?

#### D. Test coverage

- Validation checkpoints 1–13 walk through fragment_data → algorithm → each processor → utils. Are they sufficient, or are there obvious gaps (multi-test interaction, weight-zero edge cases, RandomBest5Pct distribution sanity)?
- Should there be a CkTests gym/AutoTest for EQS now (vs deferred)? CkAStar has gym precedent.
- Is the unit-test against UE's `NormalizeItemScores` (mentioned in checkpoint 4) sufficient, or should it cover all five equations across known-good inputs?

#### E. Risks the plan calls out — sized correctly?

The plan's "Known Limitations" section calls out:
- Stale transforms by one frame (group placement).
- `EntitiesWithTag` + `GameplayTag` test depend on `CkEntityTag` semantics.
- Overlap test assumes zero-length shape cast == point overlap.
- `_MaxCandidatesPerFrame` reserved, ignored in v1.
- No nav-dependent generators/tests in v1.

Are any severity-misjudged? Big ones missing? In particular:
- Worst-case query cost — a `Grid` generator with `_GridHalfSize=2000`, `_SpaceBetween=50` is 6400 candidates × N tests × possibly per-candidate trace. Is there a budget mechanism? Should there be?
- Filter-fail-but-still-score behavior — is it confusing for callers reading `_Score` on a `_Passed=false` candidate?

#### F. Forward-compat with deferred work

- Will the v1 API surface make the deferred CkGameplayDebugger work painful, or do `_Candidates`, `_Score`, `_Passed` carry enough state to drive a useful overlay?
- `_MaxCandidatesPerFrame` reserved field: does it really preserve forward-compat, or will async spreading require deeper struct changes anyway?
- `Path` test type as future work (now realistic since CkNavigation shipped): does the v1 enum/test infrastructure leave a clean slot for it, or is `ECk_Eqs_TestType` already crowded?
- Existing `UCk_Utils_Eqs_UE` in CkAi: should it be deprecated in lockstep with this module shipping, or kept indefinitely? (Tied to the name-collision adjudication.)

### Output format — fill in the CTO Review Response section below

Be direct. If the plan is good, say so and green-light it — don't manufacture issues to look thorough. Specific blockers ("Phase X step Y must do Z because of W"), not vague concerns ("this section feels under-specified").

You have full repo and engine access. Read freely. The name-collision adjudication is a hard requirement before implementation can start.

---

## CTO Review Response

### Verdict

**CHANGES REQUESTED** — three blockers, each small and well-scoped. Plan is otherwise in great shape; Pass-3 + 3.1 closed the structural problems and the algorithm contract is the cleanest version we've had. Once the three items below are resolved (none of them require redesign), this is GREEN-LIGHT.

### Name-collision adjudication

**Resolution: delete the existing `CkAi/Public/CkAi/EQS/CkEqs_Utils.{h,cpp}` helper outright. Keep the new module's `UCk_Utils_Eqs_UE`. Remove the `CkAi/Public/CkAi/EQS/` directory.**

Rationale:

- The CkAi helper is a single static method `SetEqsNamedIntParam` — a Blueprint wrapper around `UEnvQueryInstanceBlueprintWrapper::SetNamedParam` doing a `reinterpret_cast<float*>` int-bit hack. Total surface: ~20 LOC.
- Grep across the whole repo (`Grep "UCk_Utils_Eqs_UE|SetEqsNamedIntParam"`): the only two files referencing the symbol are its own `.h` and `.cpp`. **Zero external callers.** Nothing in the Blueprint asset graph likely references it either, given (a) it's unused for two years going by `git log` style on this kind of helper, and (b) UE-EQS itself is not in the runtime path of any current Chainkemists product per `CkAi/CLAUDE.md`'s "Anti-patterns" section.
- "Rename the new class" is the wrong call: `UCk_Utils_<Feature>_UE` is the naming contract for every Utils library in CkFoundation. Renaming the new module to dodge a dead helper is tail wagging dog.
- "Fold the CkAi helper into CkEqs" is the wrong call: the new CkEqs module is explicitly a from-scratch reimplementation that takes UE EQS as a math reference only. Folding the old helper in would force CkEqs to take a `EnvironmentQuery` module dep on UE-EQS — exactly what the new design is built to avoid. It also drags `UEnvQueryInstanceBlueprintWrapper` into a module that should never know about it.
- "Rename the CkAi helper" is technically valid (e.g. `UCk_Utils_UeEqs_UE`) but pays maintenance cost for no benefit when the symbol has zero callers.

Implementation note: delete the two files in CkAi, drop any `EnvironmentQuery` dep that `CkAi.Build.cs` was carrying *only* for that helper (verify before removing — if EnvironmentQuery is needed elsewhere in CkAi it stays), and confirm no `.uasset` references the BPFL by searching the redirector table after the rename. If a stray BP reference exists, redirect it to `nullptr` (the function is one-line and the caller can re-implement inline).

### Blocking issues

1. **`GameplayTag` test cannot be implemented as the plan describes — the EntityTag API has no `Has`-style method.** Pass-3 P3-E7 defers this to ASK-USER ("verify the exact EntityTag API"). I checked `CkEntityTag/Public/CkEntityTag/CkEntityTag_Utils.h` directly. The public surface is: `Add`, `Add_UsingGameplayTag`, `Request_TryRemove`, `Request_TryRemove_UsingGameplayTag`, `TryGet_Tag`, `ForEach_Entity`, `ForEach_Entity_UsingGameplayTag`. **There is no `Has` / `Has_UsingGameplayTag` / `HasTag` at all.** The deferred ASK-USER answer is "the API doesn't exist; pick a strategy." Plan must commit to one of these *before* coding — do not block-on-ASK for a question with a known structural answer:
   - **(Preferred) Add `Has_UsingGameplayTag(const FCk_Handle&, FGameplayTag) -> bool` to `CkEntityTag_Utils.{h,cpp}`** as part of CkEqs's PR. Trivial — looks up the entity's tag fragment, checks tag membership, returns bool. No new dep direction (CkEqs already depends on CkEntityTag). Document it in `CkEntityTag/CLAUDE.md`'s "Key API" list.
   - (Acceptable fallback) In `DoRunTest_GameplayTag`, call `ForEach_Entity_UsingGameplayTag(InQuerier, _RequiredTag)` **once outside the candidate loop**, build a `TSet<FCk_Handle>` from the result, and `.Contains(Candidate._EntityHandle)` per candidate. O(E + C) instead of O(C × E). Document in PROGRESS.md the chosen approach and *why* not (a).
   
   Pick (a). It's a 30-line change in CkEntityTag and gives every future caller the same primitive cleanly. Bake the choice into the plan's Test spec section before sending to the implementer.

2. **Brief–plan inconsistency on processor group placement.** Brief decision #12 ("locked in") says all five processors live in `FGroup_Gameplay`. Pass-3 V1 must-have list (line 168 of the plan) and every per-processor spec in the plan (`FProcessor_Eqs_HandleRequests` line 1488, `FProcessor_Eqs_Generate` line 1516, `FProcessor_Eqs_Test` line 1609, `FProcessor_Eqs_Finalize` line 1682) say `FGroup_PostTransform`. These are mutually exclusive. **`FGroup_PostTransform` is the correct call** — `CkEcs/Public/CkEcs/Scheduler/CkProcessorGroups.h:129-132` shows PostTransform runs *after* `FGroup_Transform_Finalize`, so the querier's location has already been written for this frame; the "stale transform" tradeoff the brief documents under #12 is moot. Keep PostTransform; **update the brief's decision #12 to match Pass-3's PostTransform placement**, OR explicitly tell me Pass-3 was wrong and we're reverting. (I expect the former — Pass-3's reasoning is right.) Until reconciled, the senior programmer is going to read both documents and pick at random.

3. **`CK_DEFINE_CONSTRUCTORS(FCk_Eqs_QueryParams, _Querier)` is missing `_GeneratorParams`.** Plan lines 970–971: only `_Querier` is in the macro. A `FCk_Eqs_QueryParams` constructed with just a querier has a default-initialized `_GeneratorParams` (SimpleGrid with default radii), no tests, RunMode::SingleBest. That's a zero-test query that returns a candidate set scored at 1.0 and picks whatever's at index 0 — silently-wrong consumer behavior. Either (a) make the constructor `CK_DEFINE_CONSTRUCTORS(FCk_Eqs_QueryParams, _Querier, _GeneratorParams, _Tests)` and treat tests as essential (matches the spirit of CkFoundation's Request struct rules in `Source/CLAUDE.md` §7), or (b) drop the parametrized constructor entirely and require callers to construct + setter the fields explicitly. I lean (a). Either way, the current single-arg constructor invites consumer bugs. Update the plan.

### Non-blocking suggestions

1. **`_OnComplete` delegate as a USTRUCT UPROPERTY (request-struct field) on `FCk_Request_Eqs_RunQuery`.** This works (delegate UPROPERTYs marshal in/out of BP nodes as long as they're `DECLARE_DYNAMIC_DELEGATE_*`), but I'd want one explicit confirmation in PROGRESS.md that a BP-authored caller can wire a custom event into the delegate field on the request struct before passing it to `Request_RunQuery`. There's a class of bugs where dynamic delegates lose binding through copy on USTRUCT pass-by-value. CkInteraction/InteractionResolver may already have a precedent for this — implementer should mirror that exactly if so.

2. **Immediate-path truncation warning needs to include both numbers.** Plan E5 truncates the candidate list to `_MaxCandidates_ImmediatePathHardCap` with a Warning. The Warning string should include both the configured count from the generator (e.g. 6400) and the cap (1024), not just "list truncated" — otherwise the consumer fixes the symptom (raises the cap) instead of the root cause (a 100m × 100m grid is too big for a single-frame query).

3. **Filter-fail-but-still-score is debug-confusing.** Pass-3 keeps `_Score` accumulating on filter-failed candidates so the debugger can show "what would this candidate's score have been." That's right for the debugger overlay — but for any non-debug consumer reading `_Candidates`, they should be reading `_Passed` first. Add a one-liner to `CkEqs/CLAUDE.md`: *"Always check `_Passed` before treating `_Score` as meaningful — failed candidates carry a residual score for debug-overlay display only."*

4. **Per-query priority deferred to v1.1 (P3-E4) is the right call**, but worth recording explicitly in the deferred-work table in `PROGRESS.md` so it doesn't get rediscovered. Same for the `_ReferenceValue` v1.1 reservation (F6).

5. **CkGameplayDebugger follow-up.** Brief item #17 says CkGameplayDebugger overlay is required follow-up but cannot ship v1 without being on the board. It is on the board (mentioned in deferred work). Confirm it's also tracked in CkGameplayDebugger's own backlog so it doesn't fall between modules. Out of scope for this PR but worth a `TODO(CkEqs/CkGameplayDebugger)` somewhere visible.

6. **Ring-r=0 and degenerate Donut case (F1).** Plan handles `NumRings == 1` correctly (single ring at OuterRadius). One additional edge: `_PointsPerRing == 1` produces a single point per ring, which can stack on `_ArcDirection` — not a bug per se, but worth a sentence in the generator comment that this is "expected, not degenerate."

7. **`Path` test future-slot in `ECk_Eqs_TestType`.** The enum already has `PathCost` and `Reachability` (added in Pass-3 against my read of the brief which still calls these "future work"). The brief's description of "v1 has no nav-dependent tests" no longer matches Pass-3 — the brief reviewer-context wants updating to acknowledge nav tests *are* in v1 now that CkNavigation has shipped. (Same kind of staleness as item-2 above.)

### Convention compliance spot-checks performed

- Read `Plugins/CkFoundation/Source/CkAi/Public/CkAi/EQS/CkEqs_Utils.h:1-35` and `.cpp:1-23` — confirmed the existing `UCk_Utils_Eqs_UE` is a one-method `SetEqsNamedIntParam` BP wrapper using `reinterpret_cast<float*>(&InValue)` for int-bit smuggling into `UEnvQueryInstanceBlueprintWrapper::SetNamedParam`.
- `Grep "UCk_Utils_Eqs_UE|SetEqsNamedIntParam"` over `D:\Repos\CkPlugins` — only two matches, both inside the helper's own files. Confirms zero external callers.
- Read `Plugins/CkFoundation/Source/CkEntityTag/Public/CkEntityTag/CkEntityTag_Utils.h:1-86` — confirmed the public API has no `Has`-style method. Only `Add`, `Add_UsingGameplayTag`, `Request_TryRemove`, `Request_TryRemove_UsingGameplayTag`, `TryGet_Tag`, `ForEach_Entity` (FName), `ForEach_Entity_UsingGameplayTag` (FGameplayTag → `TArray<FCk_Handle>`). This contradicts P3-E7's framing of the question as "verify which Has overload" — the answer is *neither overload exists.*
- Read `CkEcs/Public/CkEcs/Scheduler/CkProcessorGroups.h:1-171` — confirmed `FGroup_PostTransform` follows `FGroup_Transform_Finalize`, so processors there read finalized transforms. Brief's "stale transform" framing under decision #12 doesn't apply if the plan's PostTransform placement holds.
- Read root `Plugins/CkFoundation/CLAUDE.md` and `Plugins/CkFoundation/Source/CLAUDE.md` — confirmed `CK_PROPERTY`/`CK_DEFINE_CONSTRUCTORS`/`FCk_Request_Base` patterns the plan claims to follow are accurate. Plan's macro usage in `FCk_Request_Eqs_RunQuery` (lines 1063-1093) matches the §7 Request Struct Pattern. Spot-checked `_OnComplete` UPROPERTY against the §7 boilerplate — passes.
- Read `Plugins/CkFoundation/Source/CkAi/CLAUDE.md` — confirms the CkAi module's purpose is "AI utilities — EQS wrappers for ECS." With the new CkEqs module landing, that purpose statement no longer matches reality; CkAi will become an empty namespace once the helper is deleted. Worth flagging as a follow-up: either repurpose CkAi or remove it from `CkFoundation.uplugin`.
- Did not read UE engine source per the standing rule. Algorithm-math correctness is taken on the plan's authority and the engine_questions_block.md flow.

### Algorithm correctness notes

- F4 (scoring equations) — plan correctly identifies that "Sine" is a hallucination from earlier reviews and removes it. Final set (Linear, Square, SquareRoot, InverseLinear, Constant) matches what UE actually exposes per the plan's claim. SquareRoot was missing from Pass-1; good catch.
- F7 (degenerate normalization) — multiplicative-identity skip on `Min ≈ Max` is the right call and matches the plan's explanation. Not "return clamped ScoringFactor" — that was Pass-3's earlier mistake. Pass-3.1 implicitly retains F7 from Pass-3, which is fine.
- F1 (Donut `NumRings == 1`) — plan explicitly guards the divide-by-zero. Good. UE-parity comment is honest about UE's latent bug there.
- F3 (Cone parameterization swap) — moving from `_ConeAngle/_ConeDistance/_ConeNumPoints` to `_ConeDegrees/_ConeRange/_ConeAngleStep/_ConePointSpacing` was the right call. The old "count derived per total budget" had ambiguous distribution semantics. New form matches UE's deterministic ray-and-step layout.
- P3-E3 (test atomicity) — yielding only at test boundaries is correct. The Pass-2 mid-test yield was a real correctness bug (Min/Max corruption across frames); dropping `_NextCandidateIndexInTest` is the right surgical fix even at the cost of one-test-per-tick worst-case spikes.
- Pass-3.1 E1 (yield condition simplification with anti-deadlock invariant) — clean rewrite. The "force-run at least one test per tick" invariant is load-bearing and the plan documents it explicitly. Per the verbatim-paste-into-CLAUDE.md note, this MUST land in `CkEqs/CLAUDE.md`, not just in the prompt.

### Design / architecture observations

- **Five-processor decomposition is right; do not collapse.** The brief asks if Generate and Test could be merged. They can't, because Test may yield across frames (P3-E3). Collapsing would re-run generator math per yield, blowing the cost model. HandleRequests → Generate → Test (possibly multi-frame) → Finalize → Cleanup is correct.
- **Child-entity-per-query pattern is correct.** Querier as context-owner means lifetime cascades automatically; the P3-E6 re-validate-querier-at-Test belt-and-suspenders covers the only race (querier dies after Generate but before Test resumes from yield). CkTargeting precedent is the right one to follow.
- **`FCk_Handle` for `_Querier`/`_Context` is correct.** Forcing a typed handle (e.g. `FCk_Handle_Transform`) would cripple the API — any entity with a `FFragment_Transform` is a valid querier. Per-query `Has<FFragment_Transform>` validation in Generate is the right contract.
- **`CkEqs_Algorithm.{h,cpp}` shared helper is correct.** Replaces Pass-1's "manually invoke ForEachEntity" hack. Yes, every new test type touches both Algorithm and the Test processor's dispatch — but that's the dispatch table, not duplication. Acceptable.
- **Multiplicative composition is the right scoring model for GOAP.** The brief's question about score-stacking ceiling (E5 in Pass-3.1) is well-handled — three Score-purpose tests at 0.5 each = 0.125 is OK because GOAP only argmaxes; the ceiling guidance ("split into Filter pre-pass + smaller Score pass") is exactly the right consumer guidance.
- **EQS and Targeting eventual merge?** No — they solve different problems. CkTargeting picks a *single* target from a known candidate set under deterministic scoring. CkEqs *generates* candidate sets (via spatial generators) and applies multi-test pipelines. They share zero algorithm. EQS may consume Targeting's "candidate set" output in some future case (a Targeting result fed into an `EntitiesWithTag`-like generator), but the modules stay distinct.
- **Brief context drift.** Several "design decisions already locked in" entries in the brief don't match Pass-3. This is the second time I've reviewed a CkFoundation plan where the brief's locked-in section drifted from the plan's actual state. Process suggestion to the plan author: regenerate the brief's §8 by *reading the plan's V1 must-have list*, not from session memory. Decision #12 (Gameplay vs PostTransform) and decision #16 (no nav-dependent generators/tests in v1) are the two affected here.

### Sign-off conditions

GREEN-LIGHT flips when all three blockers are resolved:

1. CkAi `UCk_Utils_Eqs_UE` deleted (files removed, `EnvironmentQuery` dep dropped from `CkAi.Build.cs` if it was only there for this helper). New module's `UCk_Utils_Eqs_UE` keeps the name.
2. `GameplayTag` test spec updated to commit to **adding `Has_UsingGameplayTag(const FCk_Handle&, FGameplayTag) -> bool` to `CkEntityTag_Utils.{h,cpp}`** as part of this PR (preferred), OR explicit fallback to "build TSet from `ForEach_Entity_UsingGameplayTag` once per test, contains-check candidates" with rationale in PROGRESS.md. Either way, **delete the ASK-USER block entry B1** — the API doesn't exist, so the question is moot.
3. Brief decision #12 reconciled with Pass-3's `FGroup_PostTransform` placement (preferred) OR plan reverted to `FGroup_Gameplay` (not preferred — wrong group for transform-reading processors).
4. `CK_DEFINE_CONSTRUCTORS(FCk_Eqs_QueryParams, ...)` updated to include `_GeneratorParams` (and probably `_Tests`) as essential — or the parametrized constructor dropped entirely.

The non-blocking suggestions are nice-to-haves, not gates.

---

### Reviewer

- **Name:** Saad Rustam (CTO sign-off)
- **Date:** 2026-05-08
