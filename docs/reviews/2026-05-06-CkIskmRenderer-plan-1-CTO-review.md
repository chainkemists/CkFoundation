# CkIskmRenderer Plan-1 — CTO Review

> **Workflow:** Review the brief below, then fill in the **CTO Review Response** section at the bottom of this file. Commit your changes — the plan author / their assistant will pick up your notes from there.

---

## Reviewer brief

You are reviewing an implementation plan for a new Unreal Engine 5.7.4 plugin module. The plan author wants your green-light before implementation starts. We have already debated the high-level technique; this review is about whether the **plan as written** will produce correct, idiomatic code that matches the existing codebase, and whether any design decisions deserve a second look.

### Your role

Senior reviewer / architect. You're the last set of eyes before ~4–6 weeks of implementation work begins. Your job is to:

1. Catch architectural issues that would be expensive to fix mid-implementation.
2. Catch convention/idiom mismatches against the existing CkFoundation codebase that would cause review churn later.
3. Identify unclear, missing, or unsafe steps.
4. Either green-light (with optional non-blocking suggestions) or list specific blocking concerns the author must address before we proceed.

You're expected to **read code in the repo** — don't just review the plan in isolation. Spot-check that the patterns it claims to follow actually look like existing code in CkFoundation.

### What's being built

A new module, `CkIskmRenderer` (Instanced Skeletal Mesh Renderer), giving ECS entities skeletal-mesh rendering with: anim sequences, montages, optional AnimBP, ragdoll, modular outfit submeshes, per-instance custom data, sockets, line traces, and notify events.

This is **Plan 1 of 2**. Plan 1 ships the **public API** with a per-entity `USkeletalMeshComponent` implementation (one SKMC per entity, owned by a per-world manager actor). Plan 2 (separate, future) replaces the rendering substrate with a batched GPU-pose-buffer cluster proxy + custom vertex factory for performance — the API stays identical.

The end-game motivator is Rewind99 (a co-op VHS-store sim) which currently has ~110–130 NPCs and severe animation-cost problems; Plan 2 is the perf payoff, Plan 1 is the API foundation.

### Reference plugin

The architecture is informed by the Skelot marketplace plugin (`E:/UE_5.7/Engine/Plugins/Marketplace/SKELOTIn8237d489b026V6/`). We are **not** depending on Skelot — we are reimplementing the technique because the marketplace license blocks redistribution inside CkFoundation. The plan ports Skelot's *architecture* (Renderer + Proxy split, AnimCollection asset, dynamic-pose-tie) but the rendering implementation in Plan 1 is much simpler (per-entity SKMC, no GPU buffer yet).

### Plan location

Read in full: [`docs/superpowers/plans/2026-05-06-CkIskmRenderer-plan-1-api-and-skmc-backed.md`](../../../../docs/superpowers/plans/2026-05-06-CkIskmRenderer-plan-1-api-and-skmc-backed.md)

It's ~4000 lines, structured as 17 phases (A through Q) with bite-sized tasks.

### Critical context — read before reviewing

You **must** read these to evaluate convention compliance:

- `Plugins/CkFoundation/CLAUDE.md` — top-level conventions, naming, ECS patterns
- `Plugins/CkFoundation/Source/CLAUDE.md` — extended C++ rules
- `Plugins/CkFoundation/Script/CLAUDE.md` — AngelScript rules (for the AS sections)

The new module mirrors the structure of the existing `Plugins/CkFoundation/Source/CkIsmRenderer/` (instanced static mesh renderer). **Spot-check the plan against actual files** in CkIsmRenderer's `Public/CkIsmRenderer/Renderer/` and `Public/CkIsmRenderer/Proxy/` — if the plan claims to follow a pattern, verify the real file does it that way.

Additional reference modules used by the plan:

- `Plugins/CkFoundation/Source/CkAnimation/` — already exists; plan does **not** duplicate, but composes alongside.
- `Plugins/CkFoundation/Source/CkStateMachine/` — likely caller of the API (state machines drive `Request_PlayAnimation`); plan must keep API caller-agnostic.
- `Plugins/CkFoundation/Source/CkInventory/` — best example of the request-fragment + processor + Visitor dispatch pattern. Pay attention here: the plan adopts Inventory's `AddOrGet<>()._Requests.Emplace(...)` idiom for enqueuing requests; verify this is in fact what Inventory does.
- `Plugins/CkTests/Script/CkAStar/` — sample gym + AutoTest layout the plan's test gym imitates.

### Design decisions already locked in (do NOT relitigate unless you see a real problem)

These were debated and settled before plan-writing started:

1. **Reimplement, not depend on Skelot.** Marketplace license blocks bundling.
2. **Two-plan split.** Plan 1 = API + per-entity SKMC; Plan 2 = batched GPU cluster proxy + vertex factory. Plan 1 has **no** perf target — "doesn't crash at 100 entities" is enough.
3. **Per-world `UWorldSubsystem` + manager actor pattern.** One subsystem per world owns N renderer actors (one per AnimCollection). Manager actor owns the SKMC pool.
4. **Server is no-op for rendering**, but entity-side state still works (sockets, notifies, line trace). Dynamic-pose entities still spawn an SKMC on the server because AnimBP/montage logic may matter for gameplay.
5. **Replication is the caller's problem.** Renderer is local-presentation only.
6. **Async loading is caller-driven.** `Add(...)` requires a loaded pointer; callers use `FStreamableManager` and call `Add` on completion.
7. **State machine integration is OUT OF SCOPE for Plan 1.** The renderer exposes `Request_*` methods; whatever calls them (state machine, gameplay processor, AS script, debug UI) is the caller's choice. Future work (separate plan) wires CkStateMachine to drive these calls.
8. **Modular submeshes share one skeleton.** Master-pose merging across skeletons is future work.
9. **AnimBP is per-state opt-in, NOT default.** Default is sequence playback; state machine promotes to AnimBP for states that need IK / aim offsets / layered blends.
10. **Outfit system is required.** Rewind99 has extensive character customization.
11. **Acceptance numbers (Plan 2 only):** 200 entities sequence-mode < 1.5 ms/frame; 40 entities concurrent AnimBP; 8 concurrent ragdolls. Plan 1 has no perf target.
12. **Live-coding caveat:** restart editor for render-code changes is the documented compromise.

### What I specifically want you to scrutinize

#### A. Architecture / decomposition

- Is the Renderer (shared, per-AnimCollection) vs Proxy (per-entity) split the right shape? Will it block Plan 2's batched cluster proxy work?
- The pose source is an enum (`Sequence` / `AnimBP` / `Ragdoll`) on a fragment. Is this the right abstraction, or should it be a tag-based state per source?
- The five proxy processors (Setup, HandleRequests, UpdateTransform, EmitFinishedEvents, EndPlay): right granularity, or should some be merged / split?
- The custom `UCk_IskmNotify_AnimInstance` requires AnimBP authors in downstream projects to derive from it. Is this acceptable, or do we need a more invasive bridge?

#### B. CkFoundation convention compliance

- Naming, file layout, macro usage, fragment vs. fragment-data split.
- Trailing return types, `auto`, `NOT` keyword, `MoveTemp`, `ck::IsValid` usage in the plan's code samples.
- `CK_REGISTER_PROCESSOR` registration is present for every processor.
- `CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE` shape is correct (Inventory signals are a good comparator).
- Request struct pattern (`FCk_Request_Base`, `CK_REQUEST_DEFINE_DEBUG_NAME`, `CK_DEFINE_CONSTRUCTORS` with essential params, `CK_PROPERTY` on optional).
- The request-enqueue idiom `AddOrGet<...>()._Requests.Emplace(...)` — verify this matches `CkInventory_Utils.cpp`.

#### C. UE 5.7.4 API specifics

- The plan uses `LeaderPoseComponent` (correct post-5.6 API). Verify no stray `MasterPoseComponent` references slipped through.
- `USkeletalMeshComponent::PlayAnimation` (for sequence-mode without AnimBP) silently allocates a `UAnimSingleNodeInstance` per call. Plan acknowledges; sanity-check this is OK for Plan 1.
- The custom AnimInstance hooks `OnMontageEnded` and `HandleNotify`. Is this enough to forward all relevant events, or are we missing `OnMontageStarted`, `BlendingIn`, etc.?
- Ragdoll path uses `SetAllBodiesSimulatePhysics(true)` + `SetSimulatePhysics(true)`. Verify this is the correct sequence in 5.7.4.
- Anything else in the 5.6/5.7 deprecation tail (animation graph nodes, notify state APIs) that the plan should address proactively?

#### D. Test coverage

- Phase P (gym) and Phase Q (AutoTests). Do the six AutoTests cover the contract well, or are there obvious gaps (submesh detach, custom-data clamping, AnimBP-mode interactions, event-binding policies)?
- Is the gym station design realistic — five sub-stations under the Cycler, each demonstrating one capability — or do we need a single stress station?

#### E. Risks the plan calls out — sized correctly?

The plan's risks table calls out: master-pose cross-skeleton merging out-of-scope, `PlayAnimation` allocations, Live Coding, AnimBP authors not deriving from `UCk_IskmNotify_AnimInstance`, lazy montage AnimInstance notify hookup. Are any severity-misjudged, or are we missing big ones?

#### F. Forward-compat with Plan 2

- Does the Plan-1 API surface lock in any decisions that will be painful to honor when Plan 2 introduces sequence-mode-without-SKMC?
- The `UCk_IskmAnimCollection_Data` PDA has placeholder shape for Plan 2's GPU bake fields. Is the asset shape sufficient, or will Plan 2 need asset migration?

### Output format — fill in the CTO Review Response section below

Be direct. If the plan is good, say so and green-light it — don't manufacture issues to look thorough. If it's not ready, list specific blockers; "the testing strategy is unclear" is not actionable, "Phase Q needs explicit failure modes for the async-load test" is.

You have full repo access. Read freely.

---

## CTO Review Response

### Verdict

**CHANGES REQUESTED** — the plan is structurally sound and 90% of it I'd green-light as written, but there are five specific issues that will produce wrong code or stale state if implemented verbatim. None require a redesign; all are localized fixes to the plan.

### Blocking issues

1. **Variant dispatch in `FProcessor_IskmProxy_HandleRequests::ForEachEntity` violates the documented convention.** The plan uses `if constexpr (std::is_same_v<T, ...>)` chains inside the visitor lambda (Phase F1 step 2, replicated in Phases G/H/I/J/K). CkFoundation convention — explicit in `Plugins/CkFoundation/CLAUDE.md` ("Variant Dispatch with `ck::Visitor`") and matched by the existing `CkIsmProxy_Processor.cpp:396-411` — is to use a **single generic lambda calling an overloaded free function `DoHandleRequest`** with one overload per request type. The CkInventory/CkIsmProxy code is the load-bearing reference for this; new modules must follow it. Refactor every Phase F–K handler to be `DoHandleRequest(handle, params, current, anim_state, pose_source, custom_data, const FCk_Request_IskmProxy_X&)` overloads, and reduce the visitor body to one line per Inventory's pattern. This will also tidy ~150 lines out of the plan.

2. **Processor groups are wrong for two of the five proxy processors.** The plan assigns `using Group = FGroup_Gameplay_Rendering` to all five (Phase E3). The existing `CkIsmProxy_Processor.h` uses three different groups for the equivalent stages: `FGroup_Gameplay_Rendering` (Setup, HandleRequests), `FGroup_PostTransform` (UpdateTransform), `FGroup_EndPlay` (EndPlay). With everything in one group, transform sync runs *before* the gameplay tick that produced the new transform, and EndPlay can race entity-destruction. Update Phase E3 to:
   - `FProcessor_IskmProxy_Setup` / `_HandleRequests` → `FGroup_Gameplay_Rendering` (as planned)
   - `FProcessor_IskmProxy_UpdateTransform` → `FGroup_PostTransform`
   - `FProcessor_IskmProxy_EmitFinishedEvents` → `FGroup_PostTransform` (event emission after transform settles)
   - `FProcessor_IskmProxy_EndPlay` → `FGroup_EndPlay`

3. **`FTag_IskmProxy_HasActiveMontage` is added but never removed.** Phase J1 adds the tag in `DoHandleRequest_PlayMontage` but neither `DoHandleRequest_StopMontage` nor the `OnMontageBlendingOut` path in Phase M removes it. As written, every entity that ever played a montage will be permanently flagged as having an active montage — any future processor filtering on this tag is broken. Add `InHandle.Remove<FTag_IskmProxy_HasActiveMontage>()` to (a) `DoHandleRequest_StopMontage`, and (b) the `EmitFinishedEvents` processor when it observes a montage-finish (or via a dedicated cleanup in Phase M). While you're there, decide whether `_CurrentMontage` should also be reset.

4. **File-structure section advertises files that no phase actually creates.** The Phase 0 file tree lists `EntityScript/CkIskmRenderer_EntityScript.cpp/.h` (line ~42 of the plan) and the Renderer processor block lists `_AsyncLoadComplete` (line ~33). Neither is created by any phase — Phase N reduces "async load processor" to a `Claude.md` docstring + a tag-cleanup line. Either (a) add the two phases that actually build them (with the AsyncLoad processor watching the pending tag and a manager-actor entity script if one is needed), or (b) trim the file tree and the Renderer processor list down to what gets built. Right now an executor following this plan will scaffold empty files that block the build.

5. **`UCk_IskmNotify_AnimInstance::Set_OwningProxyHandle` is wired in Phase M Step 3 inside the Setup processor, but **not** re-wired in `DoHandleRequest_SetAnimInstanceClass` (Phase I) nor in `DoHandleRequest_PlayMontage`'s lazy AnimInstance branch (Phase J1).** Phase M Step 3 narratively says "the same hookup must run inside the `SetAnimInstanceClass` request handler from Phase I and the lazy-AnimInstance branch in `DoHandleRequest_PlayMontage` from Phase J", but the code blocks for I and J shipped in the plan don't include it. As written, `Request_SetAnimInstanceClass` and the lazy-montage AnimInstance path will silently break the notify forwarder. Either move the `Set_OwningProxyHandle(...)` cast-and-set into a small helper (`DoApply_AnimInstanceClass(SKMC, Class, Handle)`) called from all three sites, or paste the three lines into the Phase I / J code blocks explicitly. Don't leave it as a "must remember" prose note.

### Non-blocking suggestions

1. **Phase Q3 promises a `Get_NumAttachedSubmeshes` helper but Phase H doesn't add it.** Phase H's Utils block already includes `Get_NumAttachedSubmeshes` in my read of the plan — good — but the Q3 task still calls it out as "you'll need a small helper". Tighten Q3 so the test author doesn't add a duplicate.

2. **Subsystem layout deviates from the sibling module without justification.** Plan puts the subsystem at `Public/CkIskmRenderer/Subsystem/CkIskmSubsystem.h`; the existing `CkIsmRenderer` has it at `Public/CkIsmRenderer/CkIsmSubsystem.h` (no extra subfolder). For symmetry with the named "mirror" reference, drop the `Subsystem/` directory level. Cosmetic, but cheap consistency.

3. **`FProcessor_IskmProxy_UpdateTransform` calls `SetWorldTransform` every frame for every proxy unconditionally.** With 100 entities and SKMC sub-tick this is fine for Plan 1, but the `if (NOT current.Equals(new)) { Set... }` guard in the plan compares with float-equality default tolerance — a moving entity will always be unequal, and the branch is structurally a no-op for moving entities. Either drop the guard (it doesn't help) or use `Equals(NewTransform, KINDA_SMALL_NUMBER)`. Minor.

4. **`AnimBP authors must derive from UCk_IskmNotify_AnimInstance` is a real ergonomic tax.** The risks table acknowledges it; consider adding the suggested first-time runtime warning *as an actual checklist item* in Phase M (current text says "Document the requirement" — make it a `CK_ENSURE_IF_NOT` or `ck::iskm::Warning` line in the Setup processor when the resolved AnimInstance is not a `UCk_IskmNotify_AnimInstance` subclass). Cheaper to wire it now than to chase silent test failures later.

5. **`UCk_IskmAnimCollection_Data::IsDataValid` uses `FString::Printf(TEXT("... [%d] ..."))` for error formatting.** This works fine but is inconsistent with the `{}`-format convention used elsewhere in CkFoundation. Switch to `ck::Format_UE` or accept that `IsDataValid` is allowed to use raw printf because it's editor-only validation. Probably not worth a churn pass — flag it during PR review only.

6. **Phase E2's `FFragment_IskmProxy_PoseSource` is a one-enum fragment.** I went back and forth on whether to merge it into `AnimState`. Keep it separate — Plan 2 will filter processors on pose source via `TInclude<FTag_IskmProxy_PoseSource_Sequence>` etc., and a dedicated fragment makes the eventual tag-promotion cheap. Document the forward-compat reason in the fragment comment so a future maintainer doesn't "simplify" it.

7. **Default AnimInstance class is loaded synchronously in Setup (`LoadSynchronous()`).** Acceptable for Plan 1's "doesn't crash at 100 entities" target but it's a hidden hitch. Worth a one-line note in the module Claude.md so callers know `Add` may block briefly on first use of an AnimCollection.

### Convention compliance spot-checks performed

- `Plugins/CkFoundation/Source/CkIsmRenderer/CkIsmRenderer.Build.cs` — confirmed Build.cs shape; plan's added deps (`AnimGraphRuntime`, `CkAnimation`, `CkPhysics`) are reasonable and not in the sibling module.
- `Plugins/CkFoundation/Source/CkIsmRenderer/Public/CkIsmRenderer/Proxy/CkIsmProxy_Utils.cpp` — verified `AddOrGet<FFragment_IsmProxy_Requests>()._Requests.Emplace(InRequest)` is the live idiom (lines 44, 55, 66, 77). Plan matches this exactly. ✓
- `Plugins/CkFoundation/Source/CkIsmRenderer/Public/CkIsmRenderer/Proxy/CkIsmProxy_Utils.cpp:14-35` — verified `Add` directly fragments the input handle (does not create a child entity). Plan's choice to do the same matches the per-entity proxy convention; CkInventory's child-entity flavor doesn't apply here. ✓
- `Plugins/CkFoundation/Source/CkIsmRenderer/Public/CkIsmRenderer/Proxy/CkIsmProxy_Processor.cpp:396-411` — verified the canonical visitor-with-overloaded-`DoHandleRequest` shape; this is what triggered Blocking #1.
- `Plugins/CkFoundation/Source/CkIsmRenderer/Public/CkIsmRenderer/Proxy/CkIsmProxy_Processor.h:27-200` — verified processor group assignments per stage; this is what triggered Blocking #2.
- `Plugins/CkFoundation/Source/CkIsmRenderer/Public/CkIsmRenderer/Proxy/CkIsmProxy_Fragment.h:61-71` — verified the `using XxxRequestType = ...; using RequestType = std::variant<...>;` shape. Plan inlines the variant types directly; both styles compile, but the alias-then-variant style is the live convention. Non-blocking; mention in PR review.
- `Plugins/CkFoundation/CLAUDE.md` — confirmed `CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE` shape, `CK_REQUEST_DEFINE_DEBUG_NAME` shape, `CK_PROPERTY*` macros, `NOT` keyword, trailing return convention, and `ck::IsValid` policy. Plan adheres throughout.
- `Plugins/CkFoundation/Source/CkInventory/Public/CkInventory/Inventory/CkInventory_Utils.cpp:75-78` — confirmed `CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE` macro shape. Plan does not call this in its Utils.cpp samples (only `CK_DEFINE_CPP_CASTCHECKED_TYPESAFE` in the header) — verify the AS-side cast path still compiles. Worth checking during implementation but not a blocker; the typesafe handle infra works either way for new types.
- `Plugins/CkFoundation/Source/CLAUDE.md` "Module Tier Table" — confirmed `CkIskmRenderer` belongs at Tier 4 with the dependency closure stated. Plan's wrap-up correctly adds the row.

### Design / architecture observations

- **Renderer/Proxy split is the right shape and forward-compat with Plan 2.** The renderer is keyed on the AnimCollection asset and owns the world-side resource; proxies are per-entity and consume that resource. When Plan 2 introduces the GPU pose-buffer cluster, it slots into the existing `RendererActor` (or a sibling) without needing to relitigate per-entity API.

- **Pose source as a fragment-stored enum is correct for Plan 1.** I considered whether it should be a tag (`FTag_IskmProxy_PoseSource_Sequence` etc.) so processors can filter directly. For Plan 1 the enum is simpler. For Plan 2, when sequence-mode entities skip the SKMC entirely, you'll want a tag (or three) so the cluster-update processor can include sequence-mode and exclude AnimBP/Ragdoll without reading every entity's fragment. Add this transition to the Plan-2 backlog rather than pre-engineering it.

- **Five proxy processors is the right granularity.** Setup / HandleRequests / UpdateTransform / EmitFinishedEvents / EndPlay maps cleanly to the five concrete responsibilities. I considered whether `EmitFinishedEvents` could collapse into `HandleRequests` — it can't, because finish detection has to happen *after* the SKMC ticks, which is one frame after a request fires. Keep them separate.

- **The custom AnimInstance approach is acceptable but the burden on AnimBP authors is real.** Two things will help: (a) the warning suggested in non-blocking #4, (b) ship a `UCk_IskmNotify_AnimInstance_BPBase` AnimInstance Blueprint base in CkIskmRenderer's Content/ folder so authors can right-click → "Create from CkIskmNotify base" rather than re-rooting an existing AnimBP. Don't block Plan 1 on this; add it to the Plan 2 cleanup list.

- **Test gym design is fine.** Five sub-stations under one Cycler matches existing gym conventions (CkAStar/CkAttribute precedent). The SpawnArmy sub-station gives the visual proof of "doesn't crash at 100 entities" without needing a stress station — that's Plan 2's job. Per the user-memory rule on -X content placement and `AgentSpawnFront` anchors, the Phase P3 sample code does this correctly (`Row * -150.0f` keeps the army in -X). ✓

- **AutoTest coverage is adequate but has one obvious gap.** Q1–Q6 cover anim-finish, montage-notify, outfit, ragdoll-pose, custom-data, async-load. They don't cover **AnimBP-mode interactions** (Q7 candidate: `Request_PlayAnimation` on an AnimBP-mode proxy logs verbose-and-no-ops as documented; switching back to Sequence mode resumes playback). Add a Q7 if cheap; otherwise punt to Plan 2 since AnimBP integration there is more substantial.

- **Replication is the caller's problem — agreed, but worth saying louder.** The Claude.md anti-patterns block already says this. Consider also documenting the *recommended* state-machine pattern: state machine replicates its current state enum, and on `OnRep_State` re-issues `Request_PlayAnimation`. This is the kind of guidance that keeps people from inventing per-feature replication.

- **Live-coding caveat is correctly sized.** Documented compromise; no action.

- **Plan-2 forward-compat: asset shape is sufficient.** `_NumCustomDataFloat` and the placeholder for GPU-bake fields cover what Plan 2 needs. No migration concern.

### Sign-off conditions

To flip to **GREEN-LIGHT WITH NON-BLOCKING NOTES**, address the five blocking items above. Specifically:

1. Refactor Phase F1 / G / H / I / J / K to use overloaded free-function `DoHandleRequest` with a single generic visitor lambda, matching `CkIsmProxy_Processor.cpp:396-411`.
2. Update Phase E3 processor `using Group =` lines: `UpdateTransform → FGroup_PostTransform`, `EmitFinishedEvents → FGroup_PostTransform`, `EndPlay → FGroup_EndPlay`.
3. Add `InHandle.Remove<FTag_IskmProxy_HasActiveMontage>()` to `DoHandleRequest_StopMontage` (Phase J1) and to the montage-finish path in Phase M.
4. Either (a) trim the file-structure block and the renderer-processor list to match what gets built, or (b) add the missing phases for `EntityScript/` and `_AsyncLoadComplete`. (a) is the lower-effort fix and matches stated scope.
5. Either extract a `DoApply_AnimInstanceClass(SKMC, Class, Handle)` helper or paste the `Set_OwningProxyHandle` block into the Phase I and Phase J code blocks explicitly so executors don't miss it.

No re-review required after these changes — they're mechanical. Ship.

---

### Reviewer

- **Name:** CTO (Saad)
- **Date:** 2026-05-06
