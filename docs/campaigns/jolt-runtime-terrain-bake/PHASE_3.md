# Phase 3 — the JoltHeightField feature (public surface, all three environments)

**Goal:** requirements 4–6 as public API: `Request_BakeHeightField` /
`Request_RemoveHeightField` (synchronous, subsystem-backed — mirrors `Request_BakeComponent`) and
`Request_UpdateRegion` (deferred ECS request drained by a processor carrying the mandatory
`RunAfter FProcessor_JoltWorld_WaitForAsync` edge). Resolves the surface half of decision D3 and
implements D4's documentation stance. **Load `ck-macros-and-codegen` before starting** (typesafe
handle, request struct, processor registration, completion-guard mechanics).

## Entry criteria

1. Phase 2 exit criteria hold (this phase consumes its three functions).
2. Baseline re-run recorded.
3. Symbol re-verification:
   - `rg -n "Get_TempAllocator|_TempAllocator" Source/CkJolt/Public/CkJolt/World/CkJoltWorld.h`
     — if no public accessor exists, Step 1 adds one (pre-approved, one-liner).
   - `rg -n "NotifyShapeChanged" Source/CkThirdParty -g BodyInterface.h` → present.
   - `rg -n "TryDerive_SignatureFromProfile" Source/CkJolt` → declaration in
     `CollisionLayers/CkJoltCollisionLayer_Utils.h`.
   - `rg -n "MakeCompletionGuard" Source/CkEcs` → present (completion contract plumbing).
   - Read `Source/CkJolt/Body/CkJoltBody_Processor.cpp`'s `HandleRequests` processor +
     registration + its `RunAfter` edge declaration — it is the mold for the new processor.
   - Read `CkTimer`'s request drain (`ck::algo::ForEachRequest` + `ck::Visitor` +
     `DoHandleRequest` overloads) — the canonical drain shape.

## New files (all under `Source/CkJolt/Public/CkJolt/StaticWorld/`)

| File | Contents |
|---|---|
| `CkJoltHeightField_Fragment_Data.h` | typesafe handle, envelope enum, ParamsData, UpdateRegion request. Reflected; **no Jolt includes** |
| `CkJoltHeightField_Fragment.h` | `ck::FFragment_JoltHeightField_Current` (+ inner JPH ref) and `_Requests`. Plain C++; Jolt includes fine |
| `CkJoltHeightField_Processor.h/.cpp` | `FProcessor_JoltHeightField_HandleRequests` + `CK_REGISTER_PROCESSOR` |
| `CkJoltHeightField_Utils.h/.cpp` | `UCk_Utils_JoltHeightField_UE` BPFL |

Modified: `CkJoltStaticWorld_Subsystem.h/.cpp` (bake/remove/track + reciprocal guard),
`CkJoltWorld.h` (temp-allocator accessor if missing).

## Pre-designed shapes (fill bodies; do not redesign signatures)

`CkJoltHeightField_Fragment_Data.h`:

```cpp
USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKJOLT_API FCk_Handle_JoltHeightField : public FCk_Handle_TypeSafe { GENERATED_BODY()  CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_JoltHeightField); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_JoltHeightField);

/// FromSamples: the encodable range is exactly the initial samples' min/max — later updates may
/// not exceed them. Explicit: the caller declares the deformation envelope up front (craters may
/// dig below / debris pile above the initial surface, within the declared bounds).
UENUM(BlueprintType)
enum class ECk_Jolt_HeightFieldEnvelopeMode : uint8
{
    FromSamples,
    Explicit
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Jolt_HeightFieldEnvelopeMode);

USTRUCT(BlueprintType)
struct CKJOLT_API FCk_Fragment_JoltHeightField_ParamsData
{
    GENERATED_BODY()
public:
    CK_GENERATED_BODY(FCk_Fragment_JoltHeightField_ParamsData);
private:
    // UE-row-major world heights, _SampleCount x _SampleCount, indexed [y * N + x]. Holes:
    // UCk_Utils_JoltHeightField_UE::Get_NoCollisionHeightValue().
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    TArray<float> _WorldHeights;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    int32 _SampleCount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FVector2D _ScaleXY = FVector2D{100.0, 100.0};

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FVector _Origin = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FRotator _Rotation = FRotator::ZeroRotator;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FName _CollisionProfileName = TEXT("BlockAll");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_Jolt_HeightFieldEnvelopeMode _EnvelopeMode = ECk_Jolt_HeightFieldEnvelopeMode::FromSamples;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, EditCondition = "_EnvelopeMode == ECk_Jolt_HeightFieldEnvelopeMode::Explicit"))
    float _EnvelopeMinHeight = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, EditCondition = "_EnvelopeMode == ECk_Jolt_HeightFieldEnvelopeMode::Explicit"))
    float _EnvelopeMaxHeight = 0.0f;

    // Optional: the render component this surface was generated from. When set, the bake is
    // KEYED by it — re-baking the same component replaces the previous heightfield, and the
    // subsystem refuses (loudly) to hold both a heightfield AND a tri-mesh bake for one
    // component. When unset, addressing is handle-only.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    TObjectPtr<UPrimitiveComponent> _SourceComponent;

public:
    CK_PROPERTY_GET(_WorldHeights);
    CK_PROPERTY_GET(_SampleCount);
    CK_PROPERTY_GET(_ScaleXY);
    CK_PROPERTY(_Origin);
    CK_PROPERTY(_Rotation);
    CK_PROPERTY(_CollisionProfileName);
    CK_PROPERTY(_EnvelopeMode);
    CK_PROPERTY(_EnvelopeMinHeight);
    CK_PROPERTY(_EnvelopeMaxHeight);
    CK_PROPERTY(_SourceComponent);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_JoltHeightField_ParamsData, _WorldHeights, _SampleCount, _ScaleXY);
};

USTRUCT(BlueprintType)
struct CKJOLT_API FCk_Request_JoltHeightField_UpdateRegion : public FCk_Request_Base
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Request_JoltHeightField_UpdateRegion);
    // INSIDE the struct, right here — the macro expands to a protected member override
    // (CkRequest_Data.h); placing it after the closing brace does not compile. House placement
    // mold: the request structs in CkJoltBody_Fragment_Data.h.
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_JoltHeightField_UpdateRegion);
private:
    // UE grid coords on the baked lattice; any rect within [0, SampleCount)^2 — block alignment
    // is handled internally (expand-and-overlay), the caller never sees it.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    int32 _X = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    int32 _Y = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    int32 _SizeX = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    int32 _SizeY = 0;
    // UE-row-major, _SizeX * _SizeY world heights for the rect. Holes allowed.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    TArray<float> _WorldHeights;
public:
    CK_PROPERTY_GET(_X);
    CK_PROPERTY_GET(_Y);
    CK_PROPERTY_GET(_SizeX);
    CK_PROPERTY_GET(_SizeY);
    CK_PROPERTY_GET(_WorldHeights);
public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_JoltHeightField_UpdateRegion, _X, _Y, _SizeX, _SizeY, _WorldHeights);
};
```

`CkJoltHeightField_Fragment.h` (namespace `ck`; friends: the processor, the utils class, the
subsystem — mirror `FFragment_JoltStaticActor_Current`'s friend list style):

```cpp
struct CKJOLT_API FFragment_JoltHeightField_Current
{
    // friends: FProcessor_JoltHeightField_HandleRequests, ::UCk_Utils_JoltHeightField_UE,
    //          ::UCk_JoltStaticWorld_Subsystem_UE
private:
    JPH::Ref<JPH::HeightFieldShape> _InnerHeightField;
    int32   _LogicalSampleCount = 0;
    FVector2D _ScaleXY = FVector2D::ZeroVector;
    uint32  _BodyId = 0;      // raw index+sequence, same convention as JoltStaticActor's array
public:
    CK_PROPERTY_GET(_InnerHeightField);
    CK_PROPERTY_GET(_LogicalSampleCount);
    CK_PROPERTY_GET(_ScaleXY);
    CK_PROPERTY_GET(_BodyId);
};

struct CKJOLT_API FFragment_JoltHeightField_Requests
{
    // friends as above
private:
    TArray<FCk_Request_JoltHeightField_UpdateRegion> _Requests;
public:
    CK_PROPERTY_GET(_Requests);
};
```

The attribution entity carries `FFragment_JoltStaticActor_Current` TOO (with `_BodyIds = {the
one body}`) so `Request_RemoveBodiesForEntity`, `FProcessor_JoltStaticActor_EndPlay`, ray-hit
attribution and `Get_NumBodies` all work with ZERO changes to those paths. The heightfield
fragments are additive. Encodable range is NOT cached in the fragment — read it from the inner
shape (`GetMinHeightValue/GetMaxHeightValue`) wherever needed; one source of truth.

`CkJoltHeightField_Utils.h` (UFUNCTION shapes — concrete return type on its own line, house
category/display-name style copied from `UCk_Utils_JoltStaticWorld_UE`):

```cpp
UCLASS(NotBlueprintable)
class CKJOLT_API UCk_Utils_JoltHeightField_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    CK_GENERATED_BODY(UCk_Utils_JoltHeightField_UE);

    // Has / Cast / CastChecked / DoCast / DoCastChecked over FCk_Handle_JoltHeightField —
    // copy the exact macro/wrapper set from UCk_Utils_JoltStaticActor_UE.

    /// Bakes a heightfield body into the Jolt static world from CPU data (no component, no
    /// cook). Synchronous; returns an INVALID handle after a loud ensure on bad params
    /// (heights count != N*N, N < 2, unresolvable profile, inverted explicit envelope, source
    /// component already tri-mesh-baked). NOTE (D4): a heightfield stores one height per sample
    /// and cannot represent overhangs or vertical faces — bake wall/overhang geometry separately
    /// (Request_BakeComponent tri-mesh path).
    UFUNCTION(BlueprintCallable, Category = "Ck|Jolt",
              meta = (WorldContext = "InWorldContextObject"),
              DisplayName="[Ck][Jolt] Request Bake Height Field Into Static World")
    static FCk_Handle_JoltHeightField
    Request_BakeHeightField(
        const UObject* InWorldContextObject,
        const FCk_Fragment_JoltHeightField_ParamsData& InParams);

    /// Removes the heightfield's body and destroys its attribution entity. Idempotent.
    UFUNCTION(BlueprintCallable, Category = "Ck|Jolt",
              DisplayName="[Ck][Jolt] Request Remove Height Field From Static World")
    static void
    Request_RemoveHeightField(
        UPARAM(ref) FCk_Handle_JoltHeightField& InHeightField);

    /// DEFERRED region edit (applied by the heightfield processor after the async physics step
    /// is consumed — never mid-step). Only the given cells change; block alignment is internal.
    /// Completion: Succeeded once applied; Failed on out-of-bounds or a height outside the
    /// baked envelope (the whole request is rejected — no partial application).
    UFUNCTION(BlueprintCallable, Category = "Ck|Jolt",
              meta = (AutoCreateRefTerm = "InDelegate"),
              DisplayName="[Ck][Jolt] Request Update Height Field Region")
    static FCk_Handle_JoltHeightField
    Request_UpdateRegion(
        UPARAM(ref) FCk_Handle_JoltHeightField& InHeightField,
        const FCk_Request_JoltHeightField_UpdateRegion& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintPure, Category = "Ck|Jolt",
              DisplayName="[Ck][Jolt] Get Height Field Sample Count")
    static int32
    Get_SampleCount(
        const FCk_Handle_JoltHeightField& InHeightField);

    /// The baked envelope: region updates outside this range are rejected.
    UFUNCTION(BlueprintPure, Category = "Ck|Jolt",
              DisplayName="[Ck][Jolt] Get Height Field Encodable Height Range")
    static FFloatInterval
    Get_EncodableHeightRange(
        const FCk_Handle_JoltHeightField& InHeightField);

    /// The sentinel height that means "this cell has no collision" (hole).
    UFUNCTION(BlueprintPure, Category = "Ck|Jolt",
              DisplayName="[Ck][Jolt] Get Height Field No Collision Value")
    static float
    Get_NoCollisionHeightValue();
};
```

Utils bodies: `Request_UpdateRegion` follows the house request contract EXACTLY — validate handle
(`CK_ENSURE_IF_NOT` + fire `Failed_NotEnqueued` via `InDelegate.ExecuteIfBound` before returning
on rejection), copy the request into a named local, `Set_CompletionDelegate` only
`if (InDelegate.IsBound())`, append to `FFragment_JoltHeightField_Requests` (add the fragment if
absent). Bake/Remove resolve the subsystem from the world context and forward.

Subsystem additions (`CkJoltStaticWorld_Subsystem.h/.cpp`):

```cpp
auto Request_BakeHeightField(const FCk_Fragment_JoltHeightField_ParamsData& InParams) -> FCk_Handle_JoltHeightField;
auto Request_RemoveHeightField(FCk_Handle_JoltHeightField& InHeightField) -> void;
// tracking (BOTH containers):
//   TMap<TWeakObjectPtr<const UPrimitiveComponent>, FCk_Handle_JoltHeightField> _ManualHeightFields;
//     — keyed replacement + reciprocal-guard lookups only (only bakes with _SourceComponent).
//   TArray<FCk_Handle_JoltHeightField> _HeightFieldEntities;
//     — EVERY baked heightfield, keyed or not. This is the Deinitialize drain list: a handle-only
//       (unkeyed) heightfield sits in no map, and without this container it is unreachable at
//       subsystem teardown. Remove entries on Request_RemoveHeightField and on replace.
```

**Deinitialize (mirrors the existing manual-entity drain in the same function):** iterate
`_HeightFieldEntities`, liveness-check each handle (skip invalid — the entity may already be
dead), `Request_RemoveBodiesForEntity` + destroy, then empty both containers. Read the existing
`Deinitialize` drain for `_ManualActorEntities`/`_ManualComponentEntities` and extend it in the
same style — do not write a parallel teardown path.

**Stale-entry rule (applies to every map/array lookup added by this phase):** entity-side
destruction (anyone destroying the attribution entity directly) never prunes these containers —
the existing replace path liveness-checks its map entry for exactly this reason (see the
`ck::IsValid(*ExistingEntity)` check in `Request_BakeComponent`'s replace block). Every new
lookup must do the same: a DEAD entry is pruned-and-ignored, never treated as a live conflict
and never double-freed.

`Request_BakeHeightField` body order (mirror `Request_BakeComponent`'s structure verbatim where
it overlaps): validate params (loud) → `TryDerive_SignatureFromProfile(Profile,
ECk_Jolt_BodyDomain::Static)` (unset → loud ensure, invalid handle) → **reciprocal guard**: if
`_SourceComponent` set and `_ManualComponentEntities` holds a **LIVE** entity for it (liveness
rule above — a stale entry is pruned, not a conflict) → `CK_ENSURE` "already tri-mesh baked; one
representation per surface" → return invalid → replace-if-rebaked via
`_ManualHeightFields` (same remove+destroy dance as `_ManualComponentEntities`) →
`CreateHeightFieldShape_Updatable` (Phase 2) → transient-entity check (same ensure text pattern)
→ create attribution entity (mirror `DoCreate_ComponentEntity`; DebugName = source component
name or `"JoltHeightField"`) → add BOTH fragments → create ONE body (`BodyCreationSettings` with
the wrapper `_Shape`, `Conv(Origin)`, `Conv(Rotation.Quaternion())`, Static motion, layer from
signature, `mUserData` = entity id — copy `DoCreate_BodiesFromExtracted`'s body-creation block)
→ append body id to BOTH `FFragment_JoltStaticActor_Current::_BodyIds` and the heightfield
fragment's `_BodyId` → `DoBatchAdd_Bodies` → track → `DoNote_BodiesChanged(1)`.

Add the reciprocal guard to the EXISTING `Request_BakeComponent` too: `_ManualHeightFields`
holds a LIVE entity for the component → loud ensure, return 0 (same stale-entry rule: dead entry
= prune and proceed).

Processor (`CkJoltHeightField_Processor.h/.cpp`) — copy the structural skeleton (group,
registration, `RunAfter` edge) from `FProcessor_JoltBody_HandleRequests`:

- View: entities with `FFragment_JoltHeightField_Current` + `FFragment_JoltHeightField_Requests`
  (+ `CK_IGNORE_PENDING_KILL` exclusion conventions as the mold has them).
- **MUST declare `RunAfter FProcessor_JoltWorld_WaitForAsync`** — copy the exact edge-declaration
  spelling from the mold. Without it the drain can run while the previous frame's async step is
  in flight and `SetHeights` races the solver (this is the reason UpdateRegion is deferred at
  all — losing the edge silently voids the design).
- Drain: copy requests, `ck::algo::ForEachRequest` + `ck::Visitor` + `DoHandleRequest` overload.
  Per request: `ck::MakeCompletionGuard` (declared AFTER the `Result` local); resolve the Jolt
  world's temp allocator (registry context `TSharedPtr<ck::FJoltWorld>` →
  `Get_TempAllocator()`; add that accessor to `CkJoltWorld.h` if absent — public, returns the
  raw `JPH::TempAllocatorImpl*`, ensure-if-null); capture
  `BodyInterface->GetCenterOfMassPosition(BodyID)`; call `ApplyHeightFieldRegionUpdate`;
  - `Applied` → `BodyInterface->NotifyShapeChanged(BodyID, PrevCom, false, DontActivate)`
    (named constexpr for the bool per house style) → `Result = Succeeded`.
  - `OutOfBounds` / `OutOfEnvelope` → `CK_ENSURE` naming the rect, the envelope, and the fix
    ("declare a wider Explicit envelope at bake time") → `Result = Failed`.
- Teardown: requests on an entity already tagged for destroy follow the standard
  `FireCancelledForPending` convention — check whether the mold's feature routes teardown
  through a `FGroup_EndPlay` processor for its `_Requests` fragment and mirror it (the
  heightfield's EndPlay body-removal itself is ALREADY handled by
  `FProcessor_JoltStaticActor_EndPlay` — do not duplicate it).

UHT fallback (only if it fires): if UHT rejects `FFloatInterval` as a BlueprintPure return type,
replace `Get_EncodableHeightRange` with two floats packed in a tiny
`FCk_Jolt_HeightFieldRange` USTRUCT(BlueprintType) in the fragment-data header — record the
swap in PROGRESS.md; do not drop the getter.

## The executable spec (commit red before implementing)

`Plugins/CkTests/Source/CkTests/Private/UnitTests/CkJolt/Test_JoltHeightField_Feature.spec.cpp`,
mimicking `Test_JoltBody_Lifecycle.spec.cpp`'s harness (NumClients=1 net world on
`/Engine/Maps/Entry`, cross-latent-command statics, delta-vs-baseline body counts):

```cpp
// "Ck.Jolt.HeightField.Feature.BakeQueryUpdateRemove"
//   1. Capture baseline static-body count.
//   2. Request_BakeHeightField: 8x8, ScaleXY 100, flat height 200, Explicit envelope {-500, 500},
//      origin (0,0,0). EXPECT valid handle; count = baseline + 1.
//   3. Get_RayCastStaticWorld from (350,350,1000) down to (350,350,-1000):
//      EXPECT hit at Z ~= 200 (tol 5) AND hit._Entity == the handle's entity
//      (UCk_Utils_JoltStaticActor_UE::Has on it → true).
//   4. Request_UpdateRegion (2,2,3,3, all 50.0) with a completion delegate; tick one frame.
//      EXPECT delegate fired Succeeded; the SAME ray now hits at Z ~= 50.
//   5. Request_UpdateRegion with a height outside the envelope (e.g. 900): AddExpectedError for
//      the envelope ensure; tick. EXPECT delegate fired Failed; ray unchanged (~50).
//   6. Request_RemoveHeightField; tick. EXPECT ray misses; count back to baseline.

// "Ck.Jolt.HeightField.Feature.CrossRepresentationGuard"
//   - Spawn a static-mesh cube actor; Request_BakeComponent(its component) → N > 0 bodies.
//   - Request_BakeHeightField with _SourceComponent = that component:
//     AddExpectedError("one representation per surface" substring) → EXPECT invalid handle,
//     body count unchanged.
//   - Remove the component bake; bake the heightfield with the same source → valid handle.
//   - Now Request_BakeComponent(same component): AddExpectedError(same substring) → returns 0.
//   - Stale-entry pin (Blocker-1): destroy the heightfield's attribution entity DIRECTLY
//     (Request_DestroyEntity on the handle, not Request_RemoveHeightField); tick. Then
//     Request_BakeComponent(same component) → succeeds (N > 0) with NO expected error — the
//     dead _ManualHeightFields entry is pruned-and-ignored, never a live conflict.
```

**Decision gate G1 (red):** the spec fails to compile (new types absent). After implementation it
must go green without edits to its expected values; ray-height mismatches at step 4 mean the
UPDATE path's row flip is wrong (see PHASE_2 fence) — fix code, never expectations.

## Gate

1. Build. UHT runs (new reflected types): any UHT error → `ck-debugging-playbook`.
2. Full `Ck.Jolt` suite: prior verdicts + 2 new greens (and Phases 1–2 tests still green).
3. Request-contract self-check (root CLAUDE.md § Requests): every early-out in
   `Request_UpdateRegion` fires `Failed_NotEnqueued`; the processor fires exactly once per
   request via the guard; teardown path covered.

## Exit criteria

- Suite delta recorded; 2 new greens; zero flips.
- `rg -n "RunAfter" Source/CkJolt/Public/CkJolt/StaticWorld/CkJoltHeightField_Processor.cpp` →
  ≥1 hit naming `FProcessor_JoltWorld_WaitForAsync`.
- `rg -n "CK_REGISTER_PROCESSOR" Source/CkJolt` → count is prior+1.
- `rg -n "NotifyShapeChanged" Source/CkJolt` → exactly 1 hit (the processor).
- Reciprocal guard: `rg -n "_ManualHeightFields" Source/CkJolt` shows hits in BOTH
  `Request_BakeHeightField` and `Request_BakeComponent`, each behind a liveness check.
- Deinit reachability: `rg -n "_HeightFieldEntities" Source/CkJolt` shows hits in
  `Request_BakeHeightField`, `Request_RemoveHeightField`, AND `Deinitialize` — a handle-only
  heightfield must be freed at subsystem teardown (Blocker-2 fix; verify by reading the drain,
  not by grep count alone).
- Diff touches only the 6 new files + subsystem pair + `CkJoltWorld.h` (+ its .cpp if the
  accessor needs one) + the new spec file. Comment audit done.

## Fences

- Do NOT make UpdateRegion synchronous "for simplicity" — the deferral IS the thread-safety
  design (SetHeights vs async step); D3.
- Do NOT cache the encodable range in the fragment or the request — read it from the inner
  shape; two copies WILL drift.
- Do NOT create a second body or rebuild the shape on update — `SetHeights` mutates in place;
  the body and its broadphase entry persist (`NotifyShapeChanged` is the only follow-up).
- Do NOT add a completion delegate to Bake/Remove — they are synchronous subsystem calls
  mirroring `Request_BakeActor/Component`, which have none (consistency beats ceremony here;
  the request-completion contract governs the DEFERRED call only).
- Cast rule (CTO-adjudicated): **new code uses `UCk_Utils_JoltHeightField_UE::CastChecked`**
  (the fragments were just added — the ensure is unreachable by contract), including the
  subsystem's create path. The existing `DoCreate_ComponentEntity`/`DoCreate_ActorEntity` sites
  use `ck::StaticCast` — leave them untouched; do not copy that spelling and do not "fix" them.
- Do NOT ensure from inside Jolt callbacks or touch ECS off the game thread — all work here is
  game-thread processor code.
- Do NOT wire the heightfield into cooked data, level sweeps, or `ExtractActor` — this surface
  is runtime-only by requirement 2.

## Risks + rollback

- **Risk (highest):** the update-path row flip composed wrong with the creation flip — mirrored
  region updates. Mitigated by the asymmetric surface + off-center rect in the spec (step 4
  probes a NON-centered region; a mirrored update moves the wrong cells and the ray assert
  catches it).
- ~~Risk: `AddExpectedError` × fire-once latch~~ **RESOLVED at CTO review (verified in code):**
  ensure repeat-suppression is disabled under automation (`CkEnsure.cpp`, the automation branch
  around its suppression check), so ensures re-fire per occurrence in test runs. House pattern:
  `AddExpectedError(<substring>, EAutomationExpectedErrorFlags::Contains, /*Occurrences*/ -1)`
  (`-1` = any count). Use that; the per-test "distinct message" workaround from earlier drafts
  rests on a wrong premise (the latch is per-site, not per-message, and it is off under
  automation anyway) — do not implement it.
- **Risk:** UHT/AS binding surprises on `TArray<float>` in a request struct — none expected
  (plain POD array), but `ck-angelscript-interop` is the skill if the generated `utils_*` wrapper
  misbehaves.
- **Whole-phase rollback:** revert the commit(s); Phase 2's API remains (unreferenced, still
  tested), Phase 1 unaffected.
