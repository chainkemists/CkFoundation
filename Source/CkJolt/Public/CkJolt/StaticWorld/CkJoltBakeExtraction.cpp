#include "CkJoltBakeExtraction.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Ensure/CkEnsure.h"

#include "CkJolt/CkJolt_Log.h"
#include "CkJolt/CkJolt_Utils.h"
#include "CkJolt/Settings/CkJolt_ProjectSettings.h"

#include <Components/BrushComponent.h>
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

        if (ck::IsValid(InComponent.BodyInstance.GetPhysMaterialOverride(), ck::IsValid_Policy_NullptrOnly{}))
        { PhysMaterial = InComponent.BodyInstance.GetPhysMaterialOverride(); }
        else if (ck::IsValid(InBodySetup, ck::IsValid_Policy_NullptrOnly{}))
        { PhysMaterial = InBodySetup->GetPhysMaterial(); }

        if (ck::Is_NOT_Valid(PhysMaterial) && ck::IsValid(GEngine, ck::IsValid_Policy_NullptrOnly{}))
        { PhysMaterial = GEngine->DefaultPhysMaterial; }

        if (ck::Is_NOT_Valid(PhysMaterial))
        { return; }

        InOutBody._Friction = PhysMaterial->Friction;
        InOutBody._Restitution = PhysMaterial->Restitution;
        InOutBody._SurfaceType = PhysMaterial->SurfaceType;
    }

    // ----------------------------------------------------------------------------------------------------------------

    static auto Get_ShouldSkipComponent(
        const UPrimitiveComponent& InComponent,
        ECk_Jolt_ExtractionPolicy InPolicy) -> bool
    {
        if (NOT InComponent.IsRegistered())
        { return true; }

        if (InComponent.IsEditorOnly())
        { return true; }

#if WITH_EDITORONLY_DATA
        if (InComponent.IsVisualizationComponent())
        { return true; }
#endif

        if (InComponent.GetCollisionEnabled() == ECollisionEnabled::NoCollision)
        { return true; }

        if (InComponent.IsSimulatingPhysics())
        { return true; }

        if (InPolicy == ECk_Jolt_ExtractionPolicy::LevelSweep &&
            InComponent.Mobility == EComponentMobility::Movable)
        {
            ck::jolt::VeryVerbose(TEXT("Static bake skipping MOVABLE component [{}] — kinematic/dynamic territory"),
                InComponent.GetName());
            return true;
        }

        return false;
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
            {}
            if (NOT HalfExtentsArePositive)
            { return false; }

            constexpr auto MaximumConvexRadiusFraction = 0.5f;
            const auto MinimumHalfExtent = JoltHalfExtents.ReduceMin();
            const auto ConvexRadius = FMath::Min(
                JPH::cDefaultConvexRadius, MinimumHalfExtent * MaximumConvexRadiusFraction);
            const auto Settings = JPH::BoxShapeSettings{JoltHalfExtents, ConvexRadius};
            auto Shape = Create_ShapeFromSettings(Settings, InDebugName);
            if (Shape == nullptr)
            { return false; }

            OutLeaves.Emplace(FLeafShape{Shape, BoxElem.Center * InScale, BoxElem.Rotation.Quaternion()});
        }

        for (const auto& SphereElem : AggGeom.SphereElems)
        {
            const auto Radius = SphereElem.Radius * AbsScale.GetMin();

            const auto Settings = JPH::SphereShapeSettings{static_cast<float>(Radius)};
            auto Shape = Create_ShapeFromSettings(Settings, InDebugName);
            if (Shape == nullptr)
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
            if (CapsuleShape == nullptr)
            { continue; }

            const auto UprightSettings = JPH::RotatedTranslatedShapeSettings{
                JPH::Vec3::sZero(), jolt::Get_ShapeAxisCorrection_YToZ(), CapsuleShape.GetPtr()};
            auto UprightShape = Create_ShapeFromSettings(UprightSettings, InDebugName);
            if (UprightShape == nullptr)
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
            if (Shape == nullptr)
            { continue; }

            OutLeaves.Emplace(FLeafShape{Shape, FVector::ZeroVector, FQuat::Identity});
        }

        return true;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Chaos cooked tri-mesh -> Jolt MeshShape
    // ----------------------------------------------------------------------------------------------------------------

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

        // Chaos and Jolt disagree on triangle winding — swap b/c so faces point outward.
        const auto PushTriangle = [&](uint32 InA, uint32 InB, uint32 InC) -> void
        {
            if (InA < static_cast<uint32>(NumVerts) && InB < static_cast<uint32>(NumVerts) && InC < static_cast<uint32>(NumVerts))
            { Triangles.push_back(JPH::IndexedTriangle(InA, InC, InB)); }
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

        auto Shape = BuildShape_FromBodySetup(InBodySetup, InScale, InDebugName);

        if (Shape != nullptr)
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

        const auto HeightFieldSettings = JPH::HeightFieldShapeSettings{
            Samples.GetData(), Offset, Scale, static_cast<JPH::uint32>(SampleCount)};

        auto HeightFieldShape = Create_ShapeFromSettings(HeightFieldSettings, TEXT("HeightField"));
        if (HeightFieldShape == nullptr)
        { return {}; }

        const auto UprightSettings = JPH::RotatedTranslatedShapeSettings{
            JPH::Vec3::sZero(), Get_ShapeAxisCorrection_YToZ(), HeightFieldShape.GetPtr()};
        return Create_ShapeFromSettings(UprightSettings, TEXT("HeightField"));
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        ExtractComponent(
            const UPrimitiveComponent& InComponent,
            FCk_Jolt_ShapeCache& InShapeCache,
            TArray<FCk_Jolt_ExtractedBody>& OutBodies,
            ECk_Jolt_ExtractionPolicy InPolicy)
        -> int32
    {
        if (Get_ShouldSkipComponent(InComponent, InPolicy))
        { return 0; }

        const auto StartingCount = OutBodies.Num();
        const auto& ComponentTransform = InComponent.GetComponentTransform();
        const auto Signature = FCk_Jolt_CollisionSignature::Make_FromComponent(InComponent, ECk_Jolt_BodyDomain::Static);

        const auto EmitBody = [&](const JPH::Ref<JPH::Shape>& InShape, const FVector& InPosition,
            const FQuat& InRotation, const UBodySetup* InBodySetup) -> void
        {
            if (InShape == nullptr)
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

            CK_ENSURE_IF_NOT(ck::IsValid(BodySetup, ck::IsValid_Policy_NullptrOnly{}),
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

            CK_ENSURE_IF_NOT(ck::IsValid(BodySetup, ck::IsValid_Policy_NullptrOnly{}),
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
                    if (Shape == nullptr)
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

            CK_ENSURE_IF_NOT(ck::IsValid(BodySetup, ck::IsValid_Policy_NullptrOnly{}),
                TEXT("StaticMesh [{}] has collision enabled but no BodySetup"), DebugName)
            { return 0; }

            const auto Shape = InShapeCache.GetOrCreate_Shape(*BodySetup,
                ComponentTransform.GetScale3D(), DebugName);
            EmitBody(Shape, ComponentTransform.GetLocation(), ComponentTransform.GetRotation(), BodySetup);
        }
        else if (const auto* Brush = Cast<UBrushComponent>(&InComponent))
        {
            const auto* BodySetup = Brush->BrushBodySetup.Get();

            if (NOT ck::IsValid(BodySetup, ck::IsValid_Policy_NullptrOnly{}))
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

        return OutBodies.Num() - StartingCount;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        ExtractActor(
            const AActor& InActor,
            FCk_Jolt_ShapeCache& InShapeCache,
            TArray<FCk_Jolt_ExtractedBody>& OutBodies,
            ECk_Jolt_ExtractionPolicy InPolicy)
        -> int32
    {
        const auto StartingCount = OutBodies.Num();

#if WITH_EDITOR
        // Landscape heights are readable only in editor context — cooked builds get them from cooked data.
        if (const auto* LandscapeProxy = Cast<ALandscapeProxy>(&InActor))
        {
            for (const auto& LandscapeComponent : LandscapeProxy->LandscapeComponents)
            {
                if (ck::Is_NOT_Valid(LandscapeComponent))
                { continue; }

                const auto* CollisionComponent = LandscapeComponent->GetCollisionComponent();
                CK_ENSURE_IF_NOT(ck::IsValid(CollisionComponent, ck::IsValid_Policy_NullptrOnly{}),
                    TEXT("Landscape component [{}] has no heightfield collision component — skipping (Chaos would have no physics state for it either)"),
                    LandscapeComponent->GetName())
                { continue; }

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
                if (Shape == nullptr)
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

                ExtractComponent(*InComponent, InShapeCache, OutBodies, InPolicy);
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

    static auto DoGather_RelevantComponents(const AActor& InActor) -> TArray<const UPrimitiveComponent*>
    {
        auto Components = TArray<const UPrimitiveComponent*>{};

        InActor.ForEachComponent<UPrimitiveComponent>(false,
            [&](const UPrimitiveComponent* InComponent)
            {
                if (ck::Is_NOT_Valid(InComponent))
                { return; }

                // Hashes cover the cooked/level-sweep population — same skip policy.
                if (Get_ShouldSkipComponent(*InComponent, ECk_Jolt_ExtractionPolicy::LevelSweep))
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
            const AActor& InActor)
        -> uint64
    {
        auto Builder = FXxHash64Builder{};

        for (const auto* Component : DoGather_RelevantComponents(InActor))
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
            const AActor& InActor)
        -> uint64
    {
        auto Builder = FXxHash64Builder{};

        for (const auto* Component : DoGather_RelevantComponents(InActor))
        {
            DoHash_String(Builder, Component->GetClass()->GetName());
            DoHash_Transform(Builder, Component->GetComponentTransform());

            const auto HashBodySetup = [&](const UBodySetup* InBodySetup) -> void
            {
                if (ck::Is_NOT_Valid(InBodySetup, ck::IsValid_Policy_NullptrOnly{}))
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
