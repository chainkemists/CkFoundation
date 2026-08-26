#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Macros/CkMacros.h"

// This API returns JPH::Ref<JPH::Shape> — carry the ck::IsValid support for it with the API so
// every consumer TU sees the same executor specialization (an ODR requirement, not a convenience).
#include "CkJolt/CkJolt_Utils.h"
#include "CkJolt/CollisionLayers/CkJoltCollisionLayer_Data.h"
#include "CkJolt/Settings/CkJolt_ProjectSettings.h"

#include <PhysicalMaterials/PhysicalMaterial.h>
#include <UObject/StrongObjectPtr.h>

#include <Jolt/Jolt.h>
#include <Jolt/Core/Reference.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Geometry/IndexedTriangle.h>
#include <Jolt/Math/Float3.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>
#include <Jolt/Physics/Collision/Shape/HeightFieldShape.h>

// --------------------------------------------------------------------------------------------------------------------

class AActor;
class UPrimitiveComponent;
class UStaticMeshComponent;
class UInstancedStaticMeshComponent;
class USplineMeshComponent;
class UBrushComponent;
class UBodySetup;

// --------------------------------------------------------------------------------------------------------------------

namespace ck::jolt::bake
{
    /// LevelSweep (streaming/cook path) skips Movable-mobility components — kinematic/dynamic territory.
    /// ExplicitActor (Request_BakeActor) bakes them: the caller declared the actor static-in-intent, and
    /// runtime-spawned actors are necessarily Movable.
    enum class ECk_Jolt_ExtractionPolicy : uint8
    {
        LevelSweep,
        ExplicitActor
    };

    /// Level-sweep admission control, built from project settings once per sweep/cook and passed
    /// explicitly so tests construct filters directly (no CDO mutation). ExplicitActor extraction
    /// ignores the filter entirely — the caller declared the actor static-in-intent.
    struct CKJOLT_API FCk_Jolt_BakeFilter
    {
        ECk_Jolt_BakeMobilityPolicy _MobilityPolicy = ECk_Jolt_BakeMobilityPolicy::All;
        // Strong: the cooker's World Partition walk collects garbage between batches, and a raw
        // pointer to a Blueprint exclusion class would dangle mid-cook.
        TArray<TStrongObjectPtr<UClass>> _ExcludedActorClasses;
        TArray<FName> _ExcludedActorTags;
        TArray<FName> _ExcludedComponentTags;
        TArray<TEnumAsByte<ECollisionChannel>> _ExcludedObjectChannels;
        TArray<FName> _ExcludedCollisionProfiles;
        ECk_EnableDisable _ExcludeOverlapOnlyComponents = ECk_EnableDisable::Disable;

        static auto Make_FromProjectSettings() -> FCk_Jolt_BakeFilter;

        /// Order-insensitive fingerprint of the whole filter. Stored on the cooked index and compared at
        /// load: cooked data baked under DIFFERENT filter settings is stale for the whole map.
        auto ComputeHash() const -> uint64;
    };

    /// Per-sweep visibility: WHY a level contributed nothing is otherwise invisible (the per-component
    /// skip logs at VeryVerbose only, and an all-skipped level used to add zero bodies in total silence).
    /// Aggregated per level and per BeginPlay sweep by the static-world subsystem.
    struct CKJOLT_API FCk_Jolt_ExtractionStats
    {
        int32 _NumComponentsConsidered = 0;
        int32 _NumComponentsExcludedByMobility = 0;
        int32 _NumComponentsExcludedByFilter = 0;
        int32 _NumActorsExcludedByFilter = 0;
        int32 _NumBodiesExtracted = 0;

        auto operator+=(const FCk_Jolt_ExtractionStats& InOther) -> FCk_Jolt_ExtractionStats&
        {
            _NumComponentsConsidered += InOther._NumComponentsConsidered;
            _NumComponentsExcludedByMobility += InOther._NumComponentsExcludedByMobility;
            _NumComponentsExcludedByFilter += InOther._NumComponentsExcludedByFilter;
            _NumActorsExcludedByFilter += InOther._NumActorsExcludedByFilter;
            _NumBodiesExtracted += InOther._NumBodiesExtracted;
            return *this;
        }
    };

    /// Scale is baked into _Shape; _Position/_Rotation are the body's world transform.
    struct CKJOLT_API FCk_Jolt_ExtractedBody
    {
        JPH::Ref<JPH::Shape> _Shape;
        FVector _Position = FVector::ZeroVector;
        FQuat _Rotation = FQuat::Identity;
        FCk_Jolt_CollisionSignature _Signature;
        float _Friction = 0.7f;
        float _Restitution = 0.3f;
        TEnumAsByte<EPhysicalSurface> _SurfaceType = SurfaceType_Default;
        TWeakObjectPtr<const UPrimitiveComponent> _SourceComponent;
    };

    // ----------------------------------------------------------------------------------------------------------------

    /// One Jolt shape per unique (BodySetupGuid, quantized scale, trace flag).
    /// Not thread-safe; one cache per extraction pass.
    class CKJOLT_API FCk_Jolt_ShapeCache
    {
    public:
        auto GetOrCreate_Shape(
            const UBodySetup& InBodySetup,
            const FVector& InScale,
            const FString& InDebugName) -> JPH::Ref<JPH::Shape>;

        auto Get_NumUniqueShapes() const -> int32;

    private:
        struct FKey
        {
            FGuid _BodySetupGuid;
            FIntVector _QuantizedScale;
            uint8 _TraceFlag = 0;

            auto operator==(const FKey& InOther) const -> bool
            {
                return _BodySetupGuid == InOther._BodySetupGuid
                    && _QuantizedScale == InOther._QuantizedScale
                    && _TraceFlag == InOther._TraceFlag;
            }

            friend auto GetTypeHash(const FKey& InKey) -> uint32
            {
                return HashCombine(HashCombine(GetTypeHash(InKey._BodySetupGuid),
                    GetTypeHash(InKey._QuantizedScale)), GetTypeHash(InKey._TraceFlag));
            }
        };

        TMap<FKey, JPH::Ref<JPH::Shape>> _Shapes;
    };

    // ----------------------------------------------------------------------------------------------------------------
    // Extraction entry points
    // ----------------------------------------------------------------------------------------------------------------

    /// Appends one extracted body per physics representation Chaos would see for a STATIC actor; returns
    /// the number appended. Always skips: unregistered, editor-only/visualization, NoCollision,
    /// simulating physics. Under LevelSweep the filter additionally governs mobility and the settings
    /// exclusions (classes/tags/channels/profiles/overlap-only); ExplicitActor ignores the filter.
    /// Collision enabled but NO valid geometry ensures and is skipped.
    CKJOLT_API auto ExtractActor(
        const AActor& InActor,
        FCk_Jolt_ShapeCache& InShapeCache,
        TArray<FCk_Jolt_ExtractedBody>& OutBodies,
        const FCk_Jolt_BakeFilter& InFilter,
        ECk_Jolt_ExtractionPolicy InPolicy = ECk_Jolt_ExtractionPolicy::LevelSweep,
        FCk_Jolt_ExtractionStats* OutStats = nullptr) -> int32;

    /// The COMPONENT-level settings exclusions alone (tags/channels/profiles/overlap-only) — no
    /// eligibility or mobility checks. Exposed for consumers that admit components through their own
    /// policy but still honor the designer exclusions (CkUnrealComponent's Automatic bake).
    CKJOLT_API auto Get_IsComponentExcludedByBakeFilter(
        const UPrimitiveComponent& InComponent,
        const FCk_Jolt_BakeFilter& InFilter) -> bool;

    /// Component-level entry (used by ExtractActor and by tests). Same skip/ensure rules — but only
    /// the COMPONENT-level filter checks; actor-level exclusions (class, actor tags) live in ExtractActor.
    CKJOLT_API auto ExtractComponent(
        const UPrimitiveComponent& InComponent,
        FCk_Jolt_ShapeCache& InShapeCache,
        TArray<FCk_Jolt_ExtractedBody>& OutBodies,
        const FCk_Jolt_BakeFilter& InFilter,
        ECk_Jolt_ExtractionPolicy InPolicy = ECk_Jolt_ExtractionPolicy::LevelSweep,
        FCk_Jolt_ExtractionStats* OutStats = nullptr) -> int32;

    // ----------------------------------------------------------------------------------------------------------------
    // Shape builders (exposed for tests and for the dynamic-body feature)
    // ----------------------------------------------------------------------------------------------------------------

    /// Honors the BodySetup's CollisionTraceFlag: UseComplexAsSimple -> Chaos cooked tri-mesh; everything
    /// else -> AggGeom, falling back to the tri-mesh only when AggGeom is empty and the flag permits
    /// complex. Returns null (after CK_ENSURE) when no valid collision representation exists.
    CKJOLT_API auto BuildShape_FromBodySetup(
        const UBodySetup& InBodySetup,
        const FVector& InScale,
        const FString& InDebugName) -> JPH::Ref<JPH::Shape>;

    /// Signed enclosed volume of the triangle list (front face = right-handed winding, matching Jolt's
    /// single-sided mesh collision), normalized by AABB volume and measured about the AABB center so an
    /// open sheet stays near zero wherever the mesh sits. ~+1 for an outward-wound solid, strongly
    /// negative for INSIDE-OUT geometry, ~0 for open/flat/degenerate input — 0 is "no verdict", not
    /// "healthy". Drives the inside-out ensure in the tri-mesh bake.
    CKJOLT_API auto ComputeMeshWindingRatio(
        const JPH::VertexList& InVertices,
        const JPH::IndexedTriangleList& InTriangles) -> double;

    /// Same metric, measured by walking an already-created shape's triangles (mesh shapes only —
    /// every other subtype returns 0 = no verdict). This is the form that audits a RESTORED
    /// pre-baked blob, which never passes through the tri-mesh build above.
    CKJOLT_API auto ComputeShapeWindingRatio(const JPH::Shape& InShape) -> double;

    /// Heights are UE-row-major in world-height units (landscape local height x scale.Z already applied),
    /// InSampleCount x InSampleCount, indexed [y * InSampleCount + x]; InScaleXY = landscape quad scale.
    /// The returned shape is axis-corrected for Ck's Z-up world: Jolt heightfields are local Y-up spanning
    /// X/Z, so it wraps in RotatedTranslatedShape(+90deg about X) with rows flipped and a -(N-1)*scaleY
    /// local-Z offset. Holes: pass HeightFieldNoCollisionValue() as the sample height.
    CKJOLT_API auto CreateHeightFieldShape(
        const TArray<float>& InWorldHeights,
        int32 InSampleCount,
        const FVector2D& InScaleXY) -> JPH::Ref<JPH::Shape>;

    CKJOLT_API auto HeightFieldNoCollisionValue() -> float;

    // ----------------------------------------------------------------------------------------------------------------
    // Updatable heightfields (runtime deformation)
    // ----------------------------------------------------------------------------------------------------------------

    /// The axis-corrected wrapper a body is built from, plus the INNER heightfield the region-update
    /// path edits. Both reference the same storage, so a region update is visible through _Shape
    /// immediately — no rebuild, no new body.
    struct CKJOLT_API FCk_Jolt_UpdatableHeightField
    {
        JPH::Ref<JPH::Shape>            _Shape;
        JPH::Ref<JPH::HeightFieldShape> _HeightField;
    };

    /// Same geometry contract as CreateHeightFieldShape, plus a deformation ENVELOPE: heights are
    /// stored 16-bit-quantized over a range fixed at construction, so an edit outside that range is
    /// unrepresentable. Unset = the range is exactly the initial samples' min/max. Set = the caller
    /// declares the range up front so later craters can dig below (or debris pile above) the initial
    /// surface. Jolt ignores whichever bound the initial samples already exceed.
    CKJOLT_API auto CreateHeightFieldShape_Updatable(
        const TArray<float>& InWorldHeights,
        int32 InSampleCount,
        const FVector2D& InScaleXY,
        const TOptional<FFloatInterval>& InDeformationEnvelope) -> FCk_Jolt_UpdatableHeightField;

    /// A block-aligned Jolt-space rect (Jolt requires SetHeights rects to be block multiples).
    struct CKJOLT_API FCk_Jolt_HeightFieldRegionPlan
    {
        int32 _JoltX = 0;
        int32 _JoltY = 0;
        int32 _SizeX = 0;
        int32 _SizeY = 0;
    };

    /// Pure UE-rect -> Jolt-rect planning: applies the SAME row flip creation uses (jolt row =
    /// N-1-y) and expands OUTWARD to block alignment. Unset = the rect is empty or out of bounds;
    /// the caller diagnoses, so this stays a testable primitive.
    CKJOLT_API auto ComputeHeightFieldRegionPlan(
        int32 InLogicalSampleCount,
        int32 InShapeSampleCount,
        int32 InBlockSize,
        int32 InUeX,
        int32 InUeY,
        int32 InUeSizeX,
        int32 InUeSizeY) -> TOptional<FCk_Jolt_HeightFieldRegionPlan>;

    enum class ECk_Jolt_HeightFieldRegionUpdateResult : uint8
    {
        Applied,
        OutOfBounds,
        OutOfEnvelope
    };

    /// Expand-and-overlay region edit: plans the aligned rect, reads the CURRENT heights for it,
    /// overlays the caller's UE-row-major values (row flip applied), and writes it back. Reading
    /// first is what keeps the alignment border — and any holes in it — intact; writing an expanded
    /// rect from caller data alone would silently erase geometry the caller never addressed.
    /// Every value is validated against the shape's encodable range BEFORE anything is written,
    /// because Jolt's SetHeights CLAMPS out-of-range values silently. HeightFieldNoCollisionValue()
    /// is a legal incoming value (punching a hole) and is exempt from that validation.
    /// Reports rather than ensures: the loud boundary is the public bake/update API.
    CKJOLT_API auto ApplyHeightFieldRegionUpdate(
        JPH::HeightFieldShape& InOutHeightField,
        int32 InLogicalSampleCount,
        int32 InUeX,
        int32 InUeY,
        int32 InUeSizeX,
        int32 InUeSizeY,
        const TArray<float>& InWorldHeights,
        JPH::TempAllocator& InTempAllocator) -> ECk_Jolt_HeightFieldRegionUpdateResult;

    // ----------------------------------------------------------------------------------------------------------------
    // Source hashing (staleness detection for cooked data)
    // ----------------------------------------------------------------------------------------------------------------

    /// Cheap hash computable in COOKED builds: mesh asset path + quantized world transform +
    /// instance count. Drives load-time staleness ensures. The filter selects the hashed component
    /// population — it must be the SAME filter the bake ran under (the cooked index's filter hash
    /// guards settings drift at map granularity).
    CKJOLT_API auto ComputeRuntimeCheckHash(
        const AActor& InActor,
        const FCk_Jolt_BakeFilter& InFilter) -> uint64;

#if WITH_EDITOR
    /// Full hash (editor only): BodySetupGuids + trace flags + mesh asset paths + quantized
    /// world transform + ISM instance stream. Drives incremental re-cook decisions.
    CKJOLT_API auto ComputeSourceHash(
        const AActor& InActor,
        const FCk_Jolt_BakeFilter& InFilter) -> uint64;
#endif
}

// --------------------------------------------------------------------------------------------------------------------
