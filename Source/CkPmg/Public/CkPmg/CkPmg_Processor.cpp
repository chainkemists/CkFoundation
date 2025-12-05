#include "CkPmg_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Debug/CkDebugDraw_Utils.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

#include "CkPmg/CkPmg_Log.h"
#include "CkPmg_Utils.h"
#include "CkPmg_Utils_IconShapes.h"
#include "CkPmg_Utils_SymbolShapes.h"

#include <MaterialDomain.h>
#include <ProceduralMeshComponent.h>

// --------------------------------------------------------------------------------------------------------------------

namespace
{
    auto
        DoGenerateDonutMesh(
            UProceduralMeshComponent* InMeshComponent,
            float InInnerRadius,
            float InOuterRadius,
            int32 InSegments,
            float InFillAngle,
            ECk_Pmg_RenderMode InRenderMode)
        -> void
    {
        CK_ENSURE_IF_NOT(ck::IsValid(InMeshComponent), TEXT("Invalid mesh component"))
        { return; }

        CK_ENSURE_IF_NOT(InInnerRadius < InOuterRadius, TEXT("Inner radius must be less than outer radius"))
        { return; }

        CK_ENSURE_IF_NOT(InSegments >= 3, TEXT("Segments must be at least 3"))
        { return; }

        auto Vertices = TArray<FVector>{};
        auto Triangles = TArray<int32>{};
        auto Normals = TArray<FVector>{};
        auto UVs = TArray<FVector2D>{};

        const auto FillRadians = FMath::DegreesToRadians(FMath::Clamp(InFillAngle, 0.0f, 360.0f));
        const auto SegmentsToGenerate = InFillAngle >= 360.0f ? InSegments : InSegments + 1;

        // Generate vertices for top face
        for (auto i = 0; i < SegmentsToGenerate; ++i)
        {
            const auto Angle = (FillRadians * i) / InSegments;
            const auto CosAngle = FMath::Cos(Angle);
            const auto SinAngle = FMath::Sin(Angle);

            Vertices.Add(FVector{
                CosAngle * InOuterRadius,
                SinAngle * InOuterRadius,
                0.0f
            });

            Vertices.Add(FVector{
                CosAngle * InInnerRadius,
                SinAngle * InInnerRadius,
                0.0f
            });

            const auto UVAngle = static_cast<float>(i) / InSegments;
            UVs.Add(FVector2D{UVAngle, 1.0f});
            UVs.Add(FVector2D{UVAngle, 0.0f});

            Normals.Add(FVector::UpVector);
            Normals.Add(FVector::UpVector);
        }

        const auto TopVertexCount = Vertices.Num();

        // Generate vertices for bottom face (same positions, reversed normals) if double-sided
        if (InRenderMode == ECk_Pmg_RenderMode::DoubleSided)
        {
            for (auto i = 0; i < SegmentsToGenerate; ++i)
            {
                const auto Angle = (FillRadians * i) / InSegments;
                const auto CosAngle = FMath::Cos(Angle);
                const auto SinAngle = FMath::Sin(Angle);

                Vertices.Add(FVector{
                    CosAngle * InOuterRadius,
                    SinAngle * InOuterRadius,
                    0.0f
                });

                Vertices.Add(FVector{
                    CosAngle * InInnerRadius,
                    SinAngle * InInnerRadius,
                    0.0f
                });

                const auto UVAngle = static_cast<float>(i) / InSegments;
                UVs.Add(FVector2D{UVAngle, 1.0f});
                UVs.Add(FVector2D{UVAngle, 0.0f});

                Normals.Add(FVector::DownVector);
                Normals.Add(FVector::DownVector);
            }
        }

        const auto TriangleCount = InFillAngle >= 360.0f ? InSegments : InSegments;

        // Generate triangles for top face
        for (auto i = 0; i < TriangleCount; ++i)
        {
            const auto Current = i * 2;
            const auto Next = ((i + 1) % SegmentsToGenerate) * 2;

            Triangles.Add(Current);
            Triangles.Add(Next);
            Triangles.Add(Current + 1);

            Triangles.Add(Next);
            Triangles.Add(Next + 1);
            Triangles.Add(Current + 1);
        }

        // Generate triangles for bottom face (reversed winding) if double-sided
        if (InRenderMode == ECk_Pmg_RenderMode::DoubleSided)
        {
            for (auto i = 0; i < TriangleCount; ++i)
            {
                const auto Current = TopVertexCount + i * 2;
                const auto Next = TopVertexCount + ((i + 1) % SegmentsToGenerate) * 2;

                Triangles.Add(Current);
                Triangles.Add(Current + 1);
                Triangles.Add(Next);

                Triangles.Add(Next);
                Triangles.Add(Current + 1);
                Triangles.Add(Next + 1);
            }
        }

        ck::pmg::Verbose(TEXT("Generated donut mesh: Vertices=[{}], Triangles=[{}], Segments=[{}]"),
            Vertices.Num(), Triangles.Num(), InSegments);

        InMeshComponent->CreateMeshSection_LinearColor(
            0,
            Vertices,
            Triangles,
            Normals,
            UVs,
            TArray<FLinearColor>{},
            TArray<FProcMeshTangent>{},
            true
        );
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_Pmg_Donut_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Pmg_Donut_Params& InParams,
            FFragment_Pmg_Donut_Current& InCurrent)
            -> void
    {
        InHandle.Remove<MarkedDirtyBy>();

        ck::pmg::Verbose(TEXT("Setting up Pmg Donut [{}]"), InHandle);

        const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
        CK_ENSURE_IF_NOT(ck::IsValid(World),
            TEXT("Pmg Donut [{}] could not get valid World"), InHandle)
        { return; }

        auto MeshComponent = NewObject<UProceduralMeshComponent>(
            World,
            UProceduralMeshComponent::StaticClass(),
            NAME_None,
            RF_Transient);

        CK_ENSURE_IF_NOT(ck::IsValid(MeshComponent),
            TEXT("Pmg Donut [{}] failed to create ProceduralMeshComponent"), InHandle)
        { return; }

        MeshComponent->SetWorldLocation(FVector::ZeroVector);
        MeshComponent->RegisterComponentWithWorld(World);

        if (MeshComponent->HasBegunPlay() == false)
        {
            MeshComponent->BeginPlay();
        }

        MeshComponent->SetVisibility(true);
        MeshComponent->SetHiddenInGame(false);
        MeshComponent->SetCastShadow(true);

        InCurrent._InnerRadius = InParams.Get_Params().Get_InnerRadius();
        InCurrent._OuterRadius = InParams.Get_Params().Get_OuterRadius();
        InCurrent._Segments = InParams.Get_Params().Get_Segments();
        InCurrent._FillAngle = InParams.Get_Params().Get_FillAngle();
        InCurrent._Material = InParams.Get_Params().Get_Material();
        InCurrent._EnableCollision = InParams.Get_Params().Get_EnableCollision();
        InCurrent._RenderMode = InParams.Get_Params().Get_RenderMode();

        DoGenerateDonutMesh(
            MeshComponent,
            InCurrent._InnerRadius,
            InCurrent._OuterRadius,
            InCurrent._Segments,
            InCurrent._FillAngle,
            InCurrent._RenderMode);

        const auto ShouldBeVisible = InCurrent._RenderMode != ECk_Pmg_RenderMode::Hidden;
        MeshComponent->SetVisibility(ShouldBeVisible, true);
        MeshComponent->SetHiddenInGame(!ShouldBeVisible);
        MeshComponent->bHiddenInGame = !ShouldBeVisible;
        MeshComponent->SetRenderInMainPass(true);
        MeshComponent->SetRenderInDepthPass(true);

        if (ck::IsValid(InCurrent._Material))
        {
            MeshComponent->SetMaterial(0, InCurrent._Material);
        }
        else
        {
            ck::pmg::Warning(TEXT("No material specified for Pmg Donut [{}] - using default material"), InHandle);
            auto DefaultMaterial = UMaterial::GetDefaultMaterial(MD_Surface);
            if (ck::IsValid(DefaultMaterial))
            {
                MeshComponent->SetMaterial(0, DefaultMaterial);
            }
        }

        MeshComponent->SetCollisionEnabled(
            InCurrent._EnableCollision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);

        MeshComponent->UpdateBounds();
        MeshComponent->MarkRenderStateDirty();

        InCurrent._MeshComponent = TStrongObjectPtr{MeshComponent};

        if (InHandle.Has<ck::FFragment_Transform>())
        {
            const auto& Transform = InHandle.Get<ck::FFragment_Transform>();
            const auto& CurrentTransform = Transform.Get_Transform();
            MeshComponent->SetWorldTransform(CurrentTransform);
        }
        else
        {
            ck::pmg::Warning(TEXT("Pmg Donut [{}] has NO Transform fragment - mesh will be at world origin (0,0,0)"), InHandle);
        }

        ck::pmg::Verbose(TEXT("Pmg Donut [{}] setup complete"), InHandle);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Pmg_Donut_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Pmg_Donut_Current& InCurrent,
            const FFragment_Pmg_Donut_UpdateParams& InRequest) const
            -> void
    {
        DoHandleRequest(InHandle, InCurrent, InRequest);
        InHandle.Remove<MarkedDirtyBy>();
    }

    auto
        FProcessor_Pmg_Donut_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_Pmg_Donut_Current& InCurrent,
            const FCk_Request_Pmg_Donut_UpdateParams& InRequest)
            -> void
    {
        ck::pmg::Verbose(TEXT("Handling update params request for Pmg Donut [{}]"), InHandle);

        auto MeshComponent = InCurrent._MeshComponent.Get();
        CK_ENSURE_IF_NOT(ck::IsValid(MeshComponent),
            TEXT("Pmg Donut [{}] has invalid mesh component"), InHandle)
        { return; }

        auto NeedsRegeneration = false;

        if (InRequest.Get_InnerRadius().IsSet())
        {
            InCurrent._InnerRadius = InRequest.Get_InnerRadius().GetValue();
            NeedsRegeneration = true;
        }

        if (InRequest.Get_OuterRadius().IsSet())
        {
            InCurrent._OuterRadius = InRequest.Get_OuterRadius().GetValue();
            NeedsRegeneration = true;
        }

        if (InRequest.Get_Segments().IsSet())
        {
            InCurrent._Segments = InRequest.Get_Segments().GetValue();
            NeedsRegeneration = true;
        }

        if (InRequest.Get_FillAngle().IsSet())
        {
            InCurrent._FillAngle = InRequest.Get_FillAngle().GetValue();
            NeedsRegeneration = true;
        }

        if (InRequest.Get_Material().IsSet())
        {
            InCurrent._Material = InRequest.Get_Material().GetValue();
            if (ck::IsValid(InCurrent._Material))
            {
                MeshComponent->SetMaterial(0, InCurrent._Material);
            }
        }

        if (InRequest.Get_EnableCollision().IsSet())
        {
            InCurrent._EnableCollision = InRequest.Get_EnableCollision().GetValue();
            MeshComponent->SetCollisionEnabled(
                InCurrent._EnableCollision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
        }

        if (InRequest.Get_RenderMode().IsSet())
        {
            const auto OldRenderMode = InCurrent._RenderMode;
            InCurrent._RenderMode = InRequest.Get_RenderMode().GetValue();

            const auto ShouldBeVisible = InCurrent._RenderMode != ECk_Pmg_RenderMode::Hidden;
            MeshComponent->SetVisibility(ShouldBeVisible, true);
            MeshComponent->SetHiddenInGame(!ShouldBeVisible);

            const auto OldIsDoubleSided = OldRenderMode == ECk_Pmg_RenderMode::DoubleSided;
            const auto NewIsDoubleSided = InCurrent._RenderMode == ECk_Pmg_RenderMode::DoubleSided;
            if (OldIsDoubleSided != NewIsDoubleSided)
            {
                NeedsRegeneration = true;
            }
        }

        if (NeedsRegeneration)
        {
            CK_ENSURE_IF_NOT(InCurrent._InnerRadius < InCurrent._OuterRadius,
                TEXT("Inner radius must be less than outer radius for Pmg Donut [{}]"), InHandle)
            { return; }

            MeshComponent->ClearAllMeshSections();
            DoGenerateDonutMesh(
                MeshComponent,
                InCurrent._InnerRadius,
                InCurrent._OuterRadius,
                InCurrent._Segments,
                InCurrent._FillAngle,
                InCurrent._RenderMode);

            ck::pmg::Verbose(TEXT("Pmg Donut [{}] mesh regenerated"), InHandle);
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Pmg_Donut_UpdateTransform::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Pmg_Donut_Current& InCurrent,
            const FFragment_Transform& InTransform)
            -> void
    {
        auto MeshComponent = InCurrent._MeshComponent.Get();
        CK_ENSURE_IF_NOT(ck::IsValid(MeshComponent),
            TEXT("Pmg Donut [{}] has invalid mesh component"), InHandle)
        { return; }

        const auto& CurrentTransform = InTransform.Get_Transform();
        MeshComponent->SetWorldTransform(CurrentTransform);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Pmg_Donut_EndPlay::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Pmg_Donut_Current& InCurrent)
            -> void
    {
        ck::pmg::Verbose(TEXT("Tearing down Pmg Donut [{}]"), InHandle);

        auto MeshComponent = InCurrent._MeshComponent.Get();
        if (ck::IsValid(MeshComponent))
        {
            MeshComponent->DestroyComponent();
        }

        InCurrent._MeshComponent.Reset();
    }
}

// --------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------

namespace
{
    // Helper to get rotation quaternion for axis orientation
    // XY = default (no rotation), XZ = rotate to lie on XZ plane, YZ = rotate to lie on YZ plane
    auto GetAxisRotation(ECk_Plane_Axis InAxis) -> FQuat
    {
        switch (InAxis)
        {
            case ECk_Plane_Axis::XY: return FQuat::Identity;
            case ECk_Plane_Axis::XZ: return FQuat(FVector::ForwardVector, PI * 0.5f);
            case ECk_Plane_Axis::YZ: return FQuat(FVector::RightVector, -PI * 0.5f);
            default: return FQuat::Identity;
        }
    }

    // Apply axis rotation to all vertices and normals in-place
    auto ApplyAxisRotation(
        TArray<FVector>& InOutVertices,
        TArray<FVector>& InOutNormals,
        ECk_Plane_Axis InAxis)
        -> void
    {
        if (InAxis == ECk_Plane_Axis::XY)
        { return; }

        const auto Rotation = GetAxisRotation(InAxis);

        for (auto& Vertex : InOutVertices)
        {
            Vertex = Rotation.RotateVector(Vertex);
        }

        for (auto& Normal : InOutNormals)
        {
            Normal = Rotation.RotateVector(Normal);
        }
    }

    auto
        GenerateDebugShape_Sphere(
            UProceduralMeshComponent* InMeshComponent,
            float InRadius,
            int32 InSegments,
            int32 InRings,
            ECk_Plane_Axis InAxis)
        -> void
    {
        auto Vertices = TArray<FVector>{};
        auto Triangles = TArray<int32>{};
        auto Normals = TArray<FVector>{};
        auto UVs = TArray<FVector2D>{};

        for (auto Ring = 0; Ring <= InRings; ++Ring)
        {
            const auto Phi = PI * Ring / InRings;
            const auto RingRadius = InRadius * FMath::Sin(Phi);
            const auto RingHeight = InRadius * FMath::Cos(Phi);

            for (auto Segment = 0; Segment <= InSegments; ++Segment)
            {
                const auto Theta = 2.0f * PI * Segment / InSegments;
                const auto X = RingRadius * FMath::Cos(Theta);
                const auto Y = RingRadius * FMath::Sin(Theta);

                const auto Vertex = FVector(X, Y, RingHeight);
                Vertices.Add(Vertex);
                Normals.Add(Vertex.GetSafeNormal());
                UVs.Add(FVector2D(
                    static_cast<float>(Segment) / InSegments,
                    static_cast<float>(Ring) / InRings
                ));
            }
        }

        for (auto Ring = 0; Ring < InRings; ++Ring)
        {
            for (auto Segment = 0; Segment < InSegments; ++Segment)
            {
                const auto Current = Ring * (InSegments + 1) + Segment;
                const auto Next = Current + InSegments + 1;

                Triangles.Add(Current);
                Triangles.Add(Current + 1);
                Triangles.Add(Next);

                Triangles.Add(Current + 1);
                Triangles.Add(Next + 1);
                Triangles.Add(Next);
            }
        }

        ApplyAxisRotation(Vertices, Normals, InAxis);

        InMeshComponent->CreateMeshSection_LinearColor(
            0, Vertices, Triangles, Normals, UVs,
            TArray<FLinearColor>{}, TArray<FProcMeshTangent>{}, true);
    }

    auto
        GenerateDebugShape_Star(
            UProceduralMeshComponent* InMeshComponent,
            float InRadius,
            int32 InPoints,
            float InInnerRadiusRatio,
            ECk_Plane_Axis InAxis)
        -> void
    {
        auto Vertices = TArray<FVector>{};
        auto Triangles = TArray<int32>{};
        auto Normals = TArray<FVector>{};
        auto UVs = TArray<FVector2D>{};

        const auto InnerRadius = InRadius * InInnerRadiusRatio;

        // Top face
        Vertices.Add(FVector::ZeroVector);
        Normals.Add(FVector::UpVector);
        UVs.Add(FVector2D(0.5f, 0.5f));

        for (auto i = 0; i < InPoints * 2; ++i)
        {
            const auto Angle = 2.0f * PI * i / (InPoints * 2);
            const auto Radius = (i % 2 == 0) ? InRadius : InnerRadius;
            const auto X = Radius * FMath::Cos(Angle - PI * 0.5f);
            const auto Y = Radius * FMath::Sin(Angle - PI * 0.5f);

            Vertices.Add(FVector(X, Y, 0.0f));
            Normals.Add(FVector::UpVector);
            UVs.Add(FVector2D(
                0.5f + 0.5f * X / InRadius,
                0.5f + 0.5f * Y / InRadius));
        }

        for (auto i = 0; i < InPoints * 2; ++i)
        {
            const auto Next = (i + 1) % (InPoints * 2);
            Triangles.Add(0);
            Triangles.Add(i + 1);
            Triangles.Add(Next + 1);
        }

        // Bottom face
        const auto BottomCenterIndex = Vertices.Num();
        Vertices.Add(FVector::ZeroVector);
        Normals.Add(FVector::DownVector);
        UVs.Add(FVector2D(0.5f, 0.5f));

        for (auto i = 0; i < InPoints * 2; ++i)
        {
            const auto Angle = 2.0f * PI * i / (InPoints * 2);
            const auto Radius = (i % 2 == 0) ? InRadius : InnerRadius;
            const auto X = Radius * FMath::Cos(Angle - PI * 0.5f);
            const auto Y = Radius * FMath::Sin(Angle - PI * 0.5f);

            Vertices.Add(FVector(X, Y, 0.0f));
            Normals.Add(FVector::DownVector);
            UVs.Add(FVector2D(
                0.5f + 0.5f * X / InRadius,
                0.5f + 0.5f * Y / InRadius));
        }

        for (auto i = 0; i < InPoints * 2; ++i)
        {
            const auto Next = (i + 1) % (InPoints * 2);
            Triangles.Add(BottomCenterIndex);
            Triangles.Add(BottomCenterIndex + Next + 1);
            Triangles.Add(BottomCenterIndex + i + 1);
        }

        ApplyAxisRotation(Vertices, Normals, InAxis);

        InMeshComponent->CreateMeshSection_LinearColor(
            0, Vertices, Triangles, Normals, UVs,
            TArray<FLinearColor>{}, TArray<FProcMeshTangent>{}, true);
    }

    auto
        GenerateDebugShape_Plane(
            UProceduralMeshComponent* InMeshComponent,
            float InWidth,
            float InHeight,
            ECk_Plane_Axis InAxis)
        -> void
    {
        auto Vertices = TArray<FVector>{};
        auto Triangles = TArray<int32>{};
        auto Normals = TArray<FVector>{};
        auto UVs = TArray<FVector2D>{};

        const auto HalfWidth = InWidth * 0.5f;
        const auto HalfHeight = InHeight * 0.5f;

        // Top face
        Vertices.Add(FVector(-HalfWidth, -HalfHeight, 0.0f));
        Vertices.Add(FVector(HalfWidth, -HalfHeight, 0.0f));
        Vertices.Add(FVector(HalfWidth, HalfHeight, 0.0f));
        Vertices.Add(FVector(-HalfWidth, HalfHeight, 0.0f));

        for (auto i = 0; i < 4; ++i)
        {
            Normals.Add(FVector::UpVector);
        }

        UVs.Add(FVector2D(0, 0));
        UVs.Add(FVector2D(1, 0));
        UVs.Add(FVector2D(1, 1));
        UVs.Add(FVector2D(0, 1));

        Triangles.Add(0);
        Triangles.Add(1);
        Triangles.Add(2);
        Triangles.Add(0);
        Triangles.Add(2);
        Triangles.Add(3);

        // Bottom face
        const auto BottomStart = Vertices.Num();
        Vertices.Add(FVector(-HalfWidth, -HalfHeight, 0.0f));
        Vertices.Add(FVector(HalfWidth, -HalfHeight, 0.0f));
        Vertices.Add(FVector(HalfWidth, HalfHeight, 0.0f));
        Vertices.Add(FVector(-HalfWidth, HalfHeight, 0.0f));

        for (auto i = 0; i < 4; ++i)
        {
            Normals.Add(FVector::DownVector);
        }

        UVs.Add(FVector2D(0, 0));
        UVs.Add(FVector2D(1, 0));
        UVs.Add(FVector2D(1, 1));
        UVs.Add(FVector2D(0, 1));

        Triangles.Add(BottomStart + 0);
        Triangles.Add(BottomStart + 2);
        Triangles.Add(BottomStart + 1);
        Triangles.Add(BottomStart + 0);
        Triangles.Add(BottomStart + 3);
        Triangles.Add(BottomStart + 2);

        ApplyAxisRotation(Vertices, Normals, InAxis);

        InMeshComponent->CreateMeshSection_LinearColor(
            0, Vertices, Triangles, Normals, UVs,
            TArray<FLinearColor>{}, TArray<FProcMeshTangent>{}, true);
    }

    auto
        GenerateDebugShape_Pyramid(
            UProceduralMeshComponent* InMeshComponent,
            float InBaseSize,
            float InHeight,
            ECk_Plane_Axis InAxis)
        -> void
    {
        auto Vertices = TArray<FVector>{};
        auto Triangles = TArray<int32>{};
        auto Normals = TArray<FVector>{};
        auto UVs = TArray<FVector2D>{};

        const auto HalfSize = InBaseSize * 0.5f;
        const auto Apex = FVector(0, 0, InHeight);

        const TArray BaseCorners = {
            FVector(-HalfSize, -HalfSize, 0.0f),
            FVector(HalfSize, -HalfSize, 0.0f),
            FVector(HalfSize, HalfSize, 0.0f),
            FVector(-HalfSize, HalfSize, 0.0f)
        };

        // Four triangular faces
        for (auto i = 0; i < 4; ++i)
        {
            const auto Next = (i + 1) % 4;
            const auto BaseIdx = Vertices.Num();

            Vertices.Add(BaseCorners[i]);
            Vertices.Add(BaseCorners[Next]);
            Vertices.Add(Apex);

            const auto EdgeDir = BaseCorners[Next] - BaseCorners[i];
            const auto ToApex = Apex - BaseCorners[i];
            const auto Normal = FVector::CrossProduct(EdgeDir, ToApex).GetSafeNormal();

            Normals.Add(Normal);
            Normals.Add(Normal);
            Normals.Add(Normal);

            UVs.Add(FVector2D(0, 0));
            UVs.Add(FVector2D(1, 0));
            UVs.Add(FVector2D(0.5f, 1));

            Triangles.Add(BaseIdx + 0);
            Triangles.Add(BaseIdx + 2);
            Triangles.Add(BaseIdx + 1);
        }

        // Base
        const auto BaseStart = Vertices.Num();
        for (const auto& Corner : BaseCorners)
        {
            Vertices.Add(Corner);
            Normals.Add(FVector::DownVector);
        }

        UVs.Add(FVector2D(0, 0));
        UVs.Add(FVector2D(1, 0));
        UVs.Add(FVector2D(1, 1));
        UVs.Add(FVector2D(0, 1));

        Triangles.Add(BaseStart + 0);
        Triangles.Add(BaseStart + 1);
        Triangles.Add(BaseStart + 2);
        Triangles.Add(BaseStart + 0);
        Triangles.Add(BaseStart + 2);
        Triangles.Add(BaseStart + 3);

        ApplyAxisRotation(Vertices, Normals, InAxis);

        InMeshComponent->CreateMeshSection_LinearColor(
            0, Vertices, Triangles, Normals, UVs,
            TArray<FLinearColor>{}, TArray<FProcMeshTangent>{}, true);
    }

    auto
        GenerateDebugShape_Hemisphere(
            UProceduralMeshComponent* InMeshComponent,
            float InRadius,
            int32 InSegments,
            int32 InRings,
            ECk_Plane_Axis InAxis)
        -> void
    {
        auto Vertices = TArray<FVector>{};
        auto Triangles = TArray<int32>{};
        auto Normals = TArray<FVector>{};
        auto UVs = TArray<FVector2D>{};

        // Generate hemisphere (top half only)
        for (auto Ring = 0; Ring <= InRings; ++Ring)
        {
            const auto Phi = PI * 0.5f * Ring / InRings;
            const auto RingRadius = InRadius * FMath::Cos(Phi);
            const auto RingHeight = InRadius * FMath::Sin(Phi);

            for (auto Segment = 0; Segment <= InSegments; ++Segment)
            {
                const auto Theta = 2.0f * PI * Segment / InSegments;
                const auto X = RingRadius * FMath::Cos(Theta);
                const auto Y = RingRadius * FMath::Sin(Theta);

                const auto Vertex = FVector(X, Y, RingHeight);
                Vertices.Add(Vertex);
                Normals.Add(Vertex.GetSafeNormal());
                UVs.Add(FVector2D(
                    static_cast<float>(Segment) / InSegments,
                    static_cast<float>(Ring) / InRings
                ));
            }
        }

        for (auto Ring = 0; Ring < InRings; ++Ring)
        {
            for (auto Segment = 0; Segment < InSegments; ++Segment)
            {
                const auto Current = Ring * (InSegments + 1) + Segment;
                const auto Next = Current + InSegments + 1;

                // CCW winding for outward-facing normals
                Triangles.Add(Current);
                Triangles.Add(Next);
                Triangles.Add(Current + 1);

                Triangles.Add(Current + 1);
                Triangles.Add(Next);
                Triangles.Add(Next + 1);
            }
        }

        // Add flat base
        const auto BaseCenter = Vertices.Num();
        Vertices.Add(FVector::ZeroVector);
        Normals.Add(FVector::DownVector);
        UVs.Add(FVector2D(0.5f, 0.5f));

        for (auto i = 0; i <= InSegments; ++i)
        {
            const auto Angle = 2.0f * PI * i / InSegments;
            Vertices.Add(FVector(
                InRadius * FMath::Cos(Angle),
                InRadius * FMath::Sin(Angle),
                0.0f));
            Normals.Add(FVector::DownVector);
            UVs.Add(FVector2D(
                0.5f + 0.5f * FMath::Cos(Angle),
                0.5f + 0.5f * FMath::Sin(Angle)));
        }

        for (auto i = 0; i < InSegments; ++i)
        {
            Triangles.Add(BaseCenter);
            Triangles.Add(BaseCenter + i + 1);
            Triangles.Add(BaseCenter + i + 2);
        }

        ApplyAxisRotation(Vertices, Normals, InAxis);

        InMeshComponent->CreateMeshSection_LinearColor(
            0, Vertices, Triangles, Normals, UVs,
            TArray<FLinearColor>{}, TArray<FProcMeshTangent>{}, true);
    }

    auto
        GenerateDebugShape_DashedLine(
            UProceduralMeshComponent* InMeshComponent,
            float InLength,
            float InWidth,
            float InDashLength,
            float InGapLength,
            ECk_Plane_Axis InAxis)
        -> void
    {
        auto Vertices = TArray<FVector>{};
        auto Triangles = TArray<int32>{};
        auto Normals = TArray<FVector>{};
        auto UVs = TArray<FVector2D>{};

        const auto HalfWidth = InWidth * 0.5f;
        const auto DashCycleLength = InDashLength + InGapLength;
        auto CurrentPos = 0.0f;

        while (CurrentPos < InLength)
        {
            const auto DashEnd = FMath::Min(CurrentPos + InDashLength, InLength);
            const auto BaseIdx = Vertices.Num();

            // Top face of dash
            Vertices.Add(FVector(CurrentPos, -HalfWidth, 0.0f));
            Vertices.Add(FVector(DashEnd, -HalfWidth, 0.0f));
            Vertices.Add(FVector(DashEnd, HalfWidth, 0.0f));
            Vertices.Add(FVector(CurrentPos, HalfWidth, 0.0f));

            for (auto i = 0; i < 4; ++i) { Normals.Add(FVector::UpVector); }
            UVs.Add(FVector2D(0, 0));
            UVs.Add(FVector2D(1, 0));
            UVs.Add(FVector2D(1, 1));
            UVs.Add(FVector2D(0, 1));

            Triangles.Add(BaseIdx + 0);
            Triangles.Add(BaseIdx + 1);
            Triangles.Add(BaseIdx + 2);
            Triangles.Add(BaseIdx + 0);
            Triangles.Add(BaseIdx + 2);
            Triangles.Add(BaseIdx + 3);

            // Bottom face of dash
            const auto BottomIdx = Vertices.Num();
            Vertices.Add(FVector(CurrentPos, -HalfWidth, 0.0f));
            Vertices.Add(FVector(DashEnd, -HalfWidth, 0.0f));
            Vertices.Add(FVector(DashEnd, HalfWidth, 0.0f));
            Vertices.Add(FVector(CurrentPos, HalfWidth, 0.0f));

            for (auto i = 0; i < 4; ++i) { Normals.Add(FVector::DownVector); }
            UVs.Add(FVector2D(0, 0));
            UVs.Add(FVector2D(1, 0));
            UVs.Add(FVector2D(1, 1));
            UVs.Add(FVector2D(0, 1));

            Triangles.Add(BottomIdx + 0);
            Triangles.Add(BottomIdx + 2);
            Triangles.Add(BottomIdx + 1);
            Triangles.Add(BottomIdx + 0);
            Triangles.Add(BottomIdx + 3);
            Triangles.Add(BottomIdx + 2);

            CurrentPos += DashCycleLength;
        }

        ApplyAxisRotation(Vertices, Normals, InAxis);

        InMeshComponent->CreateMeshSection_LinearColor(
            0, Vertices, Triangles, Normals, UVs,
            TArray<FLinearColor>{}, TArray<FProcMeshTangent>{}, true);
    }

    auto
        GenerateDebugShape_Checkmark(
            UProceduralMeshComponent* InMeshComponent,
            float InSize,
            float InThickness,
            ECk_Plane_Axis InAxis)
        -> void
    {
        auto Vertices = TArray<FVector>{};
        auto Triangles = TArray<int32>{};
        auto Normals = TArray<FVector>{};
        auto UVs = TArray<FVector2D>{};

        const auto HalfThick = InThickness * 0.5f;

        // Short stroke (bottom left to middle)
        const auto ShortStart = FVector(-InSize * 0.5f, -InSize * 0.2f, 0.0f);
        const auto ShortEnd = FVector(0.0f, InSize * 0.3f, 0.0f);

        // Long stroke (middle to top right)
        const auto LongEnd = FVector(InSize * 0.7f, -InSize * 0.5f, 0.0f);

        // Generate short stroke
        const auto ShortDir = (ShortEnd - ShortStart).GetSafeNormal();
        const auto ShortPerp = FVector::CrossProduct(ShortDir, FVector::UpVector) * HalfThick;

        auto BaseIdx = Vertices.Num();
        Vertices.Add(ShortStart - ShortPerp);
        Vertices.Add(ShortStart + ShortPerp);
        Vertices.Add(ShortEnd + ShortPerp);
        Vertices.Add(ShortEnd - ShortPerp);

        for (auto i = 0; i < 4; ++i) { Normals.Add(FVector::UpVector); }
        UVs.Add(FVector2D(0, 0)); UVs.Add(FVector2D(1, 0)); UVs.Add(FVector2D(1, 1)); UVs.Add(FVector2D(0, 1));

        Triangles.Add(BaseIdx + 0); Triangles.Add(BaseIdx + 1); Triangles.Add(BaseIdx + 2);
        Triangles.Add(BaseIdx + 0); Triangles.Add(BaseIdx + 2); Triangles.Add(BaseIdx + 3);

        // Generate long stroke
        const auto LongDir = (LongEnd - ShortEnd).GetSafeNormal();
        const auto LongPerp = FVector::CrossProduct(LongDir, FVector::UpVector) * HalfThick;

        BaseIdx = Vertices.Num();
        Vertices.Add(ShortEnd - LongPerp);
        Vertices.Add(ShortEnd + LongPerp);
        Vertices.Add(LongEnd + LongPerp);
        Vertices.Add(LongEnd - LongPerp);

        for (auto i = 0; i < 4; ++i) { Normals.Add(FVector::UpVector); }
        UVs.Add(FVector2D(0, 0)); UVs.Add(FVector2D(1, 0)); UVs.Add(FVector2D(1, 1)); UVs.Add(FVector2D(0, 1));

        Triangles.Add(BaseIdx + 0); Triangles.Add(BaseIdx + 1); Triangles.Add(BaseIdx + 2);
        Triangles.Add(BaseIdx + 0); Triangles.Add(BaseIdx + 2); Triangles.Add(BaseIdx + 3);

        // Bottom faces
        const auto ShortBottomIdx = Vertices.Num();
        Vertices.Add(ShortStart - ShortPerp);
        Vertices.Add(ShortStart + ShortPerp);
        Vertices.Add(ShortEnd + ShortPerp);
        Vertices.Add(ShortEnd - ShortPerp);

        for (auto i = 0; i < 4; ++i) { Normals.Add(FVector::DownVector); }
        UVs.Add(FVector2D(0, 0)); UVs.Add(FVector2D(1, 0)); UVs.Add(FVector2D(1, 1)); UVs.Add(FVector2D(0, 1));

        Triangles.Add(ShortBottomIdx + 0); Triangles.Add(ShortBottomIdx + 2); Triangles.Add(ShortBottomIdx + 1);
        Triangles.Add(ShortBottomIdx + 0); Triangles.Add(ShortBottomIdx + 3); Triangles.Add(ShortBottomIdx + 2);

        const auto LongBottomIdx = Vertices.Num();
        Vertices.Add(ShortEnd - LongPerp);
        Vertices.Add(ShortEnd + LongPerp);
        Vertices.Add(LongEnd + LongPerp);
        Vertices.Add(LongEnd - LongPerp);

        for (auto i = 0; i < 4; ++i) { Normals.Add(FVector::DownVector); }
        UVs.Add(FVector2D(0, 0)); UVs.Add(FVector2D(1, 0)); UVs.Add(FVector2D(1, 1)); UVs.Add(FVector2D(0, 1));

        Triangles.Add(LongBottomIdx + 0); Triangles.Add(LongBottomIdx + 2); Triangles.Add(LongBottomIdx + 1);
        Triangles.Add(LongBottomIdx + 0); Triangles.Add(LongBottomIdx + 3); Triangles.Add(LongBottomIdx + 2);

        ApplyAxisRotation(Vertices, Normals, InAxis);

        InMeshComponent->CreateMeshSection_LinearColor(
            0, Vertices, Triangles, Normals, UVs,
            TArray<FLinearColor>{}, TArray<FProcMeshTangent>{}, true);
    }

    auto
        GenerateDebugShape_Diamond(
            UProceduralMeshComponent* InMeshComponent,
            float InWidth,
            float InHeight,
            ECk_Plane_Axis InAxis)
        -> void
    {
        auto Vertices = TArray<FVector>{};
        auto Triangles = TArray<int32>{};
        auto Normals = TArray<FVector>{};
        auto UVs = TArray<FVector2D>{};

        const auto HalfWidth = InWidth * 0.5f;
        const auto HalfHeight = InHeight * 0.5f;

        // Top face
        Vertices.Add(FVector::ZeroVector);
        Vertices.Add(FVector(0.0f, HalfWidth, 0.0f));
        Vertices.Add(FVector(HalfHeight, 0.0f, 0.0f));
        Vertices.Add(FVector(0.0f, -HalfWidth, 0.0f));
        Vertices.Add(FVector(-HalfHeight, 0.0f, 0.0f));

        for (auto i = 0; i < 5; ++i)
        {
            Normals.Add(FVector::UpVector);
        }

        UVs.Add(FVector2D(0.5f, 0.5f));
        UVs.Add(FVector2D(0.5f, 1.0f));
        UVs.Add(FVector2D(1.0f, 0.5f));
        UVs.Add(FVector2D(0.5f, 0.0f));
        UVs.Add(FVector2D(0.0f, 0.5f));

        Triangles.Add(0); Triangles.Add(1); Triangles.Add(2);
        Triangles.Add(0); Triangles.Add(2); Triangles.Add(3);
        Triangles.Add(0); Triangles.Add(3); Triangles.Add(4);
        Triangles.Add(0); Triangles.Add(4); Triangles.Add(1);

        // Bottom face
        const auto BottomStart = Vertices.Num();
        Vertices.Add(FVector::ZeroVector);
        Vertices.Add(FVector(0.0f, HalfWidth, 0.0f));
        Vertices.Add(FVector(HalfHeight, 0.0f, 0.0f));
        Vertices.Add(FVector(0.0f, -HalfWidth, 0.0f));
        Vertices.Add(FVector(-HalfHeight, 0.0f, 0.0f));

        for (auto i = 0; i < 5; ++i)
        {
            Normals.Add(FVector::DownVector);
        }

        UVs.Add(FVector2D(0.5f, 0.5f));
        UVs.Add(FVector2D(0.5f, 1.0f));
        UVs.Add(FVector2D(1.0f, 0.5f));
        UVs.Add(FVector2D(0.5f, 0.0f));
        UVs.Add(FVector2D(0.0f, 0.5f));

        Triangles.Add(BottomStart + 0); Triangles.Add(BottomStart + 2); Triangles.Add(BottomStart + 1);
        Triangles.Add(BottomStart + 0); Triangles.Add(BottomStart + 3); Triangles.Add(BottomStart + 2);
        Triangles.Add(BottomStart + 0); Triangles.Add(BottomStart + 4); Triangles.Add(BottomStart + 3);
        Triangles.Add(BottomStart + 0); Triangles.Add(BottomStart + 1); Triangles.Add(BottomStart + 4);

        ApplyAxisRotation(Vertices, Normals, InAxis);

        InMeshComponent->CreateMeshSection_LinearColor(
            0, Vertices, Triangles, Normals, UVs,
            TArray<FLinearColor>{}, TArray<FProcMeshTangent>{}, true);
    }

    auto
        GenerateDebugShape_Pivot(
            UProceduralMeshComponent* InMeshComponent,
            float InSize,
            ECk_Plane_Axis InAxis)
        -> void
    {
        auto Vertices = TArray<FVector>{};
        auto Triangles = TArray<int32>{};
        auto Normals = TArray<FVector>{};
        auto UVs = TArray<FVector2D>{};

        const auto ArrowLength = InSize;
        const auto ShaftWidth = InSize * 0.05f;
        const auto HeadLength = InSize * 0.2f;
        const auto HeadWidth = ShaftWidth * 3.0f;

        // Helper to add an arrow in a specific direction
        auto AddArrow = [&](const FVector& InDirection)
        {
            const auto ShaftEnd = InDirection * (ArrowLength - HeadLength);
            const auto Tip = InDirection * ArrowLength;

            // Determine perpendicular axes
            auto Perp1 = FVector::UpVector;
            if (FMath::Abs(FVector::DotProduct(InDirection, FVector::UpVector)) > 0.99f)
            {
                Perp1 = FVector::ForwardVector;
            }
            Perp1 = FVector::CrossProduct(InDirection, Perp1).GetSafeNormal();
            const auto Perp2 = FVector::CrossProduct(InDirection, Perp1).GetSafeNormal();

            // Shaft (thin box)
            const auto HalfShaft = ShaftWidth * 0.5f;
            auto BaseIdx = Vertices.Num();

            for (auto i = 0; i < 4; ++i)
            {
                const auto Corner = (i == 0 || i == 3) ? -Perp1 : Perp1;
                const auto Along = (i < 2) ? FVector::ZeroVector : ShaftEnd;
                const auto Cross = (i == 1 || i == 2) ? Perp2 * HalfShaft : -Perp2 * HalfShaft;
                Vertices.Add(Along + Corner * HalfShaft + Cross);
                Normals.Add((Corner + Cross).GetSafeNormal());
                UVs.Add(FVector2D(static_cast<float>(i % 2), static_cast<float>(i / 2)));
            }

            Triangles.Add(BaseIdx + 0); Triangles.Add(BaseIdx + 1); Triangles.Add(BaseIdx + 2);
            Triangles.Add(BaseIdx + 0); Triangles.Add(BaseIdx + 2); Triangles.Add(BaseIdx + 3);

            // Head (cone)
            const auto HeadBaseIdx = Vertices.Num();
            const auto HalfHead = HeadWidth * 0.5f;

            Vertices.Add(ShaftEnd + Perp1 * HalfHead + Perp2 * HalfHead);
            Vertices.Add(ShaftEnd + Perp1 * HalfHead - Perp2 * HalfHead);
            Vertices.Add(ShaftEnd - Perp1 * HalfHead - Perp2 * HalfHead);
            Vertices.Add(ShaftEnd - Perp1 * HalfHead + Perp2 * HalfHead);
            Vertices.Add(Tip);

            for (auto i = 0; i < 5; ++i)
            {
                Normals.Add(InDirection);
                UVs.Add(FVector2D(0.5f, 0.5f));
            }

            // Arrow head triangles - reversed winding for correct normals
            Triangles.Add(HeadBaseIdx + 0); Triangles.Add(HeadBaseIdx + 4); Triangles.Add(HeadBaseIdx + 1);
            Triangles.Add(HeadBaseIdx + 1); Triangles.Add(HeadBaseIdx + 4); Triangles.Add(HeadBaseIdx + 2);
            Triangles.Add(HeadBaseIdx + 2); Triangles.Add(HeadBaseIdx + 4); Triangles.Add(HeadBaseIdx + 3);
            Triangles.Add(HeadBaseIdx + 3); Triangles.Add(HeadBaseIdx + 4); Triangles.Add(HeadBaseIdx + 0);
        };

        // Add three arrows for X, Y, Z axes
        AddArrow(FVector::ForwardVector);  // X - Red typically
        AddArrow(FVector::RightVector);    // Y - Green typically
        AddArrow(FVector::UpVector);       // Z - Blue typically

        ApplyAxisRotation(Vertices, Normals, InAxis);

        InMeshComponent->CreateMeshSection_LinearColor(
            0, Vertices, Triangles, Normals, UVs,
            TArray<FLinearColor>{}, TArray<FProcMeshTangent>{}, true);
    }


    auto
        GenerateDebugShape_Box(
            UProceduralMeshComponent* InMeshComponent,
            const FVector& InExtent,
            ECk_Plane_Axis InAxis)
        -> void
    {
        auto Vertices = TArray<FVector>{};
        auto Triangles = TArray<int32>{};
        auto Normals = TArray<FVector>{};
        auto UVs = TArray<FVector2D>{};

        const TArray Corners = {
            FVector(-InExtent.X, -InExtent.Y, -InExtent.Z), FVector( InExtent.X, -InExtent.Y, -InExtent.Z),
            FVector( InExtent.X,  InExtent.Y, -InExtent.Z), FVector(-InExtent.X,  InExtent.Y, -InExtent.Z),
            FVector(-InExtent.X, -InExtent.Y,  InExtent.Z), FVector( InExtent.X, -InExtent.Y,  InExtent.Z),
            FVector( InExtent.X,  InExtent.Y,  InExtent.Z), FVector(-InExtent.X,  InExtent.Y,  InExtent.Z)
        };

        struct FFace { TArray<int32> Indices; FVector Normal; };
        const TArray Faces = {
            FFace{{0, 1, 2, 3}, FVector( 0,  0, -1)},
            FFace{{4, 7, 6, 5}, FVector( 0,  0,  1)},
            FFace{{0, 4, 5, 1}, FVector( 0, -1,  0)},
            FFace{{2, 6, 7, 3}, FVector( 0,  1,  0)},
            FFace{{0, 3, 7, 4}, FVector(-1,  0,  0)},
            FFace{{1, 5, 6, 2}, FVector( 1,  0,  0)}
        };

        for (const auto& Face : Faces)
        {
            const auto BaseIndex = Vertices.Num();

            for (const auto Index : Face.Indices)
            {
                Vertices.Add(Corners[Index]);
                Normals.Add(Face.Normal);
            }

            UVs.Add(FVector2D(0, 0));
            UVs.Add(FVector2D(1, 0));
            UVs.Add(FVector2D(1, 1));
            UVs.Add(FVector2D(0, 1));

            // CCW winding for outward-facing normals
            Triangles.Add(BaseIndex + 0);
            Triangles.Add(BaseIndex + 1);
            Triangles.Add(BaseIndex + 2);

            Triangles.Add(BaseIndex + 0);
            Triangles.Add(BaseIndex + 2);
            Triangles.Add(BaseIndex + 3);
        }

        ApplyAxisRotation(Vertices, Normals, InAxis);

        InMeshComponent->CreateMeshSection_LinearColor(
            0, Vertices, Triangles, Normals, UVs,
            TArray<FLinearColor>{}, TArray<FProcMeshTangent>{}, true);
    }

    auto
        GenerateDebugShape_Circle(
            UProceduralMeshComponent* InMeshComponent,
            float InRadius,
            int32 InSegments,
            bool InDrawDirectionLine,
            ECk_Plane_Axis InAxis)
        -> void
    {
        auto Vertices = TArray<FVector>{};
        auto Triangles = TArray<int32>{};
        auto Normals = TArray<FVector>{};
        auto UVs = TArray<FVector2D>{};

        // Top face (facing up)
        Vertices.Add(FVector::ZeroVector);
        Normals.Add(FVector::UpVector);
        UVs.Add(FVector2D(0.5f, 0.5f));

        for (auto i = 0; i <= InSegments; ++i)
        {
            const auto Angle = 2.0f * PI * i / InSegments;
            const auto X = InRadius * FMath::Cos(Angle);
            const auto Y = InRadius * FMath::Sin(Angle);

            Vertices.Add(FVector(X, Y, 0.0f));
            Normals.Add(FVector::UpVector);
            UVs.Add(FVector2D(
                0.5f + 0.5f * FMath::Cos(Angle),
                0.5f + 0.5f * FMath::Sin(Angle)
            ));
        }

        // CCW winding for up-facing
        for (auto i = 0; i < InSegments; ++i)
        {
            Triangles.Add(0);
            Triangles.Add(i + 1);
            Triangles.Add(i + 2);
        }

        // Bottom face (facing down)
        const auto BottomCenterIndex = Vertices.Num();
        Vertices.Add(FVector::ZeroVector);
        Normals.Add(FVector::DownVector);
        UVs.Add(FVector2D(0.5f, 0.5f));

        for (auto i = 0; i <= InSegments; ++i)
        {
            const auto Angle = 2.0f * PI * i / InSegments;
            const auto X = InRadius * FMath::Cos(Angle);
            const auto Y = InRadius * FMath::Sin(Angle);

            Vertices.Add(FVector(X, Y, 0.0f));
            Normals.Add(FVector::DownVector);
            UVs.Add(FVector2D(
                0.5f + 0.5f * FMath::Cos(Angle),
                0.5f + 0.5f * FMath::Sin(Angle)
            ));
        }

        // CW winding for down-facing
        for (auto i = 0; i < InSegments; ++i)
        {
            Triangles.Add(BottomCenterIndex);
            Triangles.Add(BottomCenterIndex + i + 2);
            Triangles.Add(BottomCenterIndex + i + 1);
        }

        // Add direction line geometry if requested
        if (InDrawDirectionLine)
        {
            const auto LineThickness = InRadius * 0.02f; // 2% of radius for visibility
            const auto DirectionStartIndex = Vertices.Num();

            // Direction line from center to forward (+X)
            // Create a thin rectangle strip
            const auto P1 = FVector(0.0f, -LineThickness, 0.0f);
            const auto P2 = FVector(0.0f, LineThickness, 0.0f);
            const auto P3 = FVector(InRadius, LineThickness, 0.0f);
            const auto P4 = FVector(InRadius, -LineThickness, 0.0f);

            // Top face of direction line
            Vertices.Add(P1);
            Vertices.Add(P2);
            Vertices.Add(P3);
            Vertices.Add(P4);

            Normals.Add(FVector::UpVector);
            Normals.Add(FVector::UpVector);
            Normals.Add(FVector::UpVector);
            Normals.Add(FVector::UpVector);

            UVs.Add(FVector2D(0.0f, 0.0f));
            UVs.Add(FVector2D(0.0f, 1.0f));
            UVs.Add(FVector2D(1.0f, 1.0f));
            UVs.Add(FVector2D(1.0f, 0.0f));

            // Top face triangles (CCW)
            Triangles.Add(DirectionStartIndex + 0);
            Triangles.Add(DirectionStartIndex + 1);
            Triangles.Add(DirectionStartIndex + 2);

            Triangles.Add(DirectionStartIndex + 0);
            Triangles.Add(DirectionStartIndex + 2);
            Triangles.Add(DirectionStartIndex + 3);

            // Bottom face of direction line
            Vertices.Add(P1);
            Vertices.Add(P2);
            Vertices.Add(P3);
            Vertices.Add(P4);

            Normals.Add(FVector::DownVector);
            Normals.Add(FVector::DownVector);
            Normals.Add(FVector::DownVector);
            Normals.Add(FVector::DownVector);

            UVs.Add(FVector2D(0.0f, 0.0f));
            UVs.Add(FVector2D(0.0f, 1.0f));
            UVs.Add(FVector2D(1.0f, 1.0f));
            UVs.Add(FVector2D(1.0f, 0.0f));

            const auto BottomDirectionStartIndex = DirectionStartIndex + 4;

            // Bottom face triangles (CW)
            Triangles.Add(BottomDirectionStartIndex + 0);
            Triangles.Add(BottomDirectionStartIndex + 2);
            Triangles.Add(BottomDirectionStartIndex + 1);

            Triangles.Add(BottomDirectionStartIndex + 0);
            Triangles.Add(BottomDirectionStartIndex + 3);
            Triangles.Add(BottomDirectionStartIndex + 2);
        }

        ApplyAxisRotation(Vertices, Normals, InAxis);

        InMeshComponent->CreateMeshSection_LinearColor(
            0, Vertices, Triangles, Normals, UVs,
            TArray<FLinearColor>{}, TArray<FProcMeshTangent>{}, true);
    }

    auto
        GenerateDebugShape_Cone(
            UProceduralMeshComponent* InMeshComponent,
            float InRadius,
            float InHeight,
            int32 InSegments,
            ECk_Plane_Axis InAxis)
        -> void
    {
        auto Vertices = TArray<FVector>{};
        auto Triangles = TArray<int32>{};
        auto Normals = TArray<FVector>{};
        auto UVs = TArray<FVector2D>{};

        const auto Apex = FVector(0, 0, InHeight);
        Vertices.Add(Apex);
        Normals.Add(FVector::UpVector);
        UVs.Add(FVector2D(0.5f, 1.0f));

        for (auto i = 0; i <= InSegments; ++i)
        {
            const auto Angle = 2.0f * PI * i / InSegments;
            const auto X = InRadius * FMath::Cos(Angle);
            const auto Y = InRadius * FMath::Sin(Angle);

            Vertices.Add(FVector(X, Y, 0.0f));

            const auto EdgeDir = FVector(X, Y, 0.0f) - Apex;
            const auto Tangent = FVector(-Y, X, 0.0f).GetSafeNormal();
            Normals.Add(FVector::CrossProduct(EdgeDir, Tangent).GetSafeNormal());

            UVs.Add(FVector2D(static_cast<float>(i) / InSegments, 0.0f));
        }

        // Reversed winding for correct normals
        for (auto i = 0; i < InSegments; ++i)
        {
            Triangles.Add(0);
            Triangles.Add(i + 2);
            Triangles.Add(i + 1);
        }

        const auto BaseCenter = Vertices.Num();
        Vertices.Add(FVector::ZeroVector);
        Normals.Add(FVector::DownVector);
        UVs.Add(FVector2D(0.5f, 0.5f));

        for (auto i = 0; i <= InSegments; ++i)
        {
            const auto Angle = 2.0f * PI * i / InSegments;
            const auto X = InRadius * FMath::Cos(Angle);
            const auto Y = InRadius * FMath::Sin(Angle);

            Vertices.Add(FVector(X, Y, 0.0f));
            Normals.Add(FVector::DownVector);
            UVs.Add(FVector2D(
                0.5f + 0.5f * FMath::Cos(Angle),
                0.5f + 0.5f * FMath::Sin(Angle)
            ));
        }

        for (auto i = 0; i < InSegments; ++i)
        {
            Triangles.Add(BaseCenter);
            Triangles.Add(BaseCenter + i + 1);
            Triangles.Add(BaseCenter + i + 2);
        }

        ApplyAxisRotation(Vertices, Normals, InAxis);

        InMeshComponent->CreateMeshSection_LinearColor(
            0, Vertices, Triangles, Normals, UVs,
            TArray<FLinearColor>{}, TArray<FProcMeshTangent>{}, true);
    }

    auto
        GenerateDebugShape_Cylinder(
            UProceduralMeshComponent* InMeshComponent,
            float InRadius,
            float InHeight,
            int32 InSegments,
            ECk_Plane_Axis InAxis)
        -> void
    {
        auto Vertices = TArray<FVector>{};
        auto Triangles = TArray<int32>{};
        auto Normals = TArray<FVector>{};
        auto UVs = TArray<FVector2D>{};

        const auto HalfHeight = InHeight * 0.5f;

        for (auto i = 0; i <= InSegments; ++i)
        {
            const auto Angle = 2.0f * PI * i / InSegments;
            const auto X = InRadius * FMath::Cos(Angle);
            const auto Y = InRadius * FMath::Sin(Angle);

            Vertices.Add(FVector(X, Y, -HalfHeight));
            Vertices.Add(FVector(X, Y,  HalfHeight));

            const auto Normal = FVector(X, Y, 0.0f).GetSafeNormal();
            Normals.Add(Normal);
            Normals.Add(Normal);

            const auto U = static_cast<float>(i) / InSegments;
            UVs.Add(FVector2D(U, 0.0f));
            UVs.Add(FVector2D(U, 1.0f));
        }

        // Reversed winding for correct normals
        for (auto i = 0; i < InSegments; ++i)
        {
            const auto Current = i * 2;
            const auto Next = (i + 1) * 2;

            Triangles.Add(Current);
            Triangles.Add(Current + 1);
            Triangles.Add(Next);

            Triangles.Add(Current + 1);
            Triangles.Add(Next + 1);
            Triangles.Add(Next);
        }

        const auto BottomCenter = Vertices.Num();
        Vertices.Add(FVector(0, 0, -HalfHeight));
        Normals.Add(FVector::DownVector);
        UVs.Add(FVector2D(0.5f, 0.5f));

        for (auto i = 0; i <= InSegments; ++i)
        {
            const auto Angle = 2.0f * PI * i / InSegments;
            Vertices.Add(FVector(InRadius * FMath::Cos(Angle), InRadius * FMath::Sin(Angle), -HalfHeight));
            Normals.Add(FVector::DownVector);
            UVs.Add(FVector2D(0.5f + 0.5f * FMath::Cos(Angle), 0.5f + 0.5f * FMath::Sin(Angle)));
        }

        for (auto i = 0; i < InSegments; ++i)
        {
            Triangles.Add(BottomCenter);
            Triangles.Add(BottomCenter + i + 1);
            Triangles.Add(BottomCenter + i + 2);
        }

        const auto TopCenter = Vertices.Num();
        Vertices.Add(FVector(0, 0, HalfHeight));
        Normals.Add(FVector::UpVector);
        UVs.Add(FVector2D(0.5f, 0.5f));

        for (auto i = 0; i <= InSegments; ++i)
        {
            const auto Angle = 2.0f * PI * i / InSegments;
            Vertices.Add(FVector(InRadius * FMath::Cos(Angle), InRadius * FMath::Sin(Angle), HalfHeight));
            Normals.Add(FVector::UpVector);
            UVs.Add(FVector2D(0.5f + 0.5f * FMath::Cos(Angle), 0.5f + 0.5f * FMath::Sin(Angle)));
        }

        for (auto i = 0; i < InSegments; ++i)
        {
            Triangles.Add(TopCenter);
            Triangles.Add(TopCenter + i + 2);
            Triangles.Add(TopCenter + i + 1);
        }

        ApplyAxisRotation(Vertices, Normals, InAxis);

        InMeshComponent->CreateMeshSection_LinearColor(
            0, Vertices, Triangles, Normals, UVs,
            TArray<FLinearColor>{}, TArray<FProcMeshTangent>{}, true);
    }

    auto
        GenerateDebugShape_Capsule(
            UProceduralMeshComponent* InMeshComponent,
            float InRadius,
            float InHalfHeight,
            int32 InSegments,
            int32 InRings,
            ECk_Plane_Axis InAxis)
        -> void
    {
        auto Vertices = TArray<FVector>{};
        auto Triangles = TArray<int32>{};
        auto Normals = TArray<FVector>{};
        auto UVs = TArray<FVector2D>{};

        // Cylinder half-height = total half-height minus radius
        const auto CylinderHalfHeight = FMath::Max(0.0f, InHalfHeight - InRadius);

        // Top hemisphere (centered at +CylinderHalfHeight)
        for (auto Ring = 0; Ring <= InRings; ++Ring)
        {
            const auto Phi = PI * 0.5f * Ring / InRings;
            const auto RingRadius = InRadius * FMath::Cos(Phi);
            const auto RingHeight = InRadius * FMath::Sin(Phi);

            for (auto Segment = 0; Segment <= InSegments; ++Segment)
            {
                const auto Theta = 2.0f * PI * Segment / InSegments;
                const auto X = RingRadius * FMath::Cos(Theta);
                const auto Y = RingRadius * FMath::Sin(Theta);

                const auto TopVertex = FVector(X, Y, CylinderHalfHeight + RingHeight);
                Vertices.Add(TopVertex);
                Normals.Add((TopVertex - FVector(0, 0, CylinderHalfHeight)).GetSafeNormal());
                UVs.Add(FVector2D(static_cast<float>(Segment) / InSegments, static_cast<float>(Ring) / InRings * 0.25f));
            }
        }

        // Reversed winding for correct normals
        for (auto Ring = 0; Ring < InRings; ++Ring)
        {
            for (auto Segment = 0; Segment < InSegments; ++Segment)
            {
                const auto Current = Ring * (InSegments + 1) + Segment;
                const auto Next = Current + InSegments + 1;

                Triangles.Add(Current);
                Triangles.Add(Current + 1);
                Triangles.Add(Next);

                Triangles.Add(Current + 1);
                Triangles.Add(Next + 1);
                Triangles.Add(Next);
            }
        }

        // Cylinder body (only if there's height)
        if (CylinderHalfHeight > 0.0f)
        {
            const auto CylinderStartIndex = Vertices.Num();
            for (auto i = 0; i <= InSegments; ++i)
            {
                const auto Angle = 2.0f * PI * i / InSegments;
                const auto X = InRadius * FMath::Cos(Angle);
                const auto Y = InRadius * FMath::Sin(Angle);
                const auto Normal = FVector(X, Y, 0.0f).GetSafeNormal();

                // Top ring of cylinder
                Vertices.Add(FVector(X, Y, CylinderHalfHeight));
                Normals.Add(Normal);
                UVs.Add(FVector2D(static_cast<float>(i) / InSegments, 0.25f));

                // Bottom ring of cylinder
                Vertices.Add(FVector(X, Y, -CylinderHalfHeight));
                Normals.Add(Normal);
                UVs.Add(FVector2D(static_cast<float>(i) / InSegments, 0.75f));
            }

            for (auto i = 0; i < InSegments; ++i)
            {
                const auto Current = CylinderStartIndex + i * 2;
                const auto Next = CylinderStartIndex + (i + 1) * 2;

                Triangles.Add(Current);
                Triangles.Add(Current + 1);
                Triangles.Add(Next);

                Triangles.Add(Current + 1);
                Triangles.Add(Next + 1);
                Triangles.Add(Next);
            }
        }

        // Bottom hemisphere (centered at -CylinderHalfHeight)
        const auto BottomStartIndex = Vertices.Num();
        for (auto Ring = 0; Ring <= InRings; ++Ring)
        {
            const auto Phi = PI * 0.5f * Ring / InRings;
            const auto RingRadius = InRadius * FMath::Cos(Phi);
            const auto RingHeight = InRadius * FMath::Sin(Phi);

            for (auto Segment = 0; Segment <= InSegments; ++Segment)
            {
                const auto Theta = 2.0f * PI * Segment / InSegments;
                const auto X = RingRadius * FMath::Cos(Theta);
                const auto Y = RingRadius * FMath::Sin(Theta);

                const auto BottomVertex = FVector(X, Y, -CylinderHalfHeight - RingHeight);
                Vertices.Add(BottomVertex);
                Normals.Add((BottomVertex - FVector(0, 0, -CylinderHalfHeight)).GetSafeNormal());
                UVs.Add(FVector2D(static_cast<float>(Segment) / InSegments, 0.75f + static_cast<float>(Ring) / InRings * 0.25f));
            }
        }

        for (auto Ring = 0; Ring < InRings; ++Ring)
        {
            for (auto Segment = 0; Segment < InSegments; ++Segment)
            {
                const auto Current = BottomStartIndex + Ring * (InSegments + 1) + Segment;
                const auto Next = Current + InSegments + 1;

                Triangles.Add(Current);
                Triangles.Add(Next);
                Triangles.Add(Current + 1);

                Triangles.Add(Current + 1);
                Triangles.Add(Next);
                Triangles.Add(Next + 1);
            }
        }

        ApplyAxisRotation(Vertices, Normals, InAxis);

        InMeshComponent->CreateMeshSection_LinearColor(
            0, Vertices, Triangles, Normals, UVs,
            TArray<FLinearColor>{}, TArray<FProcMeshTangent>{}, true);
    }

    auto
        GenerateDebugShape_Arrow(
            UProceduralMeshComponent* InMeshComponent,
            float InLength,
            float InShaftWidth,
            float InArrowHeadRatio,
            float InArrowHeadWidthMultiplier,
            ECk_Plane_Axis InAxis)
        -> void
    {
        auto Vertices = TArray<FVector>{};
        auto Triangles = TArray<int32>{};
        auto Normals = TArray<FVector>{};
        auto UVs = TArray<FVector2D>{};

        const auto HeadLength = InLength * InArrowHeadRatio;
        const auto ShaftLength = InLength - HeadLength;
        const auto HalfShaftWidth = InShaftWidth * 0.5f;
        const auto HeadWidth = InShaftWidth * InArrowHeadWidthMultiplier * 0.5f;

        const auto ShaftBottomLeft = FVector(0.0f, -HalfShaftWidth, 0.0f);
        const auto ShaftBottomRight = FVector(0.0f, HalfShaftWidth, 0.0f);
        const auto ShaftTopRight = FVector(ShaftLength, HalfShaftWidth, 0.0f);
        const auto ShaftTopLeft = FVector(ShaftLength, -HalfShaftWidth, 0.0f);

        const auto HeadBaseLeft = FVector(ShaftLength, -HeadWidth, 0.0f);
        const auto HeadBaseRight = FVector(ShaftLength, HeadWidth, 0.0f);
        const auto HeadTip = FVector(InLength, 0.0f, 0.0f);

        auto TopStartIdx = Vertices.Num();

        Vertices.Add(ShaftBottomLeft);
        Vertices.Add(ShaftBottomRight);
        Vertices.Add(ShaftTopRight);
        Vertices.Add(ShaftTopLeft);
        Vertices.Add(HeadBaseLeft);
        Vertices.Add(HeadBaseRight);
        Vertices.Add(HeadTip);

        for (auto i = 0; i < 7; ++i)
        {
            Normals.Add(FVector::UpVector);
        }

        UVs.Add(FVector2D(0.0f, 0.0f));
        UVs.Add(FVector2D(0.0f, 1.0f));
        UVs.Add(FVector2D(0.6f, 1.0f));
        UVs.Add(FVector2D(0.6f, 0.0f));
        UVs.Add(FVector2D(0.6f, 0.0f));
        UVs.Add(FVector2D(0.6f, 1.0f));
        UVs.Add(FVector2D(1.0f, 0.5f));

        Triangles.Add(TopStartIdx + 0);
        Triangles.Add(TopStartIdx + 1);
        Triangles.Add(TopStartIdx + 2);

        Triangles.Add(TopStartIdx + 0);
        Triangles.Add(TopStartIdx + 2);
        Triangles.Add(TopStartIdx + 3);

        Triangles.Add(TopStartIdx + 4);
        Triangles.Add(TopStartIdx + 5);
        Triangles.Add(TopStartIdx + 6);

        auto BottomStartIdx = Vertices.Num();

        Vertices.Add(ShaftBottomLeft);
        Vertices.Add(ShaftBottomRight);
        Vertices.Add(ShaftTopRight);
        Vertices.Add(ShaftTopLeft);
        Vertices.Add(HeadBaseLeft);
        Vertices.Add(HeadBaseRight);
        Vertices.Add(HeadTip);

        for (auto i = 0; i < 7; ++i)
        {
            Normals.Add(FVector::DownVector);
        }

        UVs.Add(FVector2D(0.0f, 0.0f));
        UVs.Add(FVector2D(0.0f, 1.0f));
        UVs.Add(FVector2D(0.6f, 1.0f));
        UVs.Add(FVector2D(0.6f, 0.0f));
        UVs.Add(FVector2D(0.6f, 0.0f));
        UVs.Add(FVector2D(0.6f, 1.0f));
        UVs.Add(FVector2D(1.0f, 0.5f));

        Triangles.Add(BottomStartIdx + 0);
        Triangles.Add(BottomStartIdx + 2);
        Triangles.Add(BottomStartIdx + 1);

        Triangles.Add(BottomStartIdx + 0);
        Triangles.Add(BottomStartIdx + 3);
        Triangles.Add(BottomStartIdx + 2);

        Triangles.Add(BottomStartIdx + 4);
        Triangles.Add(BottomStartIdx + 6);
        Triangles.Add(BottomStartIdx + 5);

        ApplyAxisRotation(Vertices, Normals, InAxis);

        InMeshComponent->CreateMeshSection_LinearColor(
            0, Vertices, Triangles, Normals, UVs,
            TArray<FLinearColor>{}, TArray<FProcMeshTangent>{}, true);
    }

    auto
        GenerateDebugShape_Ring(
            UProceduralMeshComponent* InMeshComponent,
            float InInnerRadius,
            float InOuterRadius,
            int32 InSegments,
            ECk_Plane_Axis InAxis)
        -> void
    {
        auto Vertices = TArray<FVector>{};
        auto Triangles = TArray<int32>{};
        auto Normals = TArray<FVector>{};
        auto UVs = TArray<FVector2D>{};

        for (auto i = 0; i <= InSegments; ++i)
        {
            const auto Angle = 2.0f * PI * i / InSegments;
            const auto Cos = FMath::Cos(Angle);
            const auto Sin = FMath::Sin(Angle);

            Vertices.Add(FVector(InOuterRadius * Cos, InOuterRadius * Sin, 0.0f));
            Normals.Add(FVector::UpVector);
            UVs.Add(FVector2D(0.5f + 0.5f * Cos, 0.5f + 0.5f * Sin));

            Vertices.Add(FVector(InInnerRadius * Cos, InInnerRadius * Sin, 0.0f));
            Normals.Add(FVector::UpVector);
            UVs.Add(FVector2D(
                0.5f + 0.5f * (InInnerRadius / InOuterRadius) * Cos,
                0.5f + 0.5f * (InInnerRadius / InOuterRadius) * Sin));
        }

        for (auto i = 0; i < InSegments; ++i)
        {
            const auto Current = i * 2;
            const auto Next = (i + 1) * 2;

            Triangles.Add(Current);
            Triangles.Add(Current + 1);
            Triangles.Add(Next);

            Triangles.Add(Current + 1);
            Triangles.Add(Next + 1);
            Triangles.Add(Next);
        }

        const auto BottomStartIdx = Vertices.Num();
        for (auto i = 0; i <= InSegments; ++i)
        {
            const auto Angle = 2.0f * PI * i / InSegments;
            const auto Cos = FMath::Cos(Angle);
            const auto Sin = FMath::Sin(Angle);

            Vertices.Add(FVector(InOuterRadius * Cos, InOuterRadius * Sin, 0.0f));
            Normals.Add(FVector::DownVector);
            UVs.Add(FVector2D(0.5f + 0.5f * Cos, 0.5f + 0.5f * Sin));

            Vertices.Add(FVector(InInnerRadius * Cos, InInnerRadius * Sin, 0.0f));
            Normals.Add(FVector::DownVector);
            UVs.Add(FVector2D(
                0.5f + 0.5f * (InInnerRadius / InOuterRadius) * Cos,
                0.5f + 0.5f * (InInnerRadius / InOuterRadius) * Sin));
        }

        for (auto i = 0; i < InSegments; ++i)
        {
            const auto Current = BottomStartIdx + i * 2;
            const auto Next = BottomStartIdx + (i + 1) * 2;

            Triangles.Add(Current);
            Triangles.Add(Next);
            Triangles.Add(Current + 1);

            Triangles.Add(Current + 1);
            Triangles.Add(Next);
            Triangles.Add(Next + 1);
        }

        ApplyAxisRotation(Vertices, Normals, InAxis);

        InMeshComponent->CreateMeshSection_LinearColor(
            0, Vertices, Triangles, Normals, UVs,
            TArray<FLinearColor>{}, TArray<FProcMeshTangent>{}, true);
    }

    auto
        GenerateDebugShape_Wedge(
            UProceduralMeshComponent* InMeshComponent,
            float InRadius,
            float InStartAngle,
            float InEndAngle,
            int32 InSegments,
            ECk_Plane_Axis InAxis)
        -> void
    {
        auto Vertices = TArray<FVector>{};
        auto Triangles = TArray<int32>{};
        auto Normals = TArray<FVector>{};
        auto UVs = TArray<FVector2D>{};

        const auto StartRad = FMath::DegreesToRadians(InStartAngle);
        const auto EndRad = FMath::DegreesToRadians(InEndAngle);
        auto AngleRange = EndRad - StartRad;

        while (AngleRange < 0.0f) { AngleRange += 2.0f * PI; }
        while (AngleRange > 2.0f * PI) { AngleRange -= 2.0f * PI; }

        const auto SegmentsToUse = FMath::Max(3, static_cast<int32>(InSegments * (AngleRange / (2.0f * PI))));

        Vertices.Add(FVector::ZeroVector);
        Normals.Add(FVector::UpVector);
        UVs.Add(FVector2D(0.5f, 0.5f));

        for (auto i = 0; i <= SegmentsToUse; ++i)
        {
            const auto T = static_cast<float>(i) / SegmentsToUse;
            const auto Angle = StartRad + AngleRange * T;
            const auto X = InRadius * FMath::Cos(Angle);
            const auto Y = InRadius * FMath::Sin(Angle);

            Vertices.Add(FVector(X, Y, 0.0f));
            Normals.Add(FVector::UpVector);
            UVs.Add(FVector2D(
                0.5f + 0.5f * FMath::Cos(Angle),
                0.5f + 0.5f * FMath::Sin(Angle)));
        }

        for (auto i = 0; i < SegmentsToUse; ++i)
        {
            Triangles.Add(0);
            Triangles.Add(i + 1);
            Triangles.Add(i + 2);
        }

        const auto BottomCenterIndex = Vertices.Num();
        Vertices.Add(FVector::ZeroVector);
        Normals.Add(FVector::DownVector);
        UVs.Add(FVector2D(0.5f, 0.5f));

        for (auto i = 0; i <= SegmentsToUse; ++i)
        {
            const auto T = static_cast<float>(i) / SegmentsToUse;
            const auto Angle = StartRad + AngleRange * T;
            const auto X = InRadius * FMath::Cos(Angle);
            const auto Y = InRadius * FMath::Sin(Angle);

            Vertices.Add(FVector(X, Y, 0.0f));
            Normals.Add(FVector::DownVector);
            UVs.Add(FVector2D(
                0.5f + 0.5f * FMath::Cos(Angle),
                0.5f + 0.5f * FMath::Sin(Angle)));
        }

        for (auto i = 0; i < SegmentsToUse; ++i)
        {
            Triangles.Add(BottomCenterIndex);
            Triangles.Add(BottomCenterIndex + i + 2);
            Triangles.Add(BottomCenterIndex + i + 1);
        }

        ApplyAxisRotation(Vertices, Normals, InAxis);

        InMeshComponent->CreateMeshSection_LinearColor(
            0, Vertices, Triangles, Normals, UVs,
            TArray<FLinearColor>{}, TArray<FProcMeshTangent>{}, true);
    }

    auto
        GenerateDebugShape_Frustum(
            UProceduralMeshComponent* InMeshComponent,
            float InNearWidth,
            float InNearHeight,
            float InFarWidth,
            float InFarHeight,
            float InLength,
            ECk_Plane_Axis InAxis)
        -> void
    {
        auto Vertices = TArray<FVector>{};
        auto Triangles = TArray<int32>{};
        auto Normals = TArray<FVector>{};
        auto UVs = TArray<FVector2D>{};

        const auto NearHW = InNearWidth * 0.5f;
        const auto NearHH = InNearHeight * 0.5f;
        const auto FarHW = InFarWidth * 0.5f;
        const auto FarHH = InFarHeight * 0.5f;

        const auto NearTL = FVector(0.0f, -NearHW,  NearHH);
        const auto NearTR = FVector(0.0f,  NearHW,  NearHH);
        const auto NearBR = FVector(0.0f,  NearHW, -NearHH);
        const auto NearBL = FVector(0.0f, -NearHW, -NearHH);

        const auto FarTL = FVector(InLength, -FarHW,  FarHH);
        const auto FarTR = FVector(InLength,  FarHW,  FarHH);
        const auto FarBR = FVector(InLength,  FarHW, -FarHH);
        const auto FarBL = FVector(InLength, -FarHW, -FarHH);

        struct FFace { TArray<FVector> Corners; FVector Normal; };

        auto Faces = TArray<FFace>{};

        Faces.Add(FFace{{NearTL, NearTR, NearBR, NearBL}, FVector(-1, 0, 0)});
        Faces.Add(FFace{{FarTR, FarTL, FarBL, FarBR}, FVector(1, 0, 0)});
        Faces.Add(FFace{{NearTL, FarTL, FarTR, NearTR},
            FVector::CrossProduct((FarTL - NearTL).GetSafeNormal(), (NearTR - NearTL).GetSafeNormal()).GetSafeNormal()});
        Faces.Add(FFace{{NearBL, NearBR, FarBR, FarBL},
            FVector::CrossProduct((NearBR - NearBL).GetSafeNormal(), (FarBL - NearBL).GetSafeNormal()).GetSafeNormal()});
        Faces.Add(FFace{{NearTL, NearBL, FarBL, FarTL},
            FVector::CrossProduct((NearBL - NearTL).GetSafeNormal(), (FarTL - NearTL).GetSafeNormal()).GetSafeNormal()});
        Faces.Add(FFace{{NearTR, FarTR, FarBR, NearBR},
            FVector::CrossProduct((FarTR - NearTR).GetSafeNormal(), (NearBR - NearTR).GetSafeNormal()).GetSafeNormal()});

        for (const auto& Face : Faces)
        {
            const auto BaseIndex = Vertices.Num();

            for (const auto& Corner : Face.Corners)
            {
                Vertices.Add(Corner);
                Normals.Add(Face.Normal);
            }

            UVs.Add(FVector2D(0, 0));
            UVs.Add(FVector2D(1, 0));
            UVs.Add(FVector2D(1, 1));
            UVs.Add(FVector2D(0, 1));

            Triangles.Add(BaseIndex + 0);
            Triangles.Add(BaseIndex + 1);
            Triangles.Add(BaseIndex + 2);

            Triangles.Add(BaseIndex + 0);
            Triangles.Add(BaseIndex + 2);
            Triangles.Add(BaseIndex + 3);
        }

        ApplyAxisRotation(Vertices, Normals, InAxis);

        InMeshComponent->CreateMeshSection_LinearColor(
            0, Vertices, Triangles, Normals, UVs,
            TArray<FLinearColor>{}, TArray<FProcMeshTangent>{}, true);
    }

    auto
        GenerateDebugShape_Arc(
            UProceduralMeshComponent* InMeshComponent,
            float InRadius,
            float InStartAngle,
            float InEndAngle,
            float InThickness,
            int32 InSegments,
            ECk_Plane_Axis InAxis)
        -> void
    {
        auto Vertices = TArray<FVector>{};
        auto Triangles = TArray<int32>{};
        auto Normals = TArray<FVector>{};
        auto UVs = TArray<FVector2D>{};

        const auto StartRad = FMath::DegreesToRadians(InStartAngle);
        const auto EndRad = FMath::DegreesToRadians(InEndAngle);
        auto AngleRange = EndRad - StartRad;

        while (AngleRange < 0.0f) { AngleRange += 2.0f * PI; }
        while (AngleRange > 2.0f * PI) { AngleRange -= 2.0f * PI; }

        const auto SegmentsToUse = FMath::Max(3, static_cast<int32>(InSegments * (AngleRange / (2.0f * PI))));
        const auto HalfThickness = InThickness * 0.5f;

        for (auto i = 0; i <= SegmentsToUse; ++i)
        {
            const auto T = static_cast<float>(i) / SegmentsToUse;
            const auto Angle = StartRad + AngleRange * T;
            const auto Cos = FMath::Cos(Angle);
            const auto Sin = FMath::Sin(Angle);

            const auto InnerX = (InRadius - HalfThickness) * Cos;
            const auto InnerY = (InRadius - HalfThickness) * Sin;
            const auto OuterX = (InRadius + HalfThickness) * Cos;
            const auto OuterY = (InRadius + HalfThickness) * Sin;

            Vertices.Add(FVector(OuterX, OuterY, 0.0f));
            Normals.Add(FVector::UpVector);
            UVs.Add(FVector2D(T, 1.0f));

            Vertices.Add(FVector(InnerX, InnerY, 0.0f));
            Normals.Add(FVector::UpVector);
            UVs.Add(FVector2D(T, 0.0f));
        }

        for (auto i = 0; i < SegmentsToUse; ++i)
        {
            const auto Current = i * 2;
            const auto Next = (i + 1) * 2;

            Triangles.Add(Current);
            Triangles.Add(Current + 1);
            Triangles.Add(Next);

            Triangles.Add(Current + 1);
            Triangles.Add(Next + 1);
            Triangles.Add(Next);
        }

        const auto BottomStartIdx = Vertices.Num();
        for (auto i = 0; i <= SegmentsToUse; ++i)
        {
            const auto T = static_cast<float>(i) / SegmentsToUse;
            const auto Angle = StartRad + AngleRange * T;
            const auto Cos = FMath::Cos(Angle);
            const auto Sin = FMath::Sin(Angle);

            const auto InnerX = (InRadius - HalfThickness) * Cos;
            const auto InnerY = (InRadius - HalfThickness) * Sin;
            const auto OuterX = (InRadius + HalfThickness) * Cos;
            const auto OuterY = (InRadius + HalfThickness) * Sin;

            Vertices.Add(FVector(OuterX, OuterY, 0.0f));
            Normals.Add(FVector::DownVector);
            UVs.Add(FVector2D(T, 1.0f));

            Vertices.Add(FVector(InnerX, InnerY, 0.0f));
            Normals.Add(FVector::DownVector);
            UVs.Add(FVector2D(T, 0.0f));
        }

        for (auto i = 0; i < SegmentsToUse; ++i)
        {
            const auto Current = BottomStartIdx + i * 2;
            const auto Next = BottomStartIdx + (i + 1) * 2;

            Triangles.Add(Current);
            Triangles.Add(Next);
            Triangles.Add(Current + 1);

            Triangles.Add(Current + 1);
            Triangles.Add(Next);
            Triangles.Add(Next + 1);
        }

        ApplyAxisRotation(Vertices, Normals, InAxis);

        InMeshComponent->CreateMeshSection_LinearColor(
            0, Vertices, Triangles, Normals, UVs,
            TArray<FLinearColor>{}, TArray<FProcMeshTangent>{}, true);
    }

    auto
        GenerateDebugShape_Torus(
            UProceduralMeshComponent* InMeshComponent,
            float InMajorRadius,
            float InMinorRadius,
            int32 InMajorSegments,
            int32 InMinorSegments,
            ECk_Plane_Axis InAxis)
        -> void
    {
        auto Vertices = TArray<FVector>{};
        auto Triangles = TArray<int32>{};
        auto Normals = TArray<FVector>{};
        auto UVs = TArray<FVector2D>{};

        for (auto i = 0; i <= InMajorSegments; ++i)
        {
            const auto MajorAngle = 2.0f * PI * i / InMajorSegments;
            const auto MajorCos = FMath::Cos(MajorAngle);
            const auto MajorSin = FMath::Sin(MajorAngle);

            const auto TubeCenter = FVector(
                InMajorRadius * MajorCos,
                InMajorRadius * MajorSin,
                0.0f);

            for (auto j = 0; j <= InMinorSegments; ++j)
            {
                const auto MinorAngle = 2.0f * PI * j / InMinorSegments;
                const auto MinorCos = FMath::Cos(MinorAngle);
                const auto MinorSin = FMath::Sin(MinorAngle);

                const auto Vertex = TubeCenter + FVector(
                    InMinorRadius * MinorCos * MajorCos,
                    InMinorRadius * MinorCos * MajorSin,
                    InMinorRadius * MinorSin);

                Vertices.Add(Vertex);

                const auto Normal = FVector(
                    MinorCos * MajorCos,
                    MinorCos * MajorSin,
                    MinorSin);
                Normals.Add(Normal.GetSafeNormal());

                UVs.Add(FVector2D(
                    static_cast<float>(i) / InMajorSegments,
                    static_cast<float>(j) / InMinorSegments));
            }
        }

        for (auto i = 0; i < InMajorSegments; ++i)
        {
            for (auto j = 0; j < InMinorSegments; ++j)
            {
                const auto Current = i * (InMinorSegments + 1) + j;
                const auto Next = (i + 1) * (InMinorSegments + 1) + j;

                Triangles.Add(Current);
                Triangles.Add(Current + 1);
                Triangles.Add(Next);

                Triangles.Add(Current + 1);
                Triangles.Add(Next + 1);
                Triangles.Add(Next);
            }
        }

        ApplyAxisRotation(Vertices, Normals, InAxis);

        InMeshComponent->CreateMeshSection_LinearColor(
            0, Vertices, Triangles, Normals, UVs,
            TArray<FLinearColor>{}, TArray<FProcMeshTangent>{}, true);
    }

    auto
        GenerateDebugShape_Cross(
            UProceduralMeshComponent* InMeshComponent,
            float InSize,
            float InThickness,
            ECk_Plane_Axis InAxis)
        -> void
    {
        auto Vertices = TArray<FVector>{};
        auto Triangles = TArray<int32>{};
        auto Normals = TArray<FVector>{};
        auto UVs = TArray<FVector2D>{};

        const auto HalfThickness = InThickness * 0.5f;

        const auto HorizTL = FVector(0.0f, -InSize, HalfThickness);
        const auto HorizTR = FVector(0.0f,  InSize, HalfThickness);
        const auto HorizBR = FVector(0.0f,  InSize, -HalfThickness);
        const auto HorizBL = FVector(0.0f, -InSize, -HalfThickness);

        const auto VertTL = FVector(0.0f, -HalfThickness,  InSize);
        const auto VertTR = FVector(0.0f,  HalfThickness,  InSize);
        const auto VertBR = FVector(0.0f,  HalfThickness, -InSize);
        const auto VertBL = FVector(0.0f, -HalfThickness, -InSize);

        auto BaseIdx = Vertices.Num();
        Vertices.Add(HorizTL); Vertices.Add(HorizTR); Vertices.Add(HorizBR); Vertices.Add(HorizBL);
        for (auto i = 0; i < 4; ++i) { Normals.Add(FVector::ForwardVector); }
        UVs.Add(FVector2D(0, 0)); UVs.Add(FVector2D(1, 0)); UVs.Add(FVector2D(1, 1)); UVs.Add(FVector2D(0, 1));

        Triangles.Add(BaseIdx + 0); Triangles.Add(BaseIdx + 1); Triangles.Add(BaseIdx + 2);
        Triangles.Add(BaseIdx + 0); Triangles.Add(BaseIdx + 2); Triangles.Add(BaseIdx + 3);

        BaseIdx = Vertices.Num();
        Vertices.Add(HorizTL); Vertices.Add(HorizTR); Vertices.Add(HorizBR); Vertices.Add(HorizBL);
        for (auto i = 0; i < 4; ++i) { Normals.Add(FVector::BackwardVector); }
        UVs.Add(FVector2D(0, 0)); UVs.Add(FVector2D(1, 0)); UVs.Add(FVector2D(1, 1)); UVs.Add(FVector2D(0, 1));

        Triangles.Add(BaseIdx + 0); Triangles.Add(BaseIdx + 2); Triangles.Add(BaseIdx + 1);
        Triangles.Add(BaseIdx + 0); Triangles.Add(BaseIdx + 3); Triangles.Add(BaseIdx + 2);

        BaseIdx = Vertices.Num();
        Vertices.Add(VertTL); Vertices.Add(VertTR); Vertices.Add(VertBR); Vertices.Add(VertBL);
        for (auto i = 0; i < 4; ++i) { Normals.Add(FVector::ForwardVector); }
        UVs.Add(FVector2D(0, 0)); UVs.Add(FVector2D(1, 0)); UVs.Add(FVector2D(1, 1)); UVs.Add(FVector2D(0, 1));

        Triangles.Add(BaseIdx + 0); Triangles.Add(BaseIdx + 1); Triangles.Add(BaseIdx + 2);
        Triangles.Add(BaseIdx + 0); Triangles.Add(BaseIdx + 2); Triangles.Add(BaseIdx + 3);

        BaseIdx = Vertices.Num();
        Vertices.Add(VertTL); Vertices.Add(VertTR); Vertices.Add(VertBR); Vertices.Add(VertBL);
        for (auto i = 0; i < 4; ++i) { Normals.Add(FVector::BackwardVector); }
        UVs.Add(FVector2D(0, 0)); UVs.Add(FVector2D(1, 0)); UVs.Add(FVector2D(1, 1)); UVs.Add(FVector2D(0, 1));

        Triangles.Add(BaseIdx + 0); Triangles.Add(BaseIdx + 2); Triangles.Add(BaseIdx + 1);
        Triangles.Add(BaseIdx + 0); Triangles.Add(BaseIdx + 3); Triangles.Add(BaseIdx + 2);

        ApplyAxisRotation(Vertices, Normals, InAxis);

        InMeshComponent->CreateMeshSection_LinearColor(
            0, Vertices, Triangles, Normals, UVs,
            TArray<FLinearColor>{}, TArray<FProcMeshTangent>{}, true);
    }

    auto
        GenerateDebugShape_WedgeCone(
            UProceduralMeshComponent* InMeshComponent,
            float InRadius,
            float InHeight,
            float InStartAngle,
            float InEndAngle,
            int32 InSegments,
            ECk_Plane_Axis InAxis)
        -> void
    {
        auto Vertices = TArray<FVector>{};
        auto Triangles = TArray<int32>{};
        auto Normals = TArray<FVector>{};
        auto UVs = TArray<FVector2D>{};

        const auto StartRad = FMath::DegreesToRadians(InStartAngle);
        const auto EndRad = FMath::DegreesToRadians(InEndAngle);
        auto AngleRange = EndRad - StartRad;

        while (AngleRange < 0.0f) { AngleRange += 2.0f * PI; }
        while (AngleRange > 2.0f * PI) { AngleRange -= 2.0f * PI; }

        const auto SegmentsToUse = FMath::Max(3, static_cast<int32>(InSegments * (AngleRange / (2.0f * PI))));

        const auto Apex = FVector(0, 0, InHeight);

        Vertices.Add(Apex);
        Normals.Add(FVector::UpVector);
        UVs.Add(FVector2D(0.5f, 1.0f));

        for (auto i = 0; i <= SegmentsToUse; ++i)
        {
            const auto T = static_cast<float>(i) / SegmentsToUse;
            const auto Angle = StartRad + AngleRange * T;
            const auto X = InRadius * FMath::Cos(Angle);
            const auto Y = InRadius * FMath::Sin(Angle);

            Vertices.Add(FVector(X, Y, 0.0f));

            const auto EdgeDir = FVector(X, Y, 0.0f) - Apex;
            const auto Tangent = FVector(-Y, X, 0.0f).GetSafeNormal();
            Normals.Add(FVector::CrossProduct(EdgeDir, Tangent).GetSafeNormal());

            UVs.Add(FVector2D(T, 0.0f));
        }

        for (auto i = 0; i < SegmentsToUse; ++i)
        {
            Triangles.Add(0);
            Triangles.Add(i + 2);
            Triangles.Add(i + 1);
        }

        const auto BaseCenterIdx = Vertices.Num();
        Vertices.Add(FVector::ZeroVector);
        Normals.Add(FVector::DownVector);
        UVs.Add(FVector2D(0.5f, 0.5f));

        for (auto i = 0; i <= SegmentsToUse; ++i)
        {
            const auto T = static_cast<float>(i) / SegmentsToUse;
            const auto Angle = StartRad + AngleRange * T;
            const auto X = InRadius * FMath::Cos(Angle);
            const auto Y = InRadius * FMath::Sin(Angle);

            Vertices.Add(FVector(X, Y, 0.0f));
            Normals.Add(FVector::DownVector);
            UVs.Add(FVector2D(
                0.5f + 0.5f * FMath::Cos(Angle),
                0.5f + 0.5f * FMath::Sin(Angle)));
        }

        for (auto i = 0; i < SegmentsToUse; ++i)
        {
            Triangles.Add(BaseCenterIdx);
            Triangles.Add(BaseCenterIdx + i + 1);
            Triangles.Add(BaseCenterIdx + i + 2);
        }

        const auto StartSideIdx = Vertices.Num();
        const auto StartX = InRadius * FMath::Cos(StartRad);
        const auto StartY = InRadius * FMath::Sin(StartRad);
        const auto StartNormal = FVector::CrossProduct(
            FVector(StartX, StartY, 0.0f) - Apex,
            FVector(0, 0, -1)).GetSafeNormal();

        Vertices.Add(FVector::ZeroVector);
        Vertices.Add(FVector(StartX, StartY, 0.0f));
        Vertices.Add(Apex);
        Normals.Add(StartNormal);
        Normals.Add(StartNormal);
        Normals.Add(StartNormal);
        UVs.Add(FVector2D(0, 0));
        UVs.Add(FVector2D(1, 0));
        UVs.Add(FVector2D(0.5f, 1));

        Triangles.Add(StartSideIdx);
        Triangles.Add(StartSideIdx + 2);
        Triangles.Add(StartSideIdx + 1);

        const auto EndSideIdx = Vertices.Num();
        const auto EndX = InRadius * FMath::Cos(EndRad);
        const auto EndY = InRadius * FMath::Sin(EndRad);
        const auto EndNormal = FVector::CrossProduct(
            FVector(0, 0, -1),
            FVector(EndX, EndY, 0.0f) - Apex).GetSafeNormal();

        Vertices.Add(FVector::ZeroVector);
        Vertices.Add(Apex);
        Vertices.Add(FVector(EndX, EndY, 0.0f));
        Normals.Add(EndNormal);
        Normals.Add(EndNormal);
        Normals.Add(EndNormal);
        UVs.Add(FVector2D(0, 0));
        UVs.Add(FVector2D(0.5f, 1));
        UVs.Add(FVector2D(1, 0));

        Triangles.Add(EndSideIdx);
        Triangles.Add(EndSideIdx + 2);
        Triangles.Add(EndSideIdx + 1);

        ApplyAxisRotation(Vertices, Normals, InAxis);

        InMeshComponent->CreateMeshSection_LinearColor(
            0, Vertices, Triangles, Normals, UVs,
            TArray<FLinearColor>{}, TArray<FProcMeshTangent>{}, true);
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_Pmg_DebugShape_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Pmg_DebugShape_Params& InParams,
            FFragment_Pmg_DebugShape_Current& InCurrent)
            -> void
    {
        InHandle.Remove<MarkedDirtyBy>();

        ck::pmg::Verbose(TEXT("Setting up Pmg DebugShape [{}]"), InHandle);

        const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
        CK_ENSURE_IF_NOT(ck::IsValid(World),
            TEXT("Pmg DebugShape [{}] could not get valid World"), InHandle)
        { return; }

        auto MeshComponent = NewObject<UProceduralMeshComponent>(
            World,
            UProceduralMeshComponent::StaticClass(),
            NAME_None,
            RF_Transient);

        CK_ENSURE_IF_NOT(ck::IsValid(MeshComponent),
            TEXT("Pmg DebugShape [{}] failed to create ProceduralMeshComponent"), InHandle)
        { return; }

        MeshComponent->SetWorldLocation(FVector::ZeroVector);
        MeshComponent->RegisterComponentWithWorld(World);

        if (MeshComponent->HasBegunPlay() == false)
        {
            MeshComponent->BeginPlay();
        }

        MeshComponent->SetVisibility(true);
        MeshComponent->SetHiddenInGame(false);
        MeshComponent->SetCastShadow(false);

        const auto& Params = InParams.Get_Params();
        const auto Axis = Params.Get_DefaultAxis();

        switch (Params.Get_ShapeType())
        {
            case ECk_Pmg_DebugShape_Type::Sphere:
            {
                GenerateDebugShape_Sphere(MeshComponent, Params.Get_Size(),
                    Params.Get_Segments(), Params.Get_Rings(), Axis);
                break;
            }
            case ECk_Pmg_DebugShape_Type::Box:
            {
                const auto Extent = FVector(Params.Get_Size(), Params.Get_SecondarySize(), Params.Get_TertiarySize());
                GenerateDebugShape_Box(MeshComponent, Extent, Axis);
                break;
            }
            case ECk_Pmg_DebugShape_Type::Circle:
            {
                GenerateDebugShape_Circle(MeshComponent, Params.Get_Size(), Params.Get_Segments(), Params.Get_DrawDirectionLine(), Axis);
                break;
            }
            case ECk_Pmg_DebugShape_Type::Cone:
            {
                GenerateDebugShape_Cone(MeshComponent, Params.Get_Size(),
                    Params.Get_SecondarySize(), Params.Get_Segments(), Axis);
                break;
            }
            case ECk_Pmg_DebugShape_Type::Cylinder:
            {
                GenerateDebugShape_Cylinder(MeshComponent, Params.Get_Size(),
                    Params.Get_SecondarySize(), Params.Get_Segments(), Axis);
                break;
            }
            case ECk_Pmg_DebugShape_Type::Capsule:
            {
                GenerateDebugShape_Capsule(MeshComponent, Params.Get_Size(),
                    Params.Get_SecondarySize(), Params.Get_Segments(), Params.Get_Rings(), Axis);
                break;
            }
            case ECk_Pmg_DebugShape_Type::Arrow:
            {
                GenerateDebugShape_Arrow(
                    MeshComponent,
                    Params.Get_Size(),
                    Params.Get_SecondarySize(),
                    Params.Get_ArrowHeadRatio(),
                    Params.Get_ArrowHeadWidthMultiplier(),
                    Axis);
                break;
            }
            case ECk_Pmg_DebugShape_Type::Ring:
            {
                GenerateDebugShape_Ring(
                    MeshComponent,
                    Params.Get_InnerRadius(),
                    Params.Get_Size(),
                    Params.Get_Segments(),
                    Axis);
                break;
            }
            case ECk_Pmg_DebugShape_Type::Wedge:
            {
                GenerateDebugShape_Wedge(
                    MeshComponent,
                    Params.Get_Size(),
                    Params.Get_StartAngle(),
                    Params.Get_EndAngle(),
                    Params.Get_Segments(),
                    Axis);
                break;
            }
            case ECk_Pmg_DebugShape_Type::Frustum:
            {
                GenerateDebugShape_Frustum(
                    MeshComponent,
                    Params.Get_NearWidth(),
                    Params.Get_NearHeight(),
                    Params.Get_FarWidth(),
                    Params.Get_FarHeight(),
                    Params.Get_Size(),
                    Axis);
                break;
            }
            case ECk_Pmg_DebugShape_Type::Arc:
            {
                GenerateDebugShape_Arc(
                    MeshComponent,
                    Params.Get_Size(),
                    Params.Get_StartAngle(),
                    Params.Get_EndAngle(),
                    Params.Get_SecondarySize(),
                    Params.Get_Segments(),
                    Axis);
                break;
            }
            case ECk_Pmg_DebugShape_Type::Torus:
            {
                GenerateDebugShape_Torus(
                    MeshComponent,
                    Params.Get_Size(),
                    Params.Get_SecondarySize(),
                    Params.Get_Segments(),
                    Params.Get_Rings(),
                    Axis);
                break;
            }
            case ECk_Pmg_DebugShape_Type::Cross:
            {
                GenerateDebugShape_Cross(
                    MeshComponent,
                    Params.Get_Size(),
                    Params.Get_LineThickness(),
                    Axis);
                break;
            }
            case ECk_Pmg_DebugShape_Type::WedgeCone:
            {
                GenerateDebugShape_WedgeCone(
                    MeshComponent,
                    Params.Get_Size(),
                    Params.Get_SecondarySize(),
                    Params.Get_StartAngle(),
                    Params.Get_EndAngle(),
                    Params.Get_Segments(),
                    InParams.Get_Params().Get_DefaultAxis());
                break;
            }
            case ECk_Pmg_DebugShape_Type::Star:
            {
                GenerateDebugShape_Star(
                    MeshComponent,
                    Params.Get_Size(),
                    Params.Get_Points(),
                    Params.Get_InnerRadiusRatio(),
                    InParams.Get_Params().Get_DefaultAxis());
                break;
            }
            case ECk_Pmg_DebugShape_Type::Plane:
            {
                GenerateDebugShape_Plane(
                    MeshComponent,
                    Params.Get_Size(),
                    Params.Get_SecondarySize(),
                    InParams.Get_Params().Get_DefaultAxis());
                break;
            }
            case ECk_Pmg_DebugShape_Type::Pyramid:
            {
                GenerateDebugShape_Pyramid(
                    MeshComponent,
                    Params.Get_Size(),
                    Params.Get_SecondarySize(),
                    InParams.Get_Params().Get_DefaultAxis());
                break;
            }
            case ECk_Pmg_DebugShape_Type::Hemisphere:
            {
                GenerateDebugShape_Hemisphere(
                    MeshComponent,
                    Params.Get_Size(),
                    Params.Get_Segments(),
                    Params.Get_Rings(),
                    InParams.Get_Params().Get_DefaultAxis());
                break;
            }
            case ECk_Pmg_DebugShape_Type::DashedLine:
            {
                GenerateDebugShape_DashedLine(
                    MeshComponent,
                    Params.Get_Size(),
                    Params.Get_SecondarySize(),
                    Params.Get_DashLength(),
                    Params.Get_GapLength(),
                    InParams.Get_Params().Get_DefaultAxis());
                break;
            }
            case ECk_Pmg_DebugShape_Type::Checkmark:
            {
                GenerateDebugShape_Checkmark(
                    MeshComponent,
                    Params.Get_Size(),
                    Params.Get_SecondarySize(),
                    InParams.Get_Params().Get_DefaultAxis());
                break;
            }
            case ECk_Pmg_DebugShape_Type::Diamond:
            {
                GenerateDebugShape_Diamond(
                    MeshComponent,
                    Params.Get_Size(),
                    Params.Get_SecondarySize(),
                    InParams.Get_Params().Get_DefaultAxis());
                break;
            }
            case ECk_Pmg_DebugShape_Type::Pivot:
            {
                GenerateDebugShape_Pivot(
                    MeshComponent,
                    Params.Get_Size(),
                    InParams.Get_Params().Get_DefaultAxis());
                break;
            }
            case ECk_Pmg_DebugShape_Type::Warning:
            {
                ck::pmg::GenerateDebugShape_Warning(MeshComponent, Params.Get_Size());
                break;
            }
            case ECk_Pmg_DebugShape_Type::Prohibition:
            {
                ck::pmg::GenerateDebugShape_Prohibition(
                    MeshComponent,
                    Params.Get_Size(),
                    Params.Get_Segments());
                break;
            }
            case ECk_Pmg_DebugShape_Type::NoEntry:
            {
                ck::pmg::GenerateDebugShape_NoEntry(
                    MeshComponent,
                    Params.Get_Size(),
                    Params.Get_Segments());
                break;
            }
            case ECk_Pmg_DebugShape_Type::MagnifyingGlass:
            {
                ck::pmg::GenerateDebugShape_MagnifyingGlass(MeshComponent, Params.Get_Size());
                break;
            }
            case ECk_Pmg_DebugShape_Type::QuestionMark:
            {
                ck::pmg::GenerateDebugShape_QuestionMark(MeshComponent, Params.Get_Size());
                break;
            }
            case ECk_Pmg_DebugShape_Type::ExclamationMark:
            {
                ck::pmg::GenerateDebugShape_ExclamationMark(MeshComponent, Params.Get_Size());
                break;
            }
            case ECk_Pmg_DebugShape_Type::Flag:
            {
                ck::pmg::GenerateDebugShape_Flag(MeshComponent, Params.Get_Size());
                break;
            }
            case ECk_Pmg_DebugShape_Type::InfoCircle:
            {
                ck::pmg::GenerateDebugShape_InfoCircle(
                    MeshComponent,
                    Params.Get_Size(),
                    Params.Get_Segments());
                break;
            }
            case ECk_Pmg_DebugShape_Type::Pin:
            {
                ck::pmg::GenerateDebugShape_Pin(MeshComponent, Params.Get_Size());
                break;
            }
            default:
            {
                CK_INVALID_ENUM(Params.Get_ShapeType());
                return;
            }
        }

        const auto ShouldBeVisible = Params.Get_RenderMode() != ECk_Pmg_RenderMode::Hidden;
        MeshComponent->SetVisibility(ShouldBeVisible, true);
        MeshComponent->SetHiddenInGame(!ShouldBeVisible);
        MeshComponent->bHiddenInGame = !ShouldBeVisible;
        MeshComponent->SetRenderInMainPass(true);
        MeshComponent->SetRenderInDepthPass(true);

        static auto TranslucentMaterial = LoadObject<UMaterial>(
            nullptr,
            TEXT("/Engine/EngineDebugMaterials/M_SimpleUnlitTranslucent.M_SimpleUnlitTranslucent"));

        if (ck::IsValid(TranslucentMaterial))
        {
            auto DynamicMaterial = UMaterialInstanceDynamic::Create(TranslucentMaterial, MeshComponent);
            if (ck::IsValid(DynamicMaterial))
            {
                DynamicMaterial->SetVectorParameterValue(FName("Color"), Params.Get_Color());
                MeshComponent->SetMaterial(0, DynamicMaterial);
            }
        }
        else
        {
            ck::pmg::Warning(TEXT("Failed to load M_SimpleUnlitTranslucent for Pmg DebugShape [{}]"), InHandle);
        }

        MeshComponent->SetCollisionEnabled(
            Params.Get_EnableCollision() ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);

        MeshComponent->UpdateBounds();
        MeshComponent->MarkRenderStateDirty();

        InCurrent._MeshComponent = TStrongObjectPtr{MeshComponent};
        InCurrent._SpawnTime = UCk_Utils_Time_UE::Get_WorldTime(FCk_Utils_Time_GetWorldTime_Params(UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle))).Get_WorldTime().Get_Time();

        if (InHandle.Has<ck::FFragment_Transform>())
        {
            const auto& Transform = InHandle.Get<ck::FFragment_Transform>();
            auto FinalTransform = Transform.Get_Transform();

            // Apply axis rotation for all shapes
            const auto AxisRotation = GetAxisRotation(Params.Get_DefaultAxis());
            FinalTransform.SetRotation(FinalTransform.GetRotation() * AxisRotation);

            MeshComponent->SetWorldTransform(FinalTransform);
        }

        ck::pmg::Verbose(TEXT("Pmg DebugShape [{}] setup complete"), InHandle);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Pmg_DebugShape_UpdateTransform::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Pmg_DebugShape_Current& InCurrent,
            const FFragment_Transform& InTransform)
            -> void
    {
        auto MeshComponent = InCurrent._MeshComponent.Get();
        CK_ENSURE_IF_NOT(ck::IsValid(MeshComponent),
            TEXT("Pmg DebugShape [{}] has invalid mesh component"), InHandle)
        { return; }

        MeshComponent->SetWorldTransform(InTransform.Get_Transform());
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Pmg_DebugShape_CheckDuration::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Pmg_DebugShape_Params& InParams,
            FFragment_Pmg_DebugShape_Current& InCurrent)
            -> void
    {
        const auto& Duration = InParams.Get_Params().Get_Duration();

        if (Duration.Get_Seconds() < 0.0f)
        { return; }

        auto Now = UCk_Utils_Time_UE::Get_WorldTime(FCk_Utils_Time_GetWorldTime_Params(UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle))).Get_WorldTime().Get_Time();
        const auto ElapsedTime = Now - InCurrent._SpawnTime;

        if (ElapsedTime.Get_Seconds() >= Duration.Get_Seconds())
        {
            UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InHandle);
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Pmg_DebugShape_DrawLines::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Pmg_DebugShape_Params& InParams,
            const FFragment_Pmg_DebugShape_Current& InCurrent,
            const FFragment_Transform& InTransform)
        -> void
    {
        const auto& Params = InParams.Get_Params();

        if (Params.Get_DrawLines() == false)
        { return; }

        const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
        if (ck::IsValid(World) == false)
        { return; }

        const auto& Transform = InTransform.Get_Transform();
        const auto Center = Transform.GetLocation();
        const auto Rotation = Transform.GetRotation().Rotator();

        // Line color = RGB of shape color with full alpha
        const auto& ShapeColor = Params.Get_Color();
        const auto LineColor = FLinearColor(ShapeColor.R, ShapeColor.G, ShapeColor.B, 1.0f);
        const auto Thickness = Params.Get_LineThickness();

        switch (Params.Get_ShapeType())
        {
            case ECk_Pmg_DebugShape_Type::Sphere:
            {
                const auto Radius = Params.Get_Size();
                const auto Rot = Transform.GetRotation();
                const auto ArcSegments = 32;

                // Draw 3 cross-sectional circles (XZ, YZ, and XY planes)

                // XZ plane circle
                for (auto j = 0; j < ArcSegments; ++j)
                {
                    const auto Phi1 = 2.0f * PI * j / ArcSegments;
                    const auto Phi2 = 2.0f * PI * (j + 1) / ArcSegments;

                    const auto P1 = Center + Rot.RotateVector(FVector(
                        Radius * FMath::Cos(Phi1),
                        0.0f,
                        Radius * FMath::Sin(Phi1)));
                    const auto P2 = Center + Rot.RotateVector(FVector(
                        Radius * FMath::Cos(Phi2),
                        0.0f,
                        Radius * FMath::Sin(Phi2)));

                    UCk_Utils_DebugDraw_UE::DrawDebugLine(World, P1, P2, LineColor, 0.0f, Thickness);
                }

                // YZ plane circle
                for (auto j = 0; j < ArcSegments; ++j)
                {
                    const auto Phi1 = 2.0f * PI * j / ArcSegments;
                    const auto Phi2 = 2.0f * PI * (j + 1) / ArcSegments;

                    const auto P1 = Center + Rot.RotateVector(FVector(
                        0.0f,
                        Radius * FMath::Cos(Phi1),
                        Radius * FMath::Sin(Phi1)));
                    const auto P2 = Center + Rot.RotateVector(FVector(
                        0.0f,
                        Radius * FMath::Cos(Phi2),
                        Radius * FMath::Sin(Phi2)));

                    UCk_Utils_DebugDraw_UE::DrawDebugLine(World, P1, P2, LineColor, 0.0f, Thickness);
                }

                // XY plane circle
                for (auto j = 0; j < ArcSegments; ++j)
                {
                    const auto Phi1 = 2.0f * PI * j / ArcSegments;
                    const auto Phi2 = 2.0f * PI * (j + 1) / ArcSegments;

                    const auto P1 = Center + Rot.RotateVector(FVector(
                        Radius * FMath::Cos(Phi1),
                        Radius * FMath::Sin(Phi1),
                        0.0f));
                    const auto P2 = Center + Rot.RotateVector(FVector(
                        Radius * FMath::Cos(Phi2),
                        Radius * FMath::Sin(Phi2),
                        0.0f));

                    UCk_Utils_DebugDraw_UE::DrawDebugLine(World, P1, P2, LineColor, 0.0f, Thickness);
                }
                break;
            }
            case ECk_Pmg_DebugShape_Type::Box:
            {
                const auto Extent = FVector(Params.Get_Size(), Params.Get_SecondarySize(), Params.Get_TertiarySize());
                UCk_Utils_DebugDraw_UE::DrawDebugBox(
                    World, Center, Extent, LineColor, Rotation, 0.0f, Thickness);
                break;
            }
            case ECk_Pmg_DebugShape_Type::Circle:
            {
                // Draw circle with proper rotation from transform
                const auto Radius = Params.Get_Size();
                const auto Segments = Params.Get_Segments();
                const auto Rot = Transform.GetRotation();

                // Generate circle points and apply rotation
                for (auto i = 0; i < Segments; ++i)
                {
                    const auto Angle1 = 2.0f * PI * i / Segments;
                    const auto Angle2 = 2.0f * PI * (i + 1) / Segments;

                    const auto LocalP1 = FVector(
                        Radius * FMath::Cos(Angle1),
                        Radius * FMath::Sin(Angle1),
                        0.0f);
                    const auto LocalP2 = FVector(
                        Radius * FMath::Cos(Angle2),
                        Radius * FMath::Sin(Angle2),
                        0.0f);

                    const auto WorldP1 = Center + Rot.RotateVector(LocalP1);
                    const auto WorldP2 = Center + Rot.RotateVector(LocalP2);

                    UCk_Utils_DebugDraw_UE::DrawDebugLine(World, WorldP1, WorldP2, LineColor, 0.0f, Thickness);
                }

                // Draw direction line from center to forward (+X)
                if (Params.Get_DrawDirectionLine())
                {
                    const auto ForwardEnd = Center + Rot.RotateVector(FVector(Radius, 0.0f, 0.0f));
                    UCk_Utils_DebugDraw_UE::DrawDebugLine(World, Center, ForwardEnd, LineColor, 0.0f, Thickness);
                }
                break;
            }
            case ECk_Pmg_DebugShape_Type::Cone:
            {
                // Draw cone outline: base circle + lines to apex
                const auto Radius = Params.Get_Size();
                const auto Height = Params.Get_SecondarySize();
                const auto Segments = Params.Get_Segments();
                const auto Rot = Transform.GetRotation();
                const auto Apex = Center + Rot.RotateVector(FVector(0, 0, Height));

                // Draw base circle with proper rotation (manually, not using DrawDebugCircle)
                for (auto i = 0; i < Segments; ++i)
                {
                    const auto Angle1 = 2.0f * PI * i / Segments;
                    const auto Angle2 = 2.0f * PI * (i + 1) / Segments;

                    const auto P1 = Center + Rot.RotateVector(FVector(
                        Radius * FMath::Cos(Angle1),
                        Radius * FMath::Sin(Angle1),
                        0.0f));
                    const auto P2 = Center + Rot.RotateVector(FVector(
                        Radius * FMath::Cos(Angle2),
                        Radius * FMath::Sin(Angle2),
                        0.0f));

                    UCk_Utils_DebugDraw_UE::DrawDebugLine(World, P1, P2, LineColor, 0.0f, Thickness);
                }

                // Draw lines from base to apex
                for (auto i = 0; i < Segments; i += FMath::Max(1, Segments / 4))
                {
                    const auto Angle = 2.0f * PI * i / Segments;
                    const auto BasePoint = Center + Rot.RotateVector(
                        FVector(Radius * FMath::Cos(Angle), Radius * FMath::Sin(Angle), 0.0f));
                    UCk_Utils_DebugDraw_UE::DrawDebugLine(World, BasePoint, Apex, LineColor, 0.0f, Thickness);
                }
                break;
            }
            case ECk_Pmg_DebugShape_Type::Cylinder:
            {
                const auto Radius = Params.Get_Size();
                const auto HalfHeight = Params.Get_SecondarySize() * 0.5f;
                const auto Segments = Params.Get_Segments();
                const auto Rot = Transform.GetRotation();

                const auto TopCenter = Center + Rot.RotateVector(FVector(0, 0, HalfHeight));
                const auto BottomCenter = Center + Rot.RotateVector(FVector(0, 0, -HalfHeight));

                // Draw top and bottom circles with proper rotation
                for (auto i = 0; i < Segments; ++i)
                {
                    const auto Angle1 = 2.0f * PI * i / Segments;
                    const auto Angle2 = 2.0f * PI * (i + 1) / Segments;

                    // Top circle
                    const auto TopP1 = Center + Rot.RotateVector(FVector(
                        Radius * FMath::Cos(Angle1),
                        Radius * FMath::Sin(Angle1),
                        HalfHeight));
                    const auto TopP2 = Center + Rot.RotateVector(FVector(
                        Radius * FMath::Cos(Angle2),
                        Radius * FMath::Sin(Angle2),
                        HalfHeight));
                    UCk_Utils_DebugDraw_UE::DrawDebugLine(World, TopP1, TopP2, LineColor, 0.0f, Thickness);

                    // Bottom circle
                    const auto BottomP1 = Center + Rot.RotateVector(FVector(
                        Radius * FMath::Cos(Angle1),
                        Radius * FMath::Sin(Angle1),
                        -HalfHeight));
                    const auto BottomP2 = Center + Rot.RotateVector(FVector(
                        Radius * FMath::Cos(Angle2),
                        Radius * FMath::Sin(Angle2),
                        -HalfHeight));
                    UCk_Utils_DebugDraw_UE::DrawDebugLine(World, BottomP1, BottomP2, LineColor, 0.0f, Thickness);
                }

                // Draw vertical lines
                for (auto i = 0; i < Segments; i += FMath::Max(1, Segments / 4))
                {
                    const auto Angle = 2.0f * PI * i / Segments;
                    const auto Offset = Rot.RotateVector(
                        FVector(Radius * FMath::Cos(Angle), Radius * FMath::Sin(Angle), 0.0f));
                    UCk_Utils_DebugDraw_UE::DrawDebugLine(
                        World, TopCenter + Offset, BottomCenter + Offset, LineColor, 0.0f, Thickness);
                }
                break;
            }
            case ECk_Pmg_DebugShape_Type::Capsule:
            {
                UCk_Utils_DebugDraw_UE::DrawDebugCapsule(
                    World, Center, Params.Get_SecondarySize(), Params.Get_Size(),
                    Rotation, LineColor, 0.0f, Thickness);
                break;
            }
            case ECk_Pmg_DebugShape_Type::Arrow:
            {
                const auto Length = Params.Get_Size();
                const auto ShaftWidth = Params.Get_SecondarySize();
                const auto HeadRatio = Params.Get_ArrowHeadRatio();
                const auto HeadWidthMult = Params.Get_ArrowHeadWidthMultiplier();

                const auto HeadLength = Length * HeadRatio;
                const auto ShaftLength = Length - HeadLength;
                const auto HalfShaftWidth = ShaftWidth * 0.5f;
                const auto HeadWidth = ShaftWidth * HeadWidthMult * 0.5f;

                const auto Rot = Transform.GetRotation();

                const auto P1 = Center + Rot.RotateVector(FVector(0.0f, -HalfShaftWidth, 0.0f));
                const auto P2 = Center + Rot.RotateVector(FVector(0.0f, HalfShaftWidth, 0.0f));
                const auto P3 = Center + Rot.RotateVector(FVector(ShaftLength, HalfShaftWidth, 0.0f));
                const auto P4 = Center + Rot.RotateVector(FVector(ShaftLength, -HalfShaftWidth, 0.0f));

                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, P1, P2, LineColor, 0.0f, Thickness);
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, P2, P3, LineColor, 0.0f, Thickness);
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, P4, P1, LineColor, 0.0f, Thickness);

                const auto H1 = Center + Rot.RotateVector(FVector(ShaftLength, -HeadWidth, 0.0f));
                const auto H2 = Center + Rot.RotateVector(FVector(ShaftLength, HeadWidth, 0.0f));
                const auto Tip = Center + Rot.RotateVector(FVector(Length, 0.0f, 0.0f));

                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, P4, H1, LineColor, 0.0f, Thickness);
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, P3, H2, LineColor, 0.0f, Thickness);
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, H1, Tip, LineColor, 0.0f, Thickness);
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, H2, Tip, LineColor, 0.0f, Thickness);
                break;
            }
            case ECk_Pmg_DebugShape_Type::Ring:
            {
                const auto InnerRad = Params.Get_InnerRadius();
                const auto OuterRad = Params.Get_Size();
                const auto Segments = Params.Get_Segments();
                const auto Rot = Transform.GetRotation();

                for (auto i = 0; i < Segments; ++i)
                {
                    const auto Angle1 = 2.0f * PI * i / Segments;
                    const auto Angle2 = 2.0f * PI * (i + 1) / Segments;

                    const auto P1 = Center + Rot.RotateVector(FVector(
                        OuterRad * FMath::Cos(Angle1),
                        OuterRad * FMath::Sin(Angle1),
                        0.0f));
                    const auto P2 = Center + Rot.RotateVector(FVector(
                        OuterRad * FMath::Cos(Angle2),
                        OuterRad * FMath::Sin(Angle2),
                        0.0f));

                    UCk_Utils_DebugDraw_UE::DrawDebugLine(World, P1, P2, LineColor, 0.0f, Thickness);
                }

                for (auto i = 0; i < Segments; ++i)
                {
                    const auto Angle1 = 2.0f * PI * i / Segments;
                    const auto Angle2 = 2.0f * PI * (i + 1) / Segments;

                    const auto P1 = Center + Rot.RotateVector(FVector(
                        InnerRad * FMath::Cos(Angle1),
                        InnerRad * FMath::Sin(Angle1),
                        0.0f));
                    const auto P2 = Center + Rot.RotateVector(FVector(
                        InnerRad * FMath::Cos(Angle2),
                        InnerRad * FMath::Sin(Angle2),
                        0.0f));

                    UCk_Utils_DebugDraw_UE::DrawDebugLine(World, P1, P2, LineColor, 0.0f, Thickness);
                }
                break;
            }
            case ECk_Pmg_DebugShape_Type::Wedge:
            {
                const auto Radius = Params.Get_Size();
                const auto StartAngle = FMath::DegreesToRadians(Params.Get_StartAngle());
                const auto EndAngle = FMath::DegreesToRadians(Params.Get_EndAngle());
                auto AngleRange = EndAngle - StartAngle;

                while (AngleRange < 0.0f) { AngleRange += 2.0f * PI; }
                while (AngleRange > 2.0f * PI) { AngleRange -= 2.0f * PI; }

                const auto Segments = FMath::Max(3, static_cast<int32>(Params.Get_Segments() * (AngleRange / (2.0f * PI))));
                const auto Rot = Transform.GetRotation();

                const auto StartPoint = Center + Rot.RotateVector(FVector(
                    Radius * FMath::Cos(StartAngle),
                    Radius * FMath::Sin(StartAngle),
                    0.0f));
                const auto EndPoint = Center + Rot.RotateVector(FVector(
                    Radius * FMath::Cos(EndAngle),
                    Radius * FMath::Sin(EndAngle),
                    0.0f));

                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, Center, StartPoint, LineColor, 0.0f, Thickness);
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, Center, EndPoint, LineColor, 0.0f, Thickness);

                for (auto i = 0; i < Segments; ++i)
                {
                    const auto T1 = static_cast<float>(i) / Segments;
                    const auto T2 = static_cast<float>(i + 1) / Segments;
                    const auto Angle1 = StartAngle + AngleRange * T1;
                    const auto Angle2 = StartAngle + AngleRange * T2;

                    const auto P1 = Center + Rot.RotateVector(FVector(
                        Radius * FMath::Cos(Angle1),
                        Radius * FMath::Sin(Angle1),
                        0.0f));
                    const auto P2 = Center + Rot.RotateVector(FVector(
                        Radius * FMath::Cos(Angle2),
                        Radius * FMath::Sin(Angle2),
                        0.0f));

                    UCk_Utils_DebugDraw_UE::DrawDebugLine(World, P1, P2, LineColor, 0.0f, Thickness);
                }
                break;
            }
            case ECk_Pmg_DebugShape_Type::Frustum:
            {
                const auto Length = Params.Get_Size();
                const auto NearHW = Params.Get_NearWidth() * 0.5f;
                const auto NearHH = Params.Get_NearHeight() * 0.5f;
                const auto FarHW = Params.Get_FarWidth() * 0.5f;
                const auto FarHH = Params.Get_FarHeight() * 0.5f;

                const auto Rot = Transform.GetRotation();

                const auto NearTL = Center + Rot.RotateVector(FVector(0.0f, -NearHW,  NearHH));
                const auto NearTR = Center + Rot.RotateVector(FVector(0.0f,  NearHW,  NearHH));
                const auto NearBR = Center + Rot.RotateVector(FVector(0.0f,  NearHW, -NearHH));
                const auto NearBL = Center + Rot.RotateVector(FVector(0.0f, -NearHW, -NearHH));

                const auto FarTL = Center + Rot.RotateVector(FVector(Length, -FarHW,  FarHH));
                const auto FarTR = Center + Rot.RotateVector(FVector(Length,  FarHW,  FarHH));
                const auto FarBR = Center + Rot.RotateVector(FVector(Length,  FarHW, -FarHH));
                const auto FarBL = Center + Rot.RotateVector(FVector(Length, -FarHW, -FarHH));

                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, NearTL, NearTR, LineColor, 0.0f, Thickness);
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, NearTR, NearBR, LineColor, 0.0f, Thickness);
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, NearBR, NearBL, LineColor, 0.0f, Thickness);
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, NearBL, NearTL, LineColor, 0.0f, Thickness);

                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, FarTL, FarTR, LineColor, 0.0f, Thickness);
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, FarTR, FarBR, LineColor, 0.0f, Thickness);
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, FarBR, FarBL, LineColor, 0.0f, Thickness);
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, FarBL, FarTL, LineColor, 0.0f, Thickness);

                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, NearTL, FarTL, LineColor, 0.0f, Thickness);
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, NearTR, FarTR, LineColor, 0.0f, Thickness);
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, NearBR, FarBR, LineColor, 0.0f, Thickness);
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, NearBL, FarBL, LineColor, 0.0f, Thickness);
                break;
            }
            case ECk_Pmg_DebugShape_Type::Arc:
            {
                const auto Radius = Params.Get_Size();
                const auto StartAngle = FMath::DegreesToRadians(Params.Get_StartAngle());
                const auto EndAngle = FMath::DegreesToRadians(Params.Get_EndAngle());
                auto AngleRange = EndAngle - StartAngle;

                while (AngleRange < 0.0f) { AngleRange += 2.0f * PI; }
                while (AngleRange > 2.0f * PI) { AngleRange -= 2.0f * PI; }

                const auto Segments = FMath::Max(3, static_cast<int32>(Params.Get_Segments() * (AngleRange / (2.0f * PI))));
                const auto Rot = Transform.GetRotation();
                const auto ArcThickness = Params.Get_SecondarySize();
                const auto HalfThickness = ArcThickness * 0.5f;

                for (auto i = 0; i < Segments; ++i)
                {
                    const auto T1 = static_cast<float>(i) / Segments;
                    const auto T2 = static_cast<float>(i + 1) / Segments;
                    const auto Angle1 = StartAngle + AngleRange * T1;
                    const auto Angle2 = StartAngle + AngleRange * T2;

                    const auto Inner1 = Center + Rot.RotateVector(FVector(
                        (Radius - HalfThickness) * FMath::Cos(Angle1),
                        (Radius - HalfThickness) * FMath::Sin(Angle1),
                        0.0f));
                    const auto Inner2 = Center + Rot.RotateVector(FVector(
                        (Radius - HalfThickness) * FMath::Cos(Angle2),
                        (Radius - HalfThickness) * FMath::Sin(Angle2),
                        0.0f));

                    const auto Outer1 = Center + Rot.RotateVector(FVector(
                        (Radius + HalfThickness) * FMath::Cos(Angle1),
                        (Radius + HalfThickness) * FMath::Sin(Angle1),
                        0.0f));
                    const auto Outer2 = Center + Rot.RotateVector(FVector(
                        (Radius + HalfThickness) * FMath::Cos(Angle2),
                        (Radius + HalfThickness) * FMath::Sin(Angle2),
                        0.0f));

                    UCk_Utils_DebugDraw_UE::DrawDebugLine(World, Inner1, Inner2, LineColor, 0.0f, Thickness);
                    UCk_Utils_DebugDraw_UE::DrawDebugLine(World, Outer1, Outer2, LineColor, 0.0f, Thickness);
                }

                const auto StartInner = Center + Rot.RotateVector(FVector(
                    (Radius - HalfThickness) * FMath::Cos(StartAngle),
                    (Radius - HalfThickness) * FMath::Sin(StartAngle),
                    0.0f));
                const auto StartOuter = Center + Rot.RotateVector(FVector(
                    (Radius + HalfThickness) * FMath::Cos(StartAngle),
                    (Radius + HalfThickness) * FMath::Sin(StartAngle),
                    0.0f));
                const auto EndInner = Center + Rot.RotateVector(FVector(
                    (Radius - HalfThickness) * FMath::Cos(EndAngle),
                    (Radius - HalfThickness) * FMath::Sin(EndAngle),
                    0.0f));
                const auto EndOuter = Center + Rot.RotateVector(FVector(
                    (Radius + HalfThickness) * FMath::Cos(EndAngle),
                    (Radius + HalfThickness) * FMath::Sin(EndAngle),
                    0.0f));

                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, StartInner, StartOuter, LineColor, 0.0f, Thickness);
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, EndInner, EndOuter, LineColor, 0.0f, Thickness);
                break;
            }
            case ECk_Pmg_DebugShape_Type::Torus:
            {
                const auto MajorRadius = Params.Get_Size();
                const auto MinorRadius = Params.Get_SecondarySize();
                const auto MajorSegments = Params.Get_Segments();
                const auto MinorSegments = Params.Get_Rings();
                const auto Rot = Transform.GetRotation();

                const auto MajorCirclesToDraw = 8;
                for (auto CircleIdx = 0; CircleIdx < MajorCirclesToDraw; ++CircleIdx)
                {
                    const auto MajorAngle = 2.0f * PI * CircleIdx / MajorCirclesToDraw;
                    const auto MajorCos = FMath::Cos(MajorAngle);
                    const auto MajorSin = FMath::Sin(MajorAngle);

                    const auto TubeCenter = FVector(
                        MajorRadius * MajorCos,
                        MajorRadius * MajorSin,
                        0.0f);

                    for (auto j = 0; j < MinorSegments; ++j)
                    {
                        const auto MinorAngle1 = 2.0f * PI * j / MinorSegments;
                        const auto MinorAngle2 = 2.0f * PI * (j + 1) / MinorSegments;

                        const auto P1 = TubeCenter + FVector(
                            MinorRadius * FMath::Cos(MinorAngle1) * MajorCos,
                            MinorRadius * FMath::Cos(MinorAngle1) * MajorSin,
                            MinorRadius * FMath::Sin(MinorAngle1));

                        const auto P2 = TubeCenter + FVector(
                            MinorRadius * FMath::Cos(MinorAngle2) * MajorCos,
                            MinorRadius * FMath::Cos(MinorAngle2) * MajorSin,
                            MinorRadius * FMath::Sin(MinorAngle2));

                        const auto WorldP1 = Center + Rot.RotateVector(P1);
                        const auto WorldP2 = Center + Rot.RotateVector(P2);

                        UCk_Utils_DebugDraw_UE::DrawDebugLine(World, WorldP1, WorldP2, LineColor, 0.0f, Thickness);
                    }
                }

                const auto MinorCirclesToDraw = 4;
                for (auto CircleIdx = 0; CircleIdx < MinorCirclesToDraw; ++CircleIdx)
                {
                    const auto MinorAngle = 2.0f * PI * CircleIdx / MinorCirclesToDraw;
                    const auto MinorCos = FMath::Cos(MinorAngle);
                    const auto MinorSin = FMath::Sin(MinorAngle);

                    for (auto i = 0; i < MajorSegments; ++i)
                    {
                        const auto MajorAngle1 = 2.0f * PI * i / MajorSegments;
                        const auto MajorAngle2 = 2.0f * PI * (i + 1) / MajorSegments;

                        const auto P1 = FVector(
                            (MajorRadius + MinorRadius * MinorCos) * FMath::Cos(MajorAngle1),
                            (MajorRadius + MinorRadius * MinorCos) * FMath::Sin(MajorAngle1),
                            MinorRadius * MinorSin);

                        const auto P2 = FVector(
                            (MajorRadius + MinorRadius * MinorCos) * FMath::Cos(MajorAngle2),
                            (MajorRadius + MinorRadius * MinorCos) * FMath::Sin(MajorAngle2),
                            MinorRadius * MinorSin);

                        const auto WorldP1 = Center + Rot.RotateVector(P1);
                        const auto WorldP2 = Center + Rot.RotateVector(P2);

                        UCk_Utils_DebugDraw_UE::DrawDebugLine(World, WorldP1, WorldP2, LineColor, 0.0f, Thickness);
                    }
                }
                break;
            }
            case ECk_Pmg_DebugShape_Type::Cross:
            {
                const auto Size = Params.Get_Size();
                const auto ArmThickness = Params.Get_LineThickness();
                const auto HalfThickness = ArmThickness * 0.5f;
                const auto Rot = Transform.GetRotation();

                const auto HY1 = Center + Rot.RotateVector(FVector(0.0f, -Size, -HalfThickness));
                const auto HY2 = Center + Rot.RotateVector(FVector(0.0f,  Size, -HalfThickness));
                const auto HY3 = Center + Rot.RotateVector(FVector(0.0f,  Size,  HalfThickness));
                const auto HY4 = Center + Rot.RotateVector(FVector(0.0f, -Size,  HalfThickness));

                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, HY1, HY2, LineColor, 0.0f, Thickness);
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, HY2, HY3, LineColor, 0.0f, Thickness);
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, HY3, HY4, LineColor, 0.0f, Thickness);
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, HY4, HY1, LineColor, 0.0f, Thickness);

                const auto VZ1 = Center + Rot.RotateVector(FVector(0.0f, -HalfThickness, -Size));
                const auto VZ2 = Center + Rot.RotateVector(FVector(0.0f,  HalfThickness, -Size));
                const auto VZ3 = Center + Rot.RotateVector(FVector(0.0f,  HalfThickness,  Size));
                const auto VZ4 = Center + Rot.RotateVector(FVector(0.0f, -HalfThickness,  Size));

                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, VZ1, VZ2, LineColor, 0.0f, Thickness);
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, VZ2, VZ3, LineColor, 0.0f, Thickness);
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, VZ3, VZ4, LineColor, 0.0f, Thickness);
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, VZ4, VZ1, LineColor, 0.0f, Thickness);
                break;
            }
            case ECk_Pmg_DebugShape_Type::WedgeCone:
            {
                const auto Radius = Params.Get_Size();
                const auto Height = Params.Get_SecondarySize();
                const auto StartAngle = FMath::DegreesToRadians(Params.Get_StartAngle());
                const auto EndAngle = FMath::DegreesToRadians(Params.Get_EndAngle());
                auto AngleRange = EndAngle - StartAngle;

                while (AngleRange < 0.0f) { AngleRange += 2.0f * PI; }
                while (AngleRange > 2.0f * PI) { AngleRange -= 2.0f * PI; }

                const auto Segments = FMath::Max(3, static_cast<int32>(Params.Get_Segments() * (AngleRange / (2.0f * PI))));
                const auto Rot = Transform.GetRotation();

                const auto Apex = Center + Rot.RotateVector(FVector(0, 0, Height));

                for (auto i = 0; i < Segments; ++i)
                {
                    const auto T1 = static_cast<float>(i) / Segments;
                    const auto T2 = static_cast<float>(i + 1) / Segments;
                    const auto Angle1 = StartAngle + AngleRange * T1;
                    const auto Angle2 = StartAngle + AngleRange * T2;

                    const auto P1 = Center + Rot.RotateVector(FVector(
                        Radius * FMath::Cos(Angle1),
                        Radius * FMath::Sin(Angle1),
                        0.0f));
                    const auto P2 = Center + Rot.RotateVector(FVector(
                        Radius * FMath::Cos(Angle2),
                        Radius * FMath::Sin(Angle2),
                        0.0f));

                    UCk_Utils_DebugDraw_UE::DrawDebugLine(World, P1, P2, LineColor, 0.0f, Thickness);
                }

                const auto StartPoint = Center + Rot.RotateVector(FVector(
                    Radius * FMath::Cos(StartAngle),
                    Radius * FMath::Sin(StartAngle),
                    0.0f));
                const auto EndPoint = Center + Rot.RotateVector(FVector(
                    Radius * FMath::Cos(EndAngle),
                    Radius * FMath::Sin(EndAngle),
                    0.0f));

                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, Center, StartPoint, LineColor, 0.0f, Thickness);
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, Center, EndPoint, LineColor, 0.0f, Thickness);

                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, Center, Apex, LineColor, 0.0f, Thickness);
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, StartPoint, Apex, LineColor, 0.0f, Thickness);
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, EndPoint, Apex, LineColor, 0.0f, Thickness);

                const auto AdditionalLines = 3;
                for (auto i = 1; i <= AdditionalLines; ++i)
                {
                    const auto T = static_cast<float>(i) / (AdditionalLines + 1);
                    const auto Angle = StartAngle + AngleRange * T;
                    const auto ArcPoint = Center + Rot.RotateVector(FVector(
                        Radius * FMath::Cos(Angle),
                        Radius * FMath::Sin(Angle),
                        0.0f));
                    UCk_Utils_DebugDraw_UE::DrawDebugLine(World, ArcPoint, Apex, LineColor, 0.0f, Thickness);
                }
                break;
            }
            case ECk_Pmg_DebugShape_Type::Star:
            {
                const auto Radius = Params.Get_Size();
                const auto Points = Params.Get_Points();
                const auto InnerRatio = Params.Get_InnerRadiusRatio();
                const auto InnerRadius = Radius * InnerRatio;
                const auto Rot = Transform.GetRotation();

                for (auto i = 0; i < Points * 2; ++i)
                {
                    const auto Angle1 = 2.0f * PI * i / (Points * 2);
                    const auto Angle2 = 2.0f * PI * (i + 1) / (Points * 2);
                    const auto R1 = (i % 2 == 0) ? Radius : InnerRadius;
                    const auto R2 = ((i + 1) % 2 == 0) ? Radius : InnerRadius;

                    const auto P1 = Center + Rot.RotateVector(FVector(
                        R1 * FMath::Cos(Angle1 - PI * 0.5f),
                        R1 * FMath::Sin(Angle1 - PI * 0.5f),
                        0.0f));
                    const auto P2 = Center + Rot.RotateVector(FVector(
                        R2 * FMath::Cos(Angle2 - PI * 0.5f),
                        R2 * FMath::Sin(Angle2 - PI * 0.5f),
                        0.0f));

                    UCk_Utils_DebugDraw_UE::DrawDebugLine(World, P1, P2, LineColor, 0.0f, Thickness);
                }
                break;
            }
            case ECk_Pmg_DebugShape_Type::Plane:
            {
                const auto Width = Params.Get_Size();
                const auto Height = Params.Get_SecondarySize();
                const auto HalfW = Width * 0.5f;
                const auto HalfH = Height * 0.5f;
                const auto Rot = Transform.GetRotation();

                const auto TL = Center + Rot.RotateVector(FVector(-HalfW, -HalfH, 0.0f));
                const auto TR = Center + Rot.RotateVector(FVector(HalfW, -HalfH, 0.0f));
                const auto BR = Center + Rot.RotateVector(FVector(HalfW, HalfH, 0.0f));
                const auto BL = Center + Rot.RotateVector(FVector(-HalfW, HalfH, 0.0f));

                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, TL, TR, LineColor, 0.0f, Thickness);
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, TR, BR, LineColor, 0.0f, Thickness);
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, BR, BL, LineColor, 0.0f, Thickness);
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, BL, TL, LineColor, 0.0f, Thickness);
                break;
            }
            case ECk_Pmg_DebugShape_Type::Pyramid:
            {
                const auto BaseSize = Params.Get_Size();
                const auto Height = Params.Get_SecondarySize();
                const auto HalfSize = BaseSize * 0.5f;
                const auto Rot = Transform.GetRotation();

                const auto BL = Center + Rot.RotateVector(FVector(-HalfSize, -HalfSize, 0.0f));
                const auto BR = Center + Rot.RotateVector(FVector(HalfSize, -HalfSize, 0.0f));
                const auto TR = Center + Rot.RotateVector(FVector(HalfSize, HalfSize, 0.0f));
                const auto TL = Center + Rot.RotateVector(FVector(-HalfSize, HalfSize, 0.0f));
                const auto Apex = Center + Rot.RotateVector(FVector(0, 0, Height));

                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, BL, BR, LineColor, 0.0f, Thickness);
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, BR, TR, LineColor, 0.0f, Thickness);
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, TR, TL, LineColor, 0.0f, Thickness);
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, TL, BL, LineColor, 0.0f, Thickness);

                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, BL, Apex, LineColor, 0.0f, Thickness);
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, BR, Apex, LineColor, 0.0f, Thickness);
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, TR, Apex, LineColor, 0.0f, Thickness);
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, TL, Apex, LineColor, 0.0f, Thickness);
                break;
            }
            case ECk_Pmg_DebugShape_Type::Hemisphere:
            {
                // Draw 3 semi-circles like Sphere: XZ, YZ half-arcs + XY base circle
                const auto Radius = Params.Get_Size();
                const auto ArcSegments = 16;
                const auto Rot = Transform.GetRotation();

                // XZ plane half-circle (front-back)
                for (auto j = 0; j < ArcSegments; ++j)
                {
                    const auto Phi1 = PI * j / ArcSegments; // 0 to PI for half-circle
                    const auto Phi2 = PI * (j + 1) / ArcSegments;

                    const auto P1 = Center + Rot.RotateVector(FVector(
                        Radius * FMath::Cos(Phi1),
                        0.0f,
                        Radius * FMath::Sin(Phi1)));
                    const auto P2 = Center + Rot.RotateVector(FVector(
                        Radius * FMath::Cos(Phi2),
                        0.0f,
                        Radius * FMath::Sin(Phi2)));

                    UCk_Utils_DebugDraw_UE::DrawDebugLine(World, P1, P2, LineColor, 0.0f, Thickness);
                }

                // YZ plane half-circle (left-right)
                for (auto j = 0; j < ArcSegments; ++j)
                {
                    const auto Phi1 = PI * j / ArcSegments;
                    const auto Phi2 = PI * (j + 1) / ArcSegments;

                    const auto P1 = Center + Rot.RotateVector(FVector(
                        0.0f,
                        Radius * FMath::Cos(Phi1),
                        Radius * FMath::Sin(Phi1)));
                    const auto P2 = Center + Rot.RotateVector(FVector(
                        0.0f,
                        Radius * FMath::Cos(Phi2),
                        Radius * FMath::Sin(Phi2)));

                    UCk_Utils_DebugDraw_UE::DrawDebugLine(World, P1, P2, LineColor, 0.0f, Thickness);
                }

                // XY plane full circle (base)
                const auto Segments = Params.Get_Segments();
                for (auto i = 0; i < Segments; ++i)
                {
                    const auto Angle1 = 2.0f * PI * i / Segments;
                    const auto Angle2 = 2.0f * PI * (i + 1) / Segments;

                    const auto P1 = Center + Rot.RotateVector(FVector(
                        Radius * FMath::Cos(Angle1),
                        Radius * FMath::Sin(Angle1),
                        0.0f));
                    const auto P2 = Center + Rot.RotateVector(FVector(
                        Radius * FMath::Cos(Angle2),
                        Radius * FMath::Sin(Angle2),
                        0.0f));

                    UCk_Utils_DebugDraw_UE::DrawDebugLine(World, P1, P2, LineColor, 0.0f, Thickness);
                }
                break;
            }
            case ECk_Pmg_DebugShape_Type::DashedLine:
            {
                // Dashed line doesn't need outline drawing
                break;
            }
            case ECk_Pmg_DebugShape_Type::Checkmark:
            {
                const auto Size = Params.Get_Size();
                const auto CheckThickness = Params.Get_SecondarySize();
                const auto HalfThick = CheckThickness * 0.5f;
                const auto Rot = Transform.GetRotation();

                const auto ShortStart = FVector(-Size * 0.5f, -Size * 0.2f, 0.0f);
                const auto ShortEnd = FVector(0.0f, Size * 0.3f, 0.0f);
                const auto LongEnd = FVector(Size * 0.7f, -Size * 0.5f, 0.0f);

                const auto WorldShortStart = Center + Rot.RotateVector(ShortStart);
                const auto WorldShortEnd = Center + Rot.RotateVector(ShortEnd);
                const auto WorldLongEnd = Center + Rot.RotateVector(LongEnd);

                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, WorldShortStart, WorldShortEnd, LineColor, 0.0f, Thickness);
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, WorldShortEnd, WorldLongEnd, LineColor, 0.0f, Thickness);
                break;
            }
            case ECk_Pmg_DebugShape_Type::Diamond:
            {
                const auto Width = Params.Get_Size();
                const auto Height = Params.Get_SecondarySize();
                const auto HalfW = Width * 0.5f;
                const auto HalfH = Height * 0.5f;
                const auto Rot = Transform.GetRotation();

                const auto Top = Center + Rot.RotateVector(FVector(0.0f, HalfW, 0.0f));
                const auto Right = Center + Rot.RotateVector(FVector(HalfH, 0.0f, 0.0f));
                const auto Bottom = Center + Rot.RotateVector(FVector(0.0f, -HalfW, 0.0f));
                const auto Left = Center + Rot.RotateVector(FVector(-HalfH, 0.0f, 0.0f));

                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, Top, Right, LineColor, 0.0f, Thickness);
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, Right, Bottom, LineColor, 0.0f, Thickness);
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, Bottom, Left, LineColor, 0.0f, Thickness);
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, Left, Top, LineColor, 0.0f, Thickness);
                break;
            }
            case ECk_Pmg_DebugShape_Type::Pivot:
            {
                // Pivot is 3 colored axes - Red=X, Green=Y, Blue=Z
                const auto AxisLength = Params.Get_Size();
                const auto Rot = Transform.GetRotation();

                const auto XEnd = Center + Rot.RotateVector(FVector(AxisLength, 0, 0));
                const auto YEnd = Center + Rot.RotateVector(FVector(0, AxisLength, 0));
                const auto ZEnd = Center + Rot.RotateVector(FVector(0, 0, AxisLength));

                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, Center, XEnd, FLinearColor(1.0f, 0.0f, 0.0f), 0.0f, Thickness);
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, Center, YEnd, FLinearColor(0.0f, 1.0f, 0.0f), 0.0f, Thickness);
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, Center, ZEnd, FLinearColor(0.0f, 0.0f, 1.0f), 0.0f, Thickness);
                break;
            }
            case ECk_Pmg_DebugShape_Type::Warning:
            case ECk_Pmg_DebugShape_Type::Prohibition:
            case ECk_Pmg_DebugShape_Type::NoEntry:
            case ECk_Pmg_DebugShape_Type::MagnifyingGlass:
            case ECk_Pmg_DebugShape_Type::QuestionMark:
            case ECk_Pmg_DebugShape_Type::ExclamationMark:
            case ECk_Pmg_DebugShape_Type::Flag:
            case ECk_Pmg_DebugShape_Type::InfoCircle:
            case ECk_Pmg_DebugShape_Type::Pin:
            {
                // These shapes have their outlines built into the mesh - no separate line drawing needed
                break;
            }
            default:
            {
                break;
            }
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Pmg_DebugShape_EndPlay::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Pmg_DebugShape_Current& InCurrent)
            -> void
    {
        ck::pmg::Verbose(TEXT("Tearing down Pmg DebugShape [{}]"), InHandle);

        auto MeshComponent = InCurrent._MeshComponent.Get();
        if (ck::IsValid(MeshComponent))
        {
            MeshComponent->DestroyComponent();
        }

        InCurrent._MeshComponent.Reset();
    }
}
