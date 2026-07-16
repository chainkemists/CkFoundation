#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkJolt/CollisionLayers/CkJoltCollisionLayer_Data.h"

#include <PhysicalMaterials/PhysicalMaterial.h>

#include <Jolt/Jolt.h>
#include <Jolt/Core/Reference.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>

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
    /// LevelSweep (streaming/cook path) skips Movable-mobility components — they are
    /// kinematic/dynamic territory. ExplicitActor (Request_BakeActor — the runtime-spawned-static
    /// path and the test surface) bakes them: the caller has declared the actor static-in-intent,
    /// and runtime-spawned actors are necessarily Movable (a Static-mobility component rejects
    /// SetStaticMesh at runtime).
    enum class ECk_Jolt_ExtractionPolicy : uint8
    {
        LevelSweep,
        ExplicitActor
    };

    /// One Jolt body extracted from a source component. Scale is baked into _Shape;
    /// _Position/_Rotation are the body's world transform.
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

    /// One Jolt shape per unique (BodySetupGuid, quantized scale, trace flag) — instanced meshes
    /// share a single JPH::Ref across all their bodies. Not thread-safe; one cache per
    /// extraction pass.
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

    /// Sweeps InActor's registered primitive components and appends one extracted body per
    /// physics representation Chaos would see for a STATIC actor. Returns the number of bodies
    /// appended. Skip rules: unregistered, editor-only/visualization, NoCollision, simulating
    /// physics, Movable mobility. A component with collision enabled but NO valid collision
    /// geometry fires CK_ENSURE and is skipped — never approximated.
    CKJOLT_API auto ExtractActor(
        const AActor& InActor,
        FCk_Jolt_ShapeCache& InShapeCache,
        TArray<FCk_Jolt_ExtractedBody>& OutBodies,
        ECk_Jolt_ExtractionPolicy InPolicy = ECk_Jolt_ExtractionPolicy::LevelSweep) -> int32;

    /// Component-level entry (used by ExtractActor and by tests). Same skip/ensure rules.
    CKJOLT_API auto ExtractComponent(
        const UPrimitiveComponent& InComponent,
        FCk_Jolt_ShapeCache& InShapeCache,
        TArray<FCk_Jolt_ExtractedBody>& OutBodies,
        ECk_Jolt_ExtractionPolicy InPolicy = ECk_Jolt_ExtractionPolicy::LevelSweep) -> int32;

    // ----------------------------------------------------------------------------------------------------------------
    // Shape builders (exposed for tests and for the dynamic-body feature in Phase 3)
    // ----------------------------------------------------------------------------------------------------------------

    /// Builds the Jolt shape for a BodySetup honoring its CollisionTraceFlag:
    /// UseComplexAsSimple -> Chaos cooked tri-mesh; everything else -> AggGeom
    /// (falling back to the tri-mesh only when AggGeom is empty AND a tri-mesh exists AND the
    /// flag permits complex — mirroring what Chaos actually collides against). Returns null
    /// (after CK_ENSURE) when no valid collision representation exists.
    CKJOLT_API auto BuildShape_FromBodySetup(
        const UBodySetup& InBodySetup,
        const FVector& InScale,
        const FString& InDebugName) -> JPH::Ref<JPH::Shape>;

    /// Builds a Jolt heightfield from UE-row-major height samples (world-height units, i.e.
    /// landscape local height x scale.Z already applied). InSampleCount x InSampleCount samples,
    /// indexed [y * InSampleCount + x]; InScaleXY = landscape quad scale (X, Y).
    /// The returned shape is axis-corrected for Ck's Z-up world: Jolt heightfields are local
    /// Y-up spanning X/Z, so we wrap in RotatedTranslatedShape(+90deg about X) with rows flipped
    /// and a -(N-1)*scaleY local-Z offset so the sample grid lands on UE's +X/+Y quadrant.
    /// Surface normals stay up: a heightfield is a graph surface, so sample-order changes never
    /// flip its winding. Pinned by Test_JoltBake_HeightField_KnownHeightsZUp.
    /// Holes: pass HeightFieldNoCollisionValue() as the sample height.
    CKJOLT_API auto CreateHeightFieldShape(
        const TArray<float>& InWorldHeights,
        int32 InSampleCount,
        const FVector2D& InScaleXY) -> JPH::Ref<JPH::Shape>;

    CKJOLT_API auto HeightFieldNoCollisionValue() -> float;

    // ----------------------------------------------------------------------------------------------------------------
    // Source hashing (staleness detection for cooked data)
    // ----------------------------------------------------------------------------------------------------------------

    /// Cheap hash computable in COOKED builds: mesh asset path + quantized world transform +
    /// instance count. Drives load-time staleness ensures.
    CKJOLT_API auto ComputeRuntimeCheckHash(
        const AActor& InActor) -> uint64;

#if WITH_EDITOR
    /// Full hash (editor only): BodySetupGuids + trace flags + mesh asset paths + quantized
    /// world transform + ISM instance stream. Drives incremental re-cook decisions.
    CKJOLT_API auto ComputeSourceHash(
        const AActor& InActor) -> uint64;
#endif
}

// --------------------------------------------------------------------------------------------------------------------
