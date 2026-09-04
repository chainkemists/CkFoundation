#include "CkJoltBakeExtraction.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Ensure/CkEnsure.h"

#include "CkJolt/CkJolt_Log.h"
#include "CkJolt/CkJolt_Utils.h"
#include "CkJolt/Settings/CkJolt_ProjectSettings.h"
#include "CkJolt/StaticWorld/CkJoltMeshShape_Utils.h"

#include <Components/BrushComponent.h>
#include <Components/DynamicMeshComponent.h>
#include <Components/InstancedStaticMeshComponent.h>
#include <Components/SplineMeshComponent.h>
#include <Components/StaticMeshComponent.h>
#include <Engine/StaticMesh.h>
#include <GameFramework/Actor.h>
#include <Hash/xxhash.h>
#include <PhysicsEngine/BodySetup.h>

#include <Chaos/TriangleMeshImplicitObject.h>

#if WITH_EDITOR
#include <Landscape.h>
#include <LandscapeProxy.h>
#include <LandscapeComponent.h>
#include <LandscapeHeightfieldCollisionComponent.h>
#include <LandscapeDataAccess.h>
#endif

#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/HeightFieldShape.h>
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_jolt_bake_extraction
{
    namespace jolt = ck::jolt;

    using namespace ck::jolt;
    using namespace ck::jolt::bake;

    // ----------------------------------------------------------------------------------------------------------------

    static auto Get_QuantizedScale(const FVector& InScale) -> FIntVector
    {
        constexpr auto Quantization = 1000.0;
        return FIntVector{
            static_cast<int32>(FMath::RoundToInt(InScale.X * Quantization)),
            static_cast<int32>(FMath::RoundToInt(InScale.Y * Quantization)),
            static_cast<int32>(FMath::RoundToInt(InScale.Z * Quantization))};
    }

    // ----------------------------------------------------------------------------------------------------------------

    static auto Get_SurfaceProperties(
        const UPrimitiveComponent& InComponent,
        const UBodySetup* InBodySetup,
        FCk_Jolt_ExtractedBody& InOutBody)
        -> void
    {
        auto PhysMaterial = static_cast<UPhysicalMaterial*>(nullptr);

        if (ck::IsValid(InComponent.BodyInstance.GetPhysMaterialOverride()))
        { PhysMaterial = InComponent.BodyInstance.GetPhysMaterialOverride(); }
        else if (ck::IsValid(InBodySetup))
        { PhysMaterial = InBodySetup->GetPhysMaterial(); }

        if (ck::Is_NOT_Valid(PhysMaterial) && ck::IsValid(GEngine))
        { PhysMaterial = GEngine->DefaultPhysMaterial; }

        if (ck::Is_NOT_Valid(PhysMaterial))
        { return; }

        InOutBody._Friction = PhysMaterial->Friction;
        InOutBody._Restitution = PhysMaterial->Restitution;
        InOutBody._SurfaceType = PhysMaterial->SurfaceType;
    }

    // ----------------------------------------------------------------------------------------------------------------

    // Mobility is distinguished from every other exclusion because it is the one a designer trips by
    // accident under a restrictive policy: a floor dragged in as Movable silently vanishes from the
    // static world while Chaos still blocks against it. The stats plumbing exists so every exclusion
    // is countable, not just VeryVerbose-visible.
    enum class ECk_Jolt_ComponentSkipReason : uint8
    {
        NotSkipped,
        NotEligible,
        ExcludedByMobility,
        ExcludedByFilter
    };

    static auto Get_IsMobilityExcluded(
        EComponentMobility::Type InMobility,
        ECk_Jolt_BakeMobilityPolicy InPolicy) -> bool
    {
        switch (InPolicy)
        {
            case ECk_Jolt_BakeMobilityPolicy::All:
                return false;
            case ECk_Jolt_BakeMobilityPolicy::StaticAndStationary:
                return InMobility == EComponentMobility::Movable;
            case ECk_Jolt_BakeMobilityPolicy::StaticOnly:
                return InMobility != EComponentMobility::Static;
            default:
                CK_TRIGGER_ENSURE(TEXT("Invalid ECk_Jolt_BakeMobilityPolicy [{}]"), InPolicy);
                return false;
        }
    }

    static auto Get_BlocksNothing(
        const UPrimitiveComponent& InComponent) -> bool
    {
        const auto& Responses = InComponent.GetCollisionResponseToChannels();

        for (auto ChannelIndex = 0; ChannelIndex < 32; ++ChannelIndex)
        {
            if (Responses.GetResponse(static_cast<ECollisionChannel>(ChannelIndex)) == ECR_Block)
            { return false; }
        }

        return true;
    }

    static auto Get_IsComponentExcludedByFilter(
        const UPrimitiveComponent& InComponent,
        const FCk_Jolt_BakeFilter& InFilter) -> bool
    {
        if (InFilter._ExcludedObjectChannels.Contains(InComponent.GetCollisionObjectType()))
        { return true; }

        if (InFilter._ExcludedCollisionProfiles.Contains(InComponent.GetCollisionProfileName()))
        { return true; }

        if (ck::algo::AnyOf(InComponent.ComponentTags,
            [&](const FName& InTag) { return InFilter._ExcludedComponentTags.Contains(InTag); }))
        { return true; }

        if (InFilter._ExcludeOverlapOnlyComponents == ECk_EnableDisable::Enable &&
            Get_BlocksNothing(InComponent))
        { return true; }

        return false;
    }

    static auto Get_IsActorExcludedByFilter(
        const AActor& InActor,
        const FCk_Jolt_BakeFilter& InFilter) -> bool
    {
        const auto* ActorClass = InActor.GetClass();

        if (ck::algo::AnyOf(InFilter._ExcludedActorClasses,
            [&](const TStrongObjectPtr<UClass>& InExcluded)
            { return InExcluded.IsValid() && ActorClass->IsChildOf(InExcluded.Get()); }))
        { return true; }

        return ck::algo::AnyOf(InActor.Tags,
            [&](const FName& InTag) { return InFilter._ExcludedActorTags.Contains(InTag); });
    }

    static auto Get_ComponentSkipReason(
        const UPrimitiveComponent& InComponent,
        ECk_Jolt_ExtractionPolicy InPolicy,
        const FCk_Jolt_BakeFilter& InFilter) -> ECk_Jolt_ComponentSkipReason
    {
        if (NOT InComponent.IsRegistered())
        { return ECk_Jolt_ComponentSkipReason::NotEligible; }

        if (InComponent.IsEditorOnly())
        { return ECk_Jolt_ComponentSkipReason::NotEligible; }

#if WITH_EDITORONLY_DATA
        if (InComponent.IsVisualizationComponent())
        { return ECk_Jolt_ComponentSkipReason::NotEligible; }
#endif

        if (InComponent.GetCollisionEnabled() == ECollisionEnabled::NoCollision)
        { return ECk_Jolt_ComponentSkipReason::NotEligible; }

        if (InComponent.IsSimulatingPhysics())
        { return ECk_Jolt_ComponentSkipReason::NotEligible; }

        if (InPolicy != ECk_Jolt_ExtractionPolicy::LevelSweep)
        { return ECk_Jolt_ComponentSkipReason::NotSkipped; }

        if (Get_IsMobilityExcluded(InComponent.Mobility, InFilter._MobilityPolicy))
        {
            // Engine enum — no Ck formatter; format the raw mobility value.
            ck::jolt::VeryVerbose(TEXT("Static bake skipping component [{}] — mobility [{}] excluded by policy [{}]"),
                InComponent.GetName(), static_cast<int32>(InComponent.Mobility), InFilter._MobilityPolicy);
            return ECk_Jolt_ComponentSkipReason::ExcludedByMobility;
        }

        if (Get_IsComponentExcludedByFilter(InComponent, InFilter))
        {
            ck::jolt::VeryVerbose(TEXT("Static bake skipping component [{}] — excluded by bake-filter settings"),
                InComponent.GetName());
            return ECk_Jolt_ComponentSkipReason::ExcludedByFilter;
        }

        return ECk_Jolt_ComponentSkipReason::NotSkipped;
    }

    static auto Get_ShouldSkipComponent(
        const UPrimitiveComponent& InComponent,
        ECk_Jolt_ExtractionPolicy InPolicy,
        const FCk_Jolt_BakeFilter& InFilter) -> bool
    {
        return Get_ComponentSkipReason(InComponent, InPolicy, InFilter) != ECk_Jolt_ComponentSkipReason::NotSkipped;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // AggGeom -> Jolt
    // ----------------------------------------------------------------------------------------------------------------

    struct FLeafShape
    {
        JPH::Ref<JPH::Shape> _Shape;
        FVector _LocalPosition = FVector::ZeroVector;
        FQuat _LocalRotation = FQuat::Identity;
    };

    static auto Get_IsIdentityLocal(const FLeafShape& InLeaf) -> bool
    {
        return InLeaf._LocalPosition.IsNearlyZero(KINDA_SMALL_NUMBER)
            && InLeaf._LocalRotation.Equals(FQuat::Identity);
    }

    static auto Create_ShapeFromSettings(
        const JPH::ShapeSettings& InSettings,
        const FString& InDebugName)
        -> JPH::Ref<JPH::Shape>
    {
        const auto Result = InSettings.Create();

        CK_ENSURE_IF_NOT(Result.IsValid(), TEXT("Jolt shape creation FAILED for [{}]: [{}]"),
            InDebugName, FString{Result.GetError().c_str()})
        { return {}; }

        return Result.Get();
    }

    // Jolt requires >= 2 children for a compound shape.
    static auto Combine_Leaves(
        TArray<FLeafShape>&& InLeaves,
        const FString& InDebugName)
        -> JPH::Ref<JPH::Shape>
    {
        if (InLeaves.IsEmpty())
        { return {}; }

        if (InLeaves.Num() == 1)
        {
            auto& Leaf = InLeaves[0];

            if (Get_IsIdentityLocal(Leaf))
            { return Leaf._Shape; }

            const auto Settings = JPH::RotatedTranslatedShapeSettings{
                jolt::Conv(Leaf._LocalPosition), jolt::Conv(Leaf._LocalRotation), Leaf._Shape.GetPtr()};
            return Create_ShapeFromSettings(Settings, InDebugName);
        }

        auto CompoundSettings = JPH::StaticCompoundShapeSettings{};
        for (const auto& Leaf : InLeaves)
        {
            CompoundSettings.AddShape(jolt::Conv(Leaf._LocalPosition), jolt::Conv(Leaf._LocalRotation),
                Leaf._Shape.GetPtr());
        }
        return Create_ShapeFromSettings(CompoundSettings, InDebugName);
    }

    static auto Build_AggGeomLeaves(
        const UBodySetup& InBodySetup,
        const FVector& InScale,
        const FString& InDebugName,
        TArray<FLeafShape>& OutLeaves)
        -> bool
    {
        const auto& AggGeom = InBodySetup.AggGeom;
        const auto AbsScale = InScale.GetAbs();

        for (const auto& BoxElem : AggGeom.BoxElems)
        {
            const auto HalfExtents = FVector{BoxElem.X * AbsScale.X, BoxElem.Y * AbsScale.Y, BoxElem.Z * AbsScale.Z} * 0.5;
            const auto JoltHalfExtents = jolt::Conv(HalfExtents);
            const auto HalfExtentsArePositive = FMath::IsFinite(JoltHalfExtents.GetX())
                && FMath::IsFinite(JoltHalfExtents.GetY())
                && FMath::IsFinite(JoltHalfExtents.GetZ())
                && JoltHalfExtents.GetX() > 0.0f
                && JoltHalfExtents.GetY() > 0.0f
                && JoltHalfExtents.GetZ() > 0.0f;

            CK_ENSURE_IF_NOT(HalfExtentsArePositive,
                TEXT("Jolt box collision is INVALID for [{}]: raw dimensions [{}, {}, {}], "
                     "component scale [{}], scaled half extents [{}], Jolt half extents [{}, {}, {}]. "
                     "Every Jolt half extent must be finite and positive."),
                InDebugName, BoxElem.X, BoxElem.Y, BoxElem.Z, InScale, HalfExtents,
                JoltHalfExtents.GetX(), JoltHalfExtents.GetY(), JoltHalfExtents.GetZ())
            { return false; }

            constexpr auto MaximumConvexRadiusFraction = 0.5f;
            const auto MinimumHalfExtent = JoltHalfExtents.ReduceMin();
            const auto ConvexRadius = FMath::Min(
                JPH::cDefaultConvexRadius, MinimumHalfExtent * MaximumConvexRadiusFraction);
            const auto Settings = JPH::BoxShapeSettings{JoltHalfExtents, ConvexRadius};
            auto Shape = Create_ShapeFromSettings(Settings, InDebugName);
            if (ck::Is_NOT_Valid(Shape))
            { return false; }

            OutLeaves.Emplace(FLeafShape{Shape, BoxElem.Center * InScale, BoxElem.Rotation.Quaternion()});
        }

        for (const auto& SphereElem : AggGeom.SphereElems)
        {
            const auto Radius = SphereElem.Radius * AbsScale.GetMin();

            const auto Settings = JPH::SphereShapeSettings{static_cast<float>(Radius)};
            auto Shape = Create_ShapeFromSettings(Settings, InDebugName);
            if (ck::Is_NOT_Valid(Shape))
            { continue; }

            OutLeaves.Emplace(FLeafShape{Shape, SphereElem.Center * InScale, FQuat::Identity});
        }

        for (const auto& SphylElem : AggGeom.SphylElems)
        {
            // UE capsules are Z-aligned, Jolt capsules Y-aligned — hence the Y->Z axis correction below.
            const auto Radius = SphylElem.GetScaledRadius(AbsScale);
            const auto CylinderHalfLength = SphylElem.GetScaledCylinderLength(AbsScale) * 0.5;

            const auto CapsuleSettings = JPH::CapsuleShapeSettings{
                static_cast<float>(CylinderHalfLength), static_cast<float>(Radius)};
            auto CapsuleShape = Create_ShapeFromSettings(CapsuleSettings, InDebugName);
            if (ck::Is_NOT_Valid(CapsuleShape))
            { continue; }

            const auto UprightSettings = JPH::RotatedTranslatedShapeSettings{
                JPH::Vec3::sZero(), jolt::Get_ShapeAxisCorrection_YToZ(), CapsuleShape.GetPtr()};
            auto UprightShape = Create_ShapeFromSettings(UprightSettings, InDebugName);
            if (ck::Is_NOT_Valid(UprightShape))
            { continue; }

            OutLeaves.Emplace(FLeafShape{UprightShape, SphylElem.Center * InScale, SphylElem.Rotation.Quaternion()});
        }

        for (const auto& ConvexElem : AggGeom.ConvexElems)
        {
            const auto& ElemTransform = ConvexElem.GetTransform();

            auto Points = JPH::Array<JPH::Vec3>{};
            Points.reserve(ConvexElem.VertexData.Num());
            for (const auto& Vertex : ConvexElem.VertexData)
            {
                // Baked into the points so non-uniform scale composes correctly with elem rotation.
                const auto TransformedVertex = ElemTransform.TransformPosition(Vertex) * InScale;
                Points.push_back(jolt::Conv(TransformedVertex));
            }

            const auto Settings = JPH::ConvexHullShapeSettings{Points};
            auto Shape = Create_ShapeFromSettings(Settings, InDebugName);
            if (ck::Is_NOT_Valid(Shape))
            { continue; }

            OutLeaves.Emplace(FLeafShape{Shape, FVector::ZeroVector, FQuat::Identity});
        }

        return true;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Chaos cooked tri-mesh -> Jolt MeshShape
    // ----------------------------------------------------------------------------------------------------------------

    struct FTriangleEdgeUse
    {
        int32 _TriangleIndex = INDEX_NONE;
        uint32 _StartVertex = 0;
        uint32 _EndVertex = 0;
    };

    static auto Get_UndirectedEdgeKey(uint32 InFirstVertex, uint32 InSecondVertex) -> uint64
    {
        const auto Lower = FMath::Min(InFirstVertex, InSecondVertex);
        const auto Upper = FMath::Max(InFirstVertex, InSecondVertex);
        return (static_cast<uint64>(Lower) << 32) | static_cast<uint64>(Upper);
    }

    static auto Get_ComponentWindingRatio(
        const JPH::VertexList& InVertices,
        const JPH::IndexedTriangleList& InTriangles,
        const TArray<int32>& InComponentTriangleIndices) -> double
    {
        auto ComponentVertexIndices = TSet<uint32>{};

        for (const auto TriangleIndex : InComponentTriangleIndices)
        {
            const auto& Indices = InTriangles[TriangleIndex].mIdx;
            ComponentVertexIndices.Add(Indices[0]);
            ComponentVertexIndices.Add(Indices[1]);
            ComponentVertexIndices.Add(Indices[2]);
        }

        auto BoundsMin = FVector{TNumericLimits<double>::Max()};
        auto BoundsMax = FVector{TNumericLimits<double>::Lowest()};

        for (const auto VertexIndex : ComponentVertexIndices)
        {
            const auto& Vertex = InVertices[VertexIndex];
            BoundsMin.X = FMath::Min(BoundsMin.X, static_cast<double>(Vertex.x));
            BoundsMin.Y = FMath::Min(BoundsMin.Y, static_cast<double>(Vertex.y));
            BoundsMin.Z = FMath::Min(BoundsMin.Z, static_cast<double>(Vertex.z));
            BoundsMax.X = FMath::Max(BoundsMax.X, static_cast<double>(Vertex.x));
            BoundsMax.Y = FMath::Max(BoundsMax.Y, static_cast<double>(Vertex.y));
            BoundsMax.Z = FMath::Max(BoundsMax.Z, static_cast<double>(Vertex.z));
        }

        const auto BoundsSize = BoundsMax - BoundsMin;
        constexpr auto MinJudgeableExtent = 0.1;
        if (BoundsSize.GetMin() <= MinJudgeableExtent)
        { return 0.0; }

        const auto BoundsVolume = BoundsSize.X * BoundsSize.Y * BoundsSize.Z;
        const auto Center = (BoundsMin + BoundsMax) * 0.5;
        auto SignedVolumeSum = 0.0;

        for (const auto TriangleIndex : InComponentTriangleIndices)
        {
            const auto& Indices = InTriangles[TriangleIndex].mIdx;
            const auto ToCenteredVector = [&](uint32 InVertexIndex) -> FVector
            {
                const auto& Vertex = InVertices[InVertexIndex];
                return FVector{Vertex.x, Vertex.y, Vertex.z} - Center;
            };
            const auto A = ToCenteredVector(Indices[0]);
            const auto B = ToCenteredVector(Indices[1]);
            const auto C = ToCenteredVector(Indices[2]);
            SignedVolumeSum += FVector::DotProduct(A, FVector::CrossProduct(B, C));
        }

        return (SignedVolumeSum / 6.0) / BoundsVolume;
    }

    static auto Get_AreTriangleIndicesValid(
        const JPH::VertexList& InVertices,
        const JPH::IndexedTriangle& InTriangle) -> bool
    {
        const auto& Indices = InTriangle.mIdx;
        const auto NumVertices = static_cast<uint32>(InVertices.size());

        if (Indices[0] >= NumVertices || Indices[1] >= NumVertices || Indices[2] >= NumVertices
            || Indices[0] == Indices[1] || Indices[1] == Indices[2] || Indices[2] == Indices[0])
        { return false; }

        return true;
    }

    static auto Get_IsTriangleGeometricallyValid(
        const JPH::VertexList& InVertices,
        const JPH::IndexedTriangle& InTriangle) -> bool
    {
        const auto& Indices = InTriangle.mIdx;

        const auto ToVector = [&](uint32 InVertexIndex) -> FVector
        {
            const auto& Vertex = InVertices[InVertexIndex];
            return FVector{Vertex.x, Vertex.y, Vertex.z};
        };

        const auto A = ToVector(Indices[0]);
        const auto B = ToVector(Indices[1]);
        const auto C = ToVector(Indices[2]);
        const auto HasFiniteVertices = NOT A.ContainsNaN() && NOT B.ContainsNaN() && NOT C.ContainsNaN();
        const auto TwiceAreaSquared = FVector::CrossProduct(B - A, C - A).SizeSquared();

        return HasFiniteVertices && TwiceAreaSquared > TNumericLimits<double>::Min();
    }

    static auto Build_TriMeshShape(
        const UBodySetup& InBodySetup,
        const FVector& InScale,
        const FString& InDebugName)
        -> JPH::Ref<JPH::Shape>
    {
        CK_ENSURE_IF_NOT(InBodySetup.TriMeshGeometries.Num() > 0,
            TEXT("No cooked tri-mesh on BodySetup for [{}] — complex collision cannot be baked. "
                 "Re-cook the asset or author simple collision."), InDebugName)
        { return {}; }

        const auto& TriMesh = InBodySetup.TriMeshGeometries[0];

        CK_ENSURE_IF_NOT(TriMesh.IsValid(), TEXT("Cooked tri-mesh on BodySetup for [{}] is INVALID"), InDebugName)
        { return {}; }

        const auto& Particles = TriMesh->Particles();
        const auto& Elements = TriMesh->Elements();
        const auto NumVerts = static_cast<int32>(Particles.Size());
        const auto NumTris = Elements.GetNumTriangles();

        auto Vertices = JPH::VertexList{};
        Vertices.reserve(NumVerts);

        for (auto Index = 0; Index < NumVerts; ++Index)
        {
            const auto& Particle = Particles.GetX(Index);
            Vertices.push_back(JPH::Float3(
                Particle[0] * static_cast<float>(InScale.X),
                Particle[1] * static_cast<float>(InScale.Y),
                Particle[2] * static_cast<float>(InScale.Z)));
        }

        auto Triangles = JPH::IndexedTriangleList{};
        Triangles.reserve(NumTris);

        // Chaos and Jolt AGREE on triangle winding — copy indices as-is. UE authors front faces
        // LEFT-handed (VectorUtil::Normal reverses its cross product, saying why), and every
        // collision provider marks its raw triangles bFlipNormals=true so the Chaos cook stores
        // them RIGHT-handed — which is exactly Jolt's front-face convention. This code shipped for
        // months with an extra b/c swap here that flipped the stored winding BACK to left-handed,
        // baking every tri-mesh INSIDE-OUT (proven by Ck.Jolt.Bake.StaticMesh.
        // EngineCubeTriMeshBakesOutward measuring the canonical engine cube at ratio -1). The
        // DynamicMesh winding specs did not catch it because their fixtures were authored under
        // the same reversed assumption — two errors cancelling; they are now authored in UE
        // convention and pin the corrected chain.
        //
        // A MIRRORING scale (negative determinant — odd count of negative components) flips the
        // handedness of the vertices baked above, so the index order must flip WITH it to keep the
        // final orientation outward. The pre-baked ScaledShape path never needs this (Jolt's
        // ScaleHelpers handles inside-out scale internally).
        const bool ScaleIsInsideOut = (InScale.X * InScale.Y * InScale.Z) < 0.0;
        auto HasInvalidTriangleIndices = false;
        const auto PushTriangle = [&](uint32 InA, uint32 InB, uint32 InC) -> void
        {
            if (InA < static_cast<uint32>(NumVerts) && InB < static_cast<uint32>(NumVerts) && InC < static_cast<uint32>(NumVerts))
            {
                if (ScaleIsInsideOut)
                { Triangles.push_back(JPH::IndexedTriangle(InA, InC, InB)); }
                else
                { Triangles.push_back(JPH::IndexedTriangle(InA, InB, InC)); }
            }
            else
            { HasInvalidTriangleIndices = true; }
        };

        if (Elements.RequiresLargeIndices())
        {
            for (const auto& Triangle : Elements.GetLargeIndexBuffer())
            { PushTriangle(Triangle[0], Triangle[1], Triangle[2]); }
        }
        else
        {
            for (const auto& Triangle : Elements.GetSmallIndexBuffer())
            { PushTriangle(Triangle[0], Triangle[1], Triangle[2]); }
        }

        CK_ENSURE_IF_NOT(NOT HasInvalidTriangleIndices,
            TEXT("Cooked tri-mesh for [{}] contains an out-of-range vertex index — refusing to bake collision."),
            InDebugName)
        { }

        if (HasInvalidTriangleIndices)
        { return {}; }

        // A strongly negative volume verdict is sufficient to reverse a geometrically valid connected
        // component, including open or non-manifold topology. Bad indices remain fail-closed because a
        // component walk or index swap cannot safely proceed from them.
        const auto Normalization = NormalizeInsideOutMeshComponents(Vertices, Triangles);
        if (Normalization._NumRepairedComponents > 0)
        {
            ck::jolt::Warning(TEXT("Tri-mesh source [{}] had {} inside-out component(s) normalized "
                "during the Jolt bake ({} healthy, {} no-verdict, {} open, {} non-manifold, {} inconsistent, "
                "{} malformed)."),
                InDebugName, Normalization._NumRepairedComponents, Normalization._NumHealthyComponents,
                Normalization._NumNoVerdictComponents, Normalization._NumOpenComponents,
                Normalization._NumNonManifoldComponents, Normalization._NumInconsistentComponents,
                Normalization._NumMalformedComponents);
        }

        constexpr auto InsideOutWindingRatioThreshold = -0.05;
        const auto WindingRatio = ComputeMeshWindingRatio(Vertices, Triangles);
        const auto IsStillInsideOut = WindingRatio < InsideOutWindingRatioThreshold;
        const auto HasMalformedIndices = Normalization.Get_HasMalformedIndices();
        const auto WindingIsSafe = NOT IsStillInsideOut && NOT HasMalformedIndices;

        CK_ENSURE_IF_NOT(WindingIsSafe,
            TEXT("Tri-mesh for [{}] cannot safely normalize its winding (signed-volume/bounds ratio [{}], "
                 "{} malformed-index component(s)): refusing to bake single-sided collision. Correct the "
                 "source indices or triangle winding."),
            InDebugName, WindingRatio, Normalization._NumMalformedIndexComponents)
        { }

        if (NOT WindingIsSafe)
        { return {}; }

        const auto Settings = JPH::MeshShapeSettings{Vertices, Triangles};
        return Create_ShapeFromSettings(Settings, InDebugName);
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::jolt::bake
{
    using namespace ck_jolt_bake_extraction;

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCk_Jolt_BakeFilter::
        Make_FromProjectSettings()
        -> FCk_Jolt_BakeFilter
    {
        auto Filter = FCk_Jolt_BakeFilter{};
        Filter._MobilityPolicy = UCk_Utils_Jolt_ProjectSettings::Get_BakeMobilityPolicy();
        Filter._ExcludedActorTags = UCk_Utils_Jolt_ProjectSettings::Get_BakeExcludedActorTags();
        Filter._ExcludedComponentTags = UCk_Utils_Jolt_ProjectSettings::Get_BakeExcludedComponentTags();
        Filter._ExcludedObjectChannels = UCk_Utils_Jolt_ProjectSettings::Get_BakeExcludedObjectChannels();
        Filter._ExcludedCollisionProfiles = UCk_Utils_Jolt_ProjectSettings::Get_BakeExcludedCollisionProfiles();
        Filter._ExcludeOverlapOnlyComponents = UCk_Utils_Jolt_ProjectSettings::Get_BakeExcludeOverlapOnlyComponents();

        for (const auto& SoftClass : UCk_Utils_Jolt_ProjectSettings::Get_BakeExcludedActorClasses())
        {
            auto* Resolved = SoftClass.IsNull() ? nullptr : SoftClass.LoadSynchronous();

            CK_ENSURE_IF_NOT(ck::IsValid(Resolved),
                TEXT("Bake-filter excluded actor class [{}] is unset or failed to load — the entry is IGNORED. "
                     "Fix or remove the row in Project Settings > Jolt > Bake Filter."), SoftClass.ToString())
            {}
            if (ck::Is_NOT_Valid(Resolved))
            { continue; }

            Filter._ExcludedActorClasses.Emplace(TStrongObjectPtr{Resolved});
        }

        return Filter;
    }

    auto
        FCk_Jolt_BakeFilter::
        ComputeHash() const
        -> uint64
    {
        auto Builder = FXxHash64Builder{};

        const auto HashByte = [&](uint8 InValue) -> void
        {
            Builder.Update(&InValue, sizeof(InValue));
        };

        // FNames are case-insensitive — hash the lowercase string, with a separator byte so
        // adjacent entries cannot alias by concatenation.
        const auto HashString = [&](const FString& InString) -> void
        {
            const auto Lower = InString.ToLower();
            Builder.Update(*Lower, Lower.Len() * sizeof(TCHAR));
            HashByte(0);
        };

        const auto HashSortedNames = [&](const TArray<FName>& InNames) -> void
        {
            auto Sorted = ck::algo::Transform<TArray<FString>>(InNames,
                [](const FName& InName) { return InName.ToString().ToLower(); });
            Sorted.Sort();

            for (const auto& Name : Sorted)
            { HashString(Name); }
            HashByte(0xFF);
        };

        HashByte(static_cast<uint8>(_MobilityPolicy));

        auto ClassPaths = ck::algo::Transform<TArray<FString>>(_ExcludedActorClasses,
            [](const TStrongObjectPtr<UClass>& InClass)
            { return InClass.IsValid() ? InClass->GetPathName() : FString{}; });
        ClassPaths.Sort();
        for (const auto& ClassPath : ClassPaths)
        { HashString(ClassPath); }
        HashByte(0xFF);

        HashSortedNames(_ExcludedActorTags);
        HashSortedNames(_ExcludedComponentTags);

        auto Channels = ck::algo::Transform<TArray<uint8>>(_ExcludedObjectChannels,
            [](const TEnumAsByte<ECollisionChannel>& InChannel) { return static_cast<uint8>(InChannel.GetValue()); });
        Channels.Sort();
        for (const auto& Channel : Channels)
        { HashByte(Channel); }
        HashByte(0xFF);

        HashSortedNames(_ExcludedCollisionProfiles);

        HashByte(static_cast<uint8>(_ExcludeOverlapOnlyComponents));

        return Builder.Finalize().Hash;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_IsComponentExcludedByBakeFilter(
            const UPrimitiveComponent& InComponent,
            const FCk_Jolt_BakeFilter& InFilter)
        -> bool
    {
        return Get_IsComponentExcludedByFilter(InComponent, InFilter);
    }

    auto
        FCk_Jolt_ShapeCache::
        GetOrCreate_Shape(
            const UBodySetup& InBodySetup,
            const FVector& InScale,
            const FString& InDebugName)
        -> JPH::Ref<JPH::Shape>
    {
        const auto Key = FKey{InBodySetup.BodySetupGuid, Get_QuantizedScale(InScale),
            static_cast<uint8>(InBodySetup.GetCollisionTraceFlag())};

        if (const auto* Existing = _Shapes.Find(Key))
        { return *Existing; }

        // Pre-baked path first: a mesh BodySetup's outer IS the UStaticMesh, and its cooked scale-1
        // shape (when present, valid and scalable to InScale) skips the hull/tri-mesh build
        // entirely. Brush/spline BodySetups have different outers and per-instance geometry — they
        // always build. A null wrap (missing asset, stale, or scale invalid for the topology) falls
        // through to the build, which bakes the scale into the geometry as this path always has.
        auto Shape = JPH::Ref<JPH::Shape>{};

        if (const auto* Mesh = Cast<UStaticMesh>(InBodySetup.GetOuter()))
        {
            const auto ScaleOneShape = mesh_shape_utils::TryGet_ScaleOneShape(*Mesh);
            Shape = mesh_shape_utils::TryWrap_AtScale(ScaleOneShape, InScale, InDebugName);
        }

        if (ck::Is_NOT_Valid(Shape))
        { Shape = BuildShape_FromBodySetup(InBodySetup, InScale, InDebugName); }

        if (ck::IsValid(Shape))
        { _Shapes.Add(Key, Shape); }

        return Shape;
    }

    auto
        FCk_Jolt_ShapeCache::
        Get_NumUniqueShapes() const
        -> int32
    {
        return _Shapes.Num();
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        BuildShape_FromBodySetup(
            const UBodySetup& InBodySetup,
            const FVector& InScale,
            const FString& InDebugName)
        -> JPH::Ref<JPH::Shape>
    {
        const auto TraceFlag = InBodySetup.GetCollisionTraceFlag();

        if (TraceFlag == CTF_UseComplexAsSimple)
        { return Build_TriMeshShape(InBodySetup, InScale, InDebugName); }

        auto Leaves = TArray<FLeafShape>{};
        const auto AggGeomIsValid = Build_AggGeomLeaves(InBodySetup, InScale, InDebugName, Leaves);
        if (NOT AggGeomIsValid)
        { return {}; }

        if (Leaves.IsEmpty())
        {
            // Chaos still collides against the cooked tri-mesh here — mirror that, Verbose not ensure.
            if (InBodySetup.TriMeshGeometries.Num() > 0 && TraceFlag != CTF_UseSimpleAsComplex)
            {
                ck::jolt::Verbose(TEXT("BodySetup for [{}] has no simple collision — baking the cooked tri-mesh "
                    "(matches what Chaos collides against)"), InDebugName);
                return Build_TriMeshShape(InBodySetup, InScale, InDebugName);
            }

            CK_TRIGGER_ENSURE(TEXT("BodySetup for [{}] has collision enabled but NO valid collision geometry "
                "(no AggGeom elements, no usable tri-mesh). Baked NOTHING — fix the asset's collision. "
                "A bounding-box substitute would hide the problem, so none is made."), InDebugName);
            return {};
        }

        return Combine_Leaves(MoveTemp(Leaves), InDebugName);
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        NormalizeInsideOutMeshComponents(
            const JPH::VertexList& InVertices,
            JPH::IndexedTriangleList& InOutTriangles)
        -> FCk_Jolt_WindingNormalizationResult
    {
        constexpr auto InsideOutWindingRatioThreshold = -0.05;
        constexpr auto OutwardWindingRatioThreshold = 0.05;

        auto Result = FCk_Jolt_WindingNormalizationResult{};
        if (InVertices.empty() || InOutTriangles.empty())
        { return Result; }

        // Index validity is the one precondition that cannot be recovered locally: any attempt to
        // discover components or swap indices after it fails could address arbitrary memory. Reject
        // the whole normalization atomically before touching a triangle.
        auto HasMalformedIndices = false;
        for (const auto& Triangle : InOutTriangles)
        {
            if (NOT Get_AreTriangleIndicesValid(InVertices, Triangle))
            {
                HasMalformedIndices = true;
                ++Result._NumComponents;
                ++Result._NumMalformedComponents;
                ++Result._NumMalformedIndexComponents;
            }
        }

        if (HasMalformedIndices)
        {
            Result._Status = ECk_Jolt_WindingNormalizationStatus::Malformed;
            return Result;
        }

        auto EdgeUsesByKey = TMap<uint64, TArray<FTriangleEdgeUse>>{};
        auto ValidTriangles = TArray<bool>{};
        ValidTriangles.Init(false, static_cast<int32>(InOutTriangles.size()));

        for (auto TriangleIndex = 0; TriangleIndex < static_cast<int32>(InOutTriangles.size()); ++TriangleIndex)
        {
            const auto& Triangle = InOutTriangles[TriangleIndex];
            if (NOT Get_IsTriangleGeometricallyValid(InVertices, Triangle))
            {
                ++Result._NumComponents;
                ++Result._NumMalformedComponents;
                continue;
            }

            ValidTriangles[TriangleIndex] = true;
            const auto& Indices = Triangle.mIdx;
            const auto AddEdgeUse = [&](uint32 InStartVertex, uint32 InEndVertex) -> void
            {
                EdgeUsesByKey.FindOrAdd(Get_UndirectedEdgeKey(InStartVertex, InEndVertex)).Add(
                    FTriangleEdgeUse{TriangleIndex, InStartVertex, InEndVertex});
            };

            AddEdgeUse(Indices[0], Indices[1]);
            AddEdgeUse(Indices[1], Indices[2]);
            AddEdgeUse(Indices[2], Indices[0]);
        }

        auto VisitedTriangles = TArray<bool>{};
        VisitedTriangles.Init(false, static_cast<int32>(InOutTriangles.size()));

        const auto NumTriangles = static_cast<int32>(InOutTriangles.size());
        for (auto StartTriangleIndex = 0; StartTriangleIndex < NumTriangles; ++StartTriangleIndex)
        {
            if (NOT ValidTriangles[StartTriangleIndex] || VisitedTriangles[StartTriangleIndex])
            { continue; }

            auto ComponentTriangleIndices = TArray<int32>{StartTriangleIndex};
            VisitedTriangles[StartTriangleIndex] = true;

            for (auto QueueIndex = 0; QueueIndex < ComponentTriangleIndices.Num(); ++QueueIndex)
            {
                const auto& Indices = InOutTriangles[ComponentTriangleIndices[QueueIndex]].mIdx;
                const auto VisitEdge = [&](uint32 InStartVertex, uint32 InEndVertex) -> void
                {
                    const auto EdgeKey = Get_UndirectedEdgeKey(InStartVertex, InEndVertex);
                    const auto& EdgeUses = EdgeUsesByKey.FindChecked(EdgeKey);
                    for (const auto& EdgeUse : EdgeUses)
                    {
                        if (NOT VisitedTriangles[EdgeUse._TriangleIndex])
                        {
                            VisitedTriangles[EdgeUse._TriangleIndex] = true;
                            ComponentTriangleIndices.Add(EdgeUse._TriangleIndex);
                        }
                    }
                };

                VisitEdge(Indices[0], Indices[1]);
                VisitEdge(Indices[1], Indices[2]);
                VisitEdge(Indices[2], Indices[0]);
            }

            ++Result._NumComponents;
            auto IsClosed = true;
            auto IsManifold = true;
            auto IsConsistentlyOriented = true;

            for (const auto TriangleIndex : ComponentTriangleIndices)
            {
                const auto& Indices = InOutTriangles[TriangleIndex].mIdx;
                const auto AssessEdge = [&](uint32 InStartVertex, uint32 InEndVertex) -> void
                {
                    const auto EdgeKey = Get_UndirectedEdgeKey(InStartVertex, InEndVertex);
                    const auto& EdgeUses = EdgeUsesByKey.FindChecked(EdgeKey);
                    if (EdgeUses.Num() < 2)
                    {
                        IsClosed = false;
                        return;
                    }
                    if (EdgeUses.Num() > 2)
                    {
                        IsManifold = false;
                        return;
                    }

                    const auto& FirstUse = EdgeUses[0];
                    const auto& SecondUse = EdgeUses[1];
                    if (FirstUse._StartVertex != SecondUse._EndVertex || FirstUse._EndVertex != SecondUse._StartVertex)
                    { IsConsistentlyOriented = false; }
                };

                AssessEdge(Indices[0], Indices[1]);
                AssessEdge(Indices[1], Indices[2]);
                AssessEdge(Indices[2], Indices[0]);
            }

            if (NOT IsClosed)
            { ++Result._NumOpenComponents; }
            if (NOT IsManifold)
            { ++Result._NumNonManifoldComponents; }
            if (NOT IsConsistentlyOriented)
            { ++Result._NumInconsistentComponents; }

            const auto WindingRatio = Get_ComponentWindingRatio(
                InVertices, InOutTriangles, ComponentTriangleIndices);
            if (WindingRatio < InsideOutWindingRatioThreshold)
            {
                for (const auto TriangleIndex : ComponentTriangleIndices)
                { Swap(InOutTriangles[TriangleIndex].mIdx[1], InOutTriangles[TriangleIndex].mIdx[2]); }
                ++Result._NumRepairedComponents;
            }
            else if (WindingRatio > OutwardWindingRatioThreshold)
            { ++Result._NumHealthyComponents; }
            else
            { ++Result._NumNoVerdictComponents; }
        }

        if (Result._NumRepairedComponents > 0)
        { Result._Status = ECk_Jolt_WindingNormalizationStatus::Normalized; }
        else if (Result._NumHealthyComponents > 0)
        { Result._Status = ECk_Jolt_WindingNormalizationStatus::Unchanged; }

        return Result;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        ComputeMeshWindingRatio(
            const JPH::VertexList& InVertices,
            const JPH::IndexedTriangleList& InTriangles)
        -> double
    {
        if (InVertices.empty() || InTriangles.empty())
        { return 0.0; }

        auto BoundsMin = FVector{TNumericLimits<double>::Max()};
        auto BoundsMax = FVector{TNumericLimits<double>::Lowest()};

        for (const auto& Vertex : InVertices)
        {
            BoundsMin.X = FMath::Min(BoundsMin.X, static_cast<double>(Vertex.x));
            BoundsMin.Y = FMath::Min(BoundsMin.Y, static_cast<double>(Vertex.y));
            BoundsMin.Z = FMath::Min(BoundsMin.Z, static_cast<double>(Vertex.z));
            BoundsMax.X = FMath::Max(BoundsMax.X, static_cast<double>(Vertex.x));
            BoundsMax.Y = FMath::Max(BoundsMax.Y, static_cast<double>(Vertex.y));
            BoundsMax.Z = FMath::Max(BoundsMax.Z, static_cast<double>(Vertex.z));
        }

        // Thinner than 1mm in any axis is effectively an open sheet — no volume verdict exists, and
        // dividing by a near-zero bounds volume would manufacture one out of float noise.
        const auto BoundsSize = BoundsMax - BoundsMin;
        constexpr auto MinJudgeableExtent = 0.1;
        if (BoundsSize.GetMin() <= MinJudgeableExtent)
        { return 0.0; }

        const auto BoundsVolume = BoundsSize.X * BoundsSize.Y * BoundsSize.Z;
        const auto Center = (BoundsMin + BoundsMax) * 0.5;
        const auto NumVertices = static_cast<uint32>(InVertices.size());

        auto SignedVolumeSum = 0.0;

        for (const auto& Triangle : InTriangles)
        {
            const auto& Indices = Triangle.mIdx;
            if (Indices[0] >= NumVertices || Indices[1] >= NumVertices || Indices[2] >= NumVertices)
            { continue; }

            const auto A = FVector{InVertices[Indices[0]].x, InVertices[Indices[0]].y, InVertices[Indices[0]].z} - Center;
            const auto B = FVector{InVertices[Indices[1]].x, InVertices[Indices[1]].y, InVertices[Indices[1]].z} - Center;
            const auto C = FVector{InVertices[Indices[2]].x, InVertices[Indices[2]].y, InVertices[Indices[2]].z} - Center;

            SignedVolumeSum += FVector::DotProduct(A, FVector::CrossProduct(B, C));
        }

        return (SignedVolumeSum / 6.0) / BoundsVolume;
    }

    auto
        ComputeShapeWindingRatio(
            const JPH::Shape& InShape)
        -> double
    {
        if (InShape.GetSubType() != JPH::EShapeSubType::Mesh)
        { return 0.0; }

        const auto Bounds = InShape.GetLocalBounds();
        const auto BoundsSize = Bounds.GetSize();

        constexpr auto MinJudgeableExtent = 0.1f;
        if (BoundsSize.ReduceMin() <= MinJudgeableExtent)
        { return 0.0; }

        const auto BoundsVolume = static_cast<double>(BoundsSize.GetX()) *
            static_cast<double>(BoundsSize.GetY()) * static_cast<double>(BoundsSize.GetZ());
        const auto Center = Bounds.GetCenter();

        auto Context = JPH::Shape::GetTrianglesContext{};
        InShape.GetTrianglesStart(Context, JPH::AABox::sBiggest(),
            JPH::Vec3::sZero(), JPH::Quat::sIdentity(), JPH::Vec3::sReplicate(1.0f));

        constexpr auto BatchSize = 256;
        static_assert(BatchSize >= JPH::Shape::cGetTrianglesMinTrianglesRequested);

        auto TriangleVertices = TArray<JPH::Float3>{};
        TriangleVertices.SetNumUninitialized(BatchSize * 3);

        auto SignedVolumeSum = 0.0;

        for (;;)
        {
            const auto NumTriangles = InShape.GetTrianglesNext(Context, BatchSize, TriangleVertices.GetData());
            if (NumTriangles <= 0)
            { break; }

            const auto ToCentered = [&](const JPH::Float3& InVertex) -> FVector
            {
                return FVector{
                    InVertex.x - Center.GetX(), InVertex.y - Center.GetY(), InVertex.z - Center.GetZ()};
            };

            for (auto Index = 0; Index < NumTriangles; ++Index)
            {
                const auto A = ToCentered(TriangleVertices[Index * 3 + 0]);
                const auto B = ToCentered(TriangleVertices[Index * 3 + 1]);
                const auto C = ToCentered(TriangleVertices[Index * 3 + 2]);

                SignedVolumeSum += FVector::DotProduct(A, FVector::CrossProduct(B, C));
            }
        }

        return (SignedVolumeSum / 6.0) / BoundsVolume;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        HeightFieldNoCollisionValue()
        -> float
    {
        return JPH::HeightFieldShapeConstants::cNoCollisionValue;
    }

    auto
        CreateHeightFieldShape(
            const TArray<float>& InWorldHeights,
            int32 InSampleCount,
            const FVector2D& InScaleXY)
        -> JPH::Ref<JPH::Shape>
    {
        return CreateHeightFieldShape_Updatable(InWorldHeights, InSampleCount, InScaleXY, {})._Shape;
    }

    auto
        CreateHeightFieldShape_Updatable(
            const TArray<float>& InWorldHeights,
            int32 InSampleCount,
            const FVector2D& InScaleXY,
            const TOptional<FFloatInterval>& InDeformationEnvelope)
        -> FCk_Jolt_UpdatableHeightField
    {
        CK_ENSURE_IF_NOT(InSampleCount >= 2, TEXT("Heightfield needs at least 2x2 samples, got [{}]"), InSampleCount)
        { return {}; }

        CK_ENSURE_IF_NOT(InWorldHeights.Num() == InSampleCount * InSampleCount,
            TEXT("Heightfield sample count mismatch: [{}] samples for [{}]x[{}]"),
            InWorldHeights.Num(), InSampleCount, InSampleCount)
        { return {}; }

        // Jolt heightfields are local Y-up graphs over X/Z: local = (x*sx, height, z*sz). Rotating
        // +90deg about X maps local +Y to world +Z and local +Z to world -Y; rows FLIPPED (r = N-1-y)
        // plus a -(N-1)*sy local-Z offset land sample (x, y) exactly on UE's (x*sx, y*sy, height).
        const auto SampleCount = InSampleCount;
        auto Samples = TArray<float>{};
        Samples.SetNumUninitialized(SampleCount * SampleCount);

        for (auto Row = 0; Row < SampleCount; ++Row)
        {
            const auto SourceRow = (SampleCount - 1) - Row;
            FMemory::Memcpy(
                Samples.GetData() + Row * SampleCount,
                InWorldHeights.GetData() + SourceRow * SampleCount,
                SampleCount * sizeof(float));
        }

        const auto Offset = JPH::Vec3{0.0f, 0.0f, -static_cast<float>((SampleCount - 1) * InScaleXY.Y)};
        const auto Scale = JPH::Vec3{static_cast<float>(InScaleXY.X), 1.0f, static_cast<float>(InScaleXY.Y)};

        auto HeightFieldSettings = JPH::HeightFieldShapeSettings{
            Samples.GetData(), Offset, Scale, static_cast<JPH::uint32>(SampleCount)};

        if (InDeformationEnvelope.IsSet())
        {
            const auto& Envelope = *InDeformationEnvelope;

            CK_ENSURE_IF_NOT(Envelope.Min < Envelope.Max,
                TEXT("Heightfield deformation envelope is INVERTED or empty: [{}, {}]. Declare Min < Max, "
                     "or leave it unset to encode exactly the initial samples' range."),
                Envelope.Min, Envelope.Max)
            { return {}; }

            // Heights are stored with scale.Y = 1 and no Y offset, so envelope values ARE world
            // heights. Jolt widens this by the initial samples, never narrows below them.
            HeightFieldSettings.mMinHeightValue = Envelope.Min;
            HeightFieldSettings.mMaxHeightValue = Envelope.Max;
        }

        auto HeightFieldShape = Create_ShapeFromSettings(HeightFieldSettings, TEXT("HeightField"));
        if (ck::Is_NOT_Valid(HeightFieldShape))
        { return {}; }

        const auto UprightSettings = JPH::RotatedTranslatedShapeSettings{
            JPH::Vec3::sZero(), Get_ShapeAxisCorrection_YToZ(), HeightFieldShape.GetPtr()};

        auto UprightShape = Create_ShapeFromSettings(UprightSettings, TEXT("HeightField"));
        if (ck::Is_NOT_Valid(UprightShape))
        { return {}; }

        auto Result = FCk_Jolt_UpdatableHeightField{};
        Result._Shape = UprightShape;
        // Known-exact type: these settings produce a HeightFieldShape and nothing else.
        Result._HeightField = static_cast<JPH::HeightFieldShape*>(HeightFieldShape.GetPtr());

        return Result;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        ComputeHeightFieldRegionPlan(
            int32 InLogicalSampleCount,
            int32 InShapeSampleCount,
            int32 InBlockSize,
            int32 InUeX,
            int32 InUeY,
            int32 InUeSizeX,
            int32 InUeSizeY)
        -> TOptional<FCk_Jolt_HeightFieldRegionPlan>
    {
        if (InBlockSize <= 0 || InLogicalSampleCount <= 0 || InShapeSampleCount < InLogicalSampleCount)
        { return {}; }

        if (InUeSizeX <= 0 || InUeSizeY <= 0)
        { return {}; }

        if (InUeX < 0 || InUeY < 0 ||
            InUeX + InUeSizeX > InLogicalSampleCount ||
            InUeY + InUeSizeY > InLogicalSampleCount)
        { return {}; }

        const auto RoundDown = [InBlockSize](int32 InValue) -> int32
        {
            return (InValue / InBlockSize) * InBlockSize;
        };

        const auto RoundUp = [InBlockSize](int32 InValue) -> int32
        {
            return ((InValue + InBlockSize - 1) / InBlockSize) * InBlockSize;
        };

        // Same row flip as creation: UE row y lives at Jolt row N-1-y, so the UE row RANGE
        // [UeY, UeY+SizeY) reverses into [N-UeY-SizeY, N-UeY).
        const auto JoltRowBegin = InLogicalSampleCount - InUeY - InUeSizeY;
        const auto JoltRowEnd = InLogicalSampleCount - InUeY;

        const auto AlignedX = RoundDown(InUeX);
        const auto AlignedRight = RoundUp(InUeX + InUeSizeX);
        const auto AlignedY = RoundDown(JoltRowBegin);
        const auto AlignedTop = RoundUp(JoltRowEnd);

        // The shape's sample count is the logical count rounded UP to a block multiple, so an
        // outward-aligned rect over in-bounds input always fits. A violation would mean the two
        // roundings disagree — report rather than clamp, which would silently edit the wrong cells.
        if (AlignedRight > InShapeSampleCount || AlignedTop > InShapeSampleCount)
        { return {}; }

        auto Plan = FCk_Jolt_HeightFieldRegionPlan{};
        Plan._JoltX = AlignedX;
        Plan._JoltY = AlignedY;
        Plan._SizeX = AlignedRight - AlignedX;
        Plan._SizeY = AlignedTop - AlignedY;

        return Plan;
    }

    auto
        ApplyHeightFieldRegionUpdate(
            JPH::HeightFieldShape& InOutHeightField,
            int32 InLogicalSampleCount,
            int32 InUeX,
            int32 InUeY,
            int32 InUeSizeX,
            int32 InUeSizeY,
            const TArray<float>& InWorldHeights,
            JPH::TempAllocator& InTempAllocator)
        -> ECk_Jolt_HeightFieldRegionUpdateResult
    {
        if (InWorldHeights.Num() != InUeSizeX * InUeSizeY)
        { return ECk_Jolt_HeightFieldRegionUpdateResult::OutOfBounds; }

        const auto ShapeSampleCount = static_cast<int32>(InOutHeightField.GetSampleCount());
        const auto BlockSize = static_cast<int32>(InOutHeightField.GetBlockSize());

        const auto MaybePlan = ComputeHeightFieldRegionPlan(InLogicalSampleCount, ShapeSampleCount,
            BlockSize, InUeX, InUeY, InUeSizeX, InUeSizeY);

        if (NOT MaybePlan.IsSet())
        { return ECk_Jolt_HeightFieldRegionUpdateResult::OutOfBounds; }

        const auto& Plan = *MaybePlan;

        // Validate BEFORE reading or writing anything: Jolt CLAMPS out-of-range heights silently,
        // and a partially applied crater is worse than a rejected one.
        const auto NoCollision = HeightFieldNoCollisionValue();
        const auto MinEncodable = InOutHeightField.GetMinHeightValue();
        const auto MaxEncodable = InOutHeightField.GetMaxHeightValue();

        for (const auto& Height : InWorldHeights)
        {
            if (Height == NoCollision)
            { continue; }

            if (Height < MinEncodable || Height > MaxEncodable)
            { return ECk_Jolt_HeightFieldRegionUpdateResult::OutOfEnvelope; }
        }

        auto AlignedHeights = TArray<float>{};
        AlignedHeights.SetNumUninitialized(Plan._SizeX * Plan._SizeY);

        // Read the aligned rect first so the expansion border keeps its current heights AND its
        // holes; only the caller's own cells are then overwritten.
        InOutHeightField.GetHeights(Plan._JoltX, Plan._JoltY, Plan._SizeX, Plan._SizeY,
            AlignedHeights.GetData(), Plan._SizeX);

        for (auto LocalRow = 0; LocalRow < InUeSizeY; ++LocalRow)
        {
            const auto JoltRow = (InLogicalSampleCount - 1) - (InUeY + LocalRow);
            const auto BufferRow = JoltRow - Plan._JoltY;

            for (auto LocalColumn = 0; LocalColumn < InUeSizeX; ++LocalColumn)
            {
                const auto BufferColumn = (InUeX + LocalColumn) - Plan._JoltX;
                AlignedHeights[BufferRow * Plan._SizeX + BufferColumn] =
                    InWorldHeights[LocalRow * InUeSizeX + LocalColumn];
            }
        }

        InOutHeightField.SetHeights(Plan._JoltX, Plan._JoltY, Plan._SizeX, Plan._SizeY,
            AlignedHeights.GetData(), Plan._SizeX, InTempAllocator);

        return ECk_Jolt_HeightFieldRegionUpdateResult::Applied;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        ExtractComponent(
            const UPrimitiveComponent& InComponent,
            FCk_Jolt_ShapeCache& InShapeCache,
            TArray<FCk_Jolt_ExtractedBody>& OutBodies,
            const FCk_Jolt_BakeFilter& InFilter,
            ECk_Jolt_ExtractionPolicy InPolicy,
            FCk_Jolt_ExtractionStats* OutStats)
        -> int32
    {
        if (OutStats != nullptr)
        { ++OutStats->_NumComponentsConsidered; }

        if (const auto SkipReason = Get_ComponentSkipReason(InComponent, InPolicy, InFilter);
            SkipReason != ECk_Jolt_ComponentSkipReason::NotSkipped)
        {
            if (OutStats != nullptr && SkipReason == ECk_Jolt_ComponentSkipReason::ExcludedByMobility)
            { ++OutStats->_NumComponentsExcludedByMobility; }

            if (OutStats != nullptr && SkipReason == ECk_Jolt_ComponentSkipReason::ExcludedByFilter)
            { ++OutStats->_NumComponentsExcludedByFilter; }

            return 0;
        }

        const auto StartingCount = OutBodies.Num();
        const auto& ComponentTransform = InComponent.GetComponentTransform();
        const auto Signature = FCk_Jolt_CollisionSignature::Make_FromComponent(InComponent, ECk_Jolt_BodyDomain::Static);

        const auto EmitBody = [&](const JPH::Ref<JPH::Shape>& InShape, const FVector& InPosition,
            const FQuat& InRotation, const UBodySetup* InBodySetup) -> void
        {
            if (ck::Is_NOT_Valid(InShape))
            { return; }

            auto Body = FCk_Jolt_ExtractedBody{};
            Body._Shape = InShape;
            Body._Position = InPosition;
            Body._Rotation = InRotation;
            Body._Signature = Signature;
            Body._SourceComponent = &InComponent;
            Get_SurfaceProperties(InComponent, InBodySetup, Body);
            OutBodies.Emplace(MoveTemp(Body));
        };

        // Dispatch most-derived-first: SplineMesh and ISM both derive UStaticMeshComponent.
        if (const auto* SplineMesh = Cast<USplineMeshComponent>(&InComponent))
        {
            // The BodySetup is per-instance (deformed geometry), so this bypasses the shared cache.
            const auto* BodySetup = SplineMesh->GetBodySetup();
            const auto DebugName = ck::Format_UE(TEXT("{} on {}"),
                GetNameSafe(SplineMesh->GetStaticMesh()), InComponent.GetPathName());

            CK_ENSURE_IF_NOT(ck::IsValid(BodySetup),
                TEXT("SplineMeshComponent [{}] has collision enabled but no BodySetup"), DebugName)
            { return 0; }

            const auto Shape = BuildShape_FromBodySetup(*BodySetup, ComponentTransform.GetScale3D(),
                DebugName);
            EmitBody(Shape, ComponentTransform.GetLocation(), ComponentTransform.GetRotation(), BodySetup);
        }
        else if (const auto* Instanced = Cast<UInstancedStaticMeshComponent>(&InComponent))
        {
            const auto* Mesh = Instanced->GetStaticMesh().Get();
            if (ck::Is_NOT_Valid(Mesh))
            { return 0; }

            const auto* BodySetup = Mesh->GetBodySetup();
            const auto DebugName = ck::Format_UE(TEXT("{} on {}"), Mesh->GetName(), InComponent.GetPathName());

            CK_ENSURE_IF_NOT(ck::IsValid(BodySetup),
                TEXT("Instanced mesh [{}] has collision enabled but no BodySetup"), DebugName)
            { return 0; }

            const auto InstanceCount = Instanced->GetInstanceCount();
            constexpr auto WorldSpace = true;

            auto InstanceTransforms = TArray<FTransform>{};
            InstanceTransforms.Reserve(InstanceCount);
            for (auto Index = 0; Index < InstanceCount; ++Index)
            {
                auto InstanceTransform = FTransform{};
                if (Instanced->GetInstanceTransform(Index, InstanceTransform, WorldSpace))
                { InstanceTransforms.Emplace(InstanceTransform); }
            }

            const auto CompoundThreshold = UCk_Utils_Jolt_ProjectSettings::Get_CompoundShapeInstanceThreshold();

            if (InstanceTransforms.Num() >= CompoundThreshold)
            {
                // ONE StaticCompoundShape per dense cluster — per-instance bodies would flood the broadphase.
                const auto BodyPosition = ComponentTransform.GetLocation();
                const auto BodyRotation = ComponentTransform.GetRotation();
                const auto InvBodyRotation = BodyRotation.Inverse();

                auto CompoundSettings = JPH::StaticCompoundShapeSettings{};
                auto ChildCount = 0;

                for (const auto& InstanceTransform : InstanceTransforms)
                {
                    const auto Shape = InShapeCache.GetOrCreate_Shape(*BodySetup,
                        InstanceTransform.GetScale3D(), DebugName);
                    if (ck::Is_NOT_Valid(Shape))
                    { continue; }

                    const auto LocalPosition = InvBodyRotation.RotateVector(InstanceTransform.GetLocation() - BodyPosition);
                    const auto LocalRotation = InvBodyRotation * InstanceTransform.GetRotation();
                    CompoundSettings.AddShape(Conv(LocalPosition), Conv(LocalRotation), Shape.GetPtr());
                    ++ChildCount;
                }

                if (ChildCount >= 2)
                {
                    const auto CompoundShape = Create_ShapeFromSettings(CompoundSettings, DebugName);
                    EmitBody(CompoundShape, BodyPosition, BodyRotation, BodySetup);
                }
                else if (ChildCount == 1)
                {
                    // Degenerate compound: all but one instance failed shape creation.
                    const auto& InstanceTransform = InstanceTransforms[0];
                    const auto Shape = InShapeCache.GetOrCreate_Shape(*BodySetup,
                        InstanceTransform.GetScale3D(), DebugName);
                    EmitBody(Shape, InstanceTransform.GetLocation(), InstanceTransform.GetRotation(), BodySetup);
                }
            }
            else
            {
                for (const auto& InstanceTransform : InstanceTransforms)
                {
                    const auto Shape = InShapeCache.GetOrCreate_Shape(*BodySetup,
                        InstanceTransform.GetScale3D(), DebugName);
                    EmitBody(Shape, InstanceTransform.GetLocation(), InstanceTransform.GetRotation(), BodySetup);
                }
            }
        }
        else if (const auto* StaticMesh = Cast<UStaticMeshComponent>(&InComponent))
        {
            const auto* Mesh = StaticMesh->GetStaticMesh().Get();
            if (ck::Is_NOT_Valid(Mesh))
            { return 0; }

            const auto* BodySetup = Mesh->GetBodySetup();
            const auto DebugName = ck::Format_UE(TEXT("{} on {}"), Mesh->GetName(), InComponent.GetPathName());

            CK_ENSURE_IF_NOT(ck::IsValid(BodySetup),
                TEXT("StaticMesh [{}] has collision enabled but no BodySetup"), DebugName)
            { return 0; }

            const auto Shape = InShapeCache.GetOrCreate_Shape(*BodySetup,
                ComponentTransform.GetScale3D(), DebugName);
            EmitBody(Shape, ComponentTransform.GetLocation(), ComponentTransform.GetRotation(), BodySetup);
        }
        else if (const auto* Brush = Cast<UBrushComponent>(&InComponent))
        {
            const auto* BodySetup = Brush->BrushBodySetup.Get();

            if (ck::Is_NOT_Valid(BodySetup))
            {
                ck::jolt::Verbose(TEXT("Static bake skipping BrushComponent [{}] on [{}] — no BrushBodySetup (Chaos creates no physics state for it either)"),
                    InComponent.GetName(), GetNameSafe(InComponent.GetOwner()));
                return 0;
            }

            // Brush geometry is already convex-decomposed into the BodySetup's ConvexElems.
            const auto DebugName = ck::Format_UE(TEXT("Brush on {}"), InComponent.GetPathName());
            const auto Shape = BuildShape_FromBodySetup(*BodySetup, ComponentTransform.GetScale3D(),
                DebugName);
            EmitBody(Shape, ComponentTransform.GetLocation(), ComponentTransform.GetRotation(), BodySetup);
        }
        else if (const auto* DynamicMesh = Cast<UDynamicMeshComponent>(&InComponent))
        {
            // The CONST overload returns MeshBodySetup as-is — a never-cooked component reads null
            // here instead of conjuring an empty setup that would bake as silently-absent collision.
            const auto* BodySetup = DynamicMesh->GetBodySetup();
            const auto DebugName = ck::Format_UE(TEXT("DynamicMesh on {}"), InComponent.GetPathName());

            CK_ENSURE_IF_NOT(ck::IsValid(BodySetup),
                TEXT("DynamicMeshComponent [{}] has collision enabled but no BodySetup — its collision was "
                     "never cooked. Call UpdateCollision after authoring the mesh; with bUseAsyncCooking the "
                     "cook may still be in flight, which this bake cannot wait for."), DebugName)
            { return 0; }

            // Runtime-recooked geometry: UpdateCollision assigns a FRESH BodySetupGuid on every sync
            // recook, so the guid-keyed shared cache would leak one entry per edit. Build directly —
            // the same reason the SplineMesh branch above bypasses the cache.
            const auto Shape = BuildShape_FromBodySetup(*BodySetup, ComponentTransform.GetScale3D(),
                DebugName);
            EmitBody(Shape, ComponentTransform.GetLocation(), ComponentTransform.GetRotation(), BodySetup);
        }
        else
        {
            // Loudness belongs to the CALLER, not here. Request_BakeComponent always extracts under
            // ExplicitActor, so ensuring on this branch also fires for CkUnrealComponent's Automatic
            // policy — whose contract is an explicit QUIET skip on zero bodies. The callers that did
            // declare complete collision (BakeOnSetup) already ensure on a zero-body result, so the
            // unsupported-class case stays diagnosable without spamming every map that hosts a shape
            // component on a baked entity.
            ck::jolt::Verbose(TEXT("Static bake skipping [{}] ([{}]) — no extraction path for this component class"),
                InComponent.GetName(), InComponent.GetClass()->GetName());
        }

        const auto NumExtracted = OutBodies.Num() - StartingCount;

        if (OutStats != nullptr)
        { OutStats->_NumBodiesExtracted += NumExtracted; }

        return NumExtracted;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        ExtractActor(
            const AActor& InActor,
            FCk_Jolt_ShapeCache& InShapeCache,
            TArray<FCk_Jolt_ExtractedBody>& OutBodies,
            const FCk_Jolt_BakeFilter& InFilter,
            ECk_Jolt_ExtractionPolicy InPolicy,
            FCk_Jolt_ExtractionStats* OutStats)
        -> int32
    {
        const auto StartingCount = OutBodies.Num();

        if (InPolicy == ECk_Jolt_ExtractionPolicy::LevelSweep &&
            Get_IsActorExcludedByFilter(InActor, InFilter))
        {
            if (OutStats != nullptr)
            { ++OutStats->_NumActorsExcludedByFilter; }

            ck::jolt::VeryVerbose(TEXT("Static bake skipping actor [{}] — excluded by bake-filter settings "
                "(class or actor tag)"), InActor.GetFName());
            return 0;
        }

#if WITH_EDITOR
        // Landscape heights are readable only in editor context — cooked builds get them from cooked data.
        if (const auto* LandscapeProxy = Cast<ALandscapeProxy>(&InActor))
        {
            for (const auto& LandscapeComponent : LandscapeProxy->LandscapeComponents)
            {
                if (ck::Is_NOT_Valid(LandscapeComponent))
                { continue; }

                const auto* CollisionComponent = LandscapeComponent->GetCollisionComponent();
                CK_ENSURE_IF_NOT(ck::IsValid(CollisionComponent),
                    TEXT("Landscape component [{}] has no heightfield collision component — skipping (Chaos would have no physics state for it either)"),
                    LandscapeComponent->GetName())
                { continue; }

                // The collision component carries the landscape's real collision setup — apply the
                // FILTER axes on it (mirroring the signature capture below), but not the eligibility
                // gates: the landscape path never had them, and a WP-loaded-but-unregistered
                // collision component must not silently drop the landscape from a cook.
                if (InPolicy == ECk_Jolt_ExtractionPolicy::LevelSweep &&
                    Get_IsComponentExcludedByFilter(*CollisionComponent, InFilter))
                {
                    if (OutStats != nullptr)
                    { ++OutStats->_NumComponentsExcludedByFilter; }
                    continue;
                }

                auto DataInterface = FLandscapeComponentDataInterface{LandscapeComponent};

                const auto SampleCount = LandscapeComponent->ComponentSizeQuads + 1;
                const auto ComponentTransform = LandscapeComponent->GetComponentTransform();
                const auto Scale = ComponentTransform.GetScale3D();

                auto Heights = TArray<float>{};
                Heights.SetNumUninitialized(SampleCount * SampleCount);

                for (auto Y = 0; Y < SampleCount; ++Y)
                {
                    for (auto X = 0; X < SampleCount; ++X)
                    {
                        Heights[Y * SampleCount + X] =
                            LandscapeDataAccess::GetLocalHeight(DataInterface.GetHeight(X, Y)) * Scale.Z;
                    }
                }

                const auto Shape = CreateHeightFieldShape(Heights, SampleCount, FVector2D{Scale.X, Scale.Y});
                if (ck::Is_NOT_Valid(Shape))
                { continue; }

                auto Body = FCk_Jolt_ExtractedBody{};
                Body._Shape = Shape;
                Body._Position = ComponentTransform.GetLocation();
                Body._Rotation = ComponentTransform.GetRotation();
                Body._Signature = FCk_Jolt_CollisionSignature::Make_FromComponent(*CollisionComponent,
                    ECk_Jolt_BodyDomain::Static);
                Body._SourceComponent = LandscapeComponent;

                if (const auto* DefaultPhysMaterial = LandscapeProxy->DefaultPhysMaterial.Get())
                {
                    Body._Friction = DefaultPhysMaterial->Friction;
                    Body._Restitution = DefaultPhysMaterial->Restitution;
                    Body._SurfaceType = DefaultPhysMaterial->SurfaceType;
                }

                OutBodies.Emplace(MoveTemp(Body));
            }
        }

        // Landscape bodies bypass ExtractComponent, so their count is added here.
        if (OutStats != nullptr)
        { OutStats->_NumBodiesExtracted += OutBodies.Num() - StartingCount; }
#endif

        InActor.ForEachComponent<UPrimitiveComponent>(false,
            [&](const UPrimitiveComponent* InComponent)
            {
                if (ck::Is_NOT_Valid(InComponent))
                { return; }

#if WITH_EDITOR
                // Landscape render components were handled above via the heightfield path.
                if (InComponent->IsA<ULandscapeComponent>() ||
                    InComponent->IsA<ULandscapeHeightfieldCollisionComponent>())
                { return; }
#endif

                ExtractComponent(*InComponent, InShapeCache, OutBodies, InFilter, InPolicy, OutStats);
            });

        return OutBodies.Num() - StartingCount;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Hashing
    // ----------------------------------------------------------------------------------------------------------------

    static auto DoHash_Transform(FXxHash64Builder& InOutBuilder, const FTransform& InTransform) -> void
    {
        const auto Quantize = [](double InValue) -> int64
        {
            return static_cast<int64>(FMath::RoundToDouble(InValue * 10.0));
        };

        const auto Location = InTransform.GetLocation();
        const auto Rotation = InTransform.GetRotation();
        const auto Scale = InTransform.GetScale3D();

        const int64 Values[] = {
            Quantize(Location.X), Quantize(Location.Y), Quantize(Location.Z),
            Quantize(Rotation.X * 1000.0), Quantize(Rotation.Y * 1000.0),
            Quantize(Rotation.Z * 1000.0), Quantize(Rotation.W * 1000.0),
            Quantize(Scale.X * 100.0), Quantize(Scale.Y * 100.0), Quantize(Scale.Z * 100.0)};

        InOutBuilder.Update(Values, sizeof(Values));
    }

    static auto DoHash_String(FXxHash64Builder& InOutBuilder, const FString& InString) -> void
    {
        InOutBuilder.Update(*InString, InString.Len() * sizeof(TCHAR));
    }

    static auto DoGather_RelevantComponents(
        const AActor& InActor,
        const FCk_Jolt_BakeFilter& InFilter) -> TArray<const UPrimitiveComponent*>
    {
        auto Components = TArray<const UPrimitiveComponent*>{};

        InActor.ForEachComponent<UPrimitiveComponent>(false,
            [&](const UPrimitiveComponent* InComponent)
            {
                if (ck::Is_NOT_Valid(InComponent))
                { return; }

                // Hashes cover the cooked/level-sweep population — same skip policy, same filter.
                if (Get_ShouldSkipComponent(*InComponent, ECk_Jolt_ExtractionPolicy::LevelSweep, InFilter))
                { return; }

                Components.Emplace(InComponent);
            });

        // Deterministic order regardless of registration order.
        Components.Sort([](const UPrimitiveComponent& InA, const UPrimitiveComponent& InB)
        {
            return InA.GetFName().LexicalLess(InB.GetFName());
        });

        return Components;
    }

    auto
        ComputeRuntimeCheckHash(
            const AActor& InActor,
            const FCk_Jolt_BakeFilter& InFilter)
        -> uint64
    {
        auto Builder = FXxHash64Builder{};

        for (const auto* Component : DoGather_RelevantComponents(InActor, InFilter))
        {
            DoHash_String(Builder, Component->GetClass()->GetName());
            DoHash_Transform(Builder, Component->GetComponentTransform());

            if (const auto* StaticMesh = Cast<UStaticMeshComponent>(Component))
            {
                if (const auto* Mesh = StaticMesh->GetStaticMesh().Get())
                { DoHash_String(Builder, Mesh->GetPathName()); }
            }

            if (const auto* Instanced = Cast<UInstancedStaticMeshComponent>(Component))
            {
                const int32 InstanceCount = Instanced->GetInstanceCount();
                Builder.Update(&InstanceCount, sizeof(InstanceCount));
            }
        }

        return Builder.Finalize().Hash;
    }

#if WITH_EDITOR
    auto
        ComputeSourceHash(
            const AActor& InActor,
            const FCk_Jolt_BakeFilter& InFilter)
        -> uint64
    {
        auto Builder = FXxHash64Builder{};

        for (const auto* Component : DoGather_RelevantComponents(InActor, InFilter))
        {
            DoHash_String(Builder, Component->GetClass()->GetName());
            DoHash_Transform(Builder, Component->GetComponentTransform());

            const auto HashBodySetup = [&](const UBodySetup* InBodySetup) -> void
            {
                if (ck::Is_NOT_Valid(InBodySetup))
                { return; }

                Builder.Update(&InBodySetup->BodySetupGuid, sizeof(FGuid));
                const uint8 TraceFlag = static_cast<uint8>(InBodySetup->GetCollisionTraceFlag());
                Builder.Update(&TraceFlag, sizeof(TraceFlag));
            };

            if (const auto* StaticMesh = Cast<UStaticMeshComponent>(Component))
            {
                if (const auto* Mesh = StaticMesh->GetStaticMesh().Get())
                {
                    DoHash_String(Builder, Mesh->GetPathName());
                    HashBodySetup(Mesh->GetBodySetup());
                }
            }

            if (const auto* Brush = Cast<UBrushComponent>(Component))
            { HashBodySetup(Brush->BrushBodySetup.Get()); }

            if (const auto* Instanced = Cast<UInstancedStaticMeshComponent>(Component))
            {
                constexpr auto WorldSpace = true;
                for (auto Index = 0; Index < Instanced->GetInstanceCount(); ++Index)
                {
                    auto InstanceTransform = FTransform{};
                    if (Instanced->GetInstanceTransform(Index, InstanceTransform, WorldSpace))
                    { DoHash_Transform(Builder, InstanceTransform); }
                }
            }
        }

        return Builder.Finalize().Hash;
    }
#endif
}

// --------------------------------------------------------------------------------------------------------------------
