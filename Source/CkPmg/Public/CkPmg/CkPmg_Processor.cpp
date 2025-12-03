#include "CkPmg_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Debug/CkDebugDraw_Utils.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

#include "CkPmg/CkPmg_Log.h"
#include "CkPmg_Utils.h"

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
    auto
        GenerateDebugShape_Sphere(
            UProceduralMeshComponent* InMeshComponent,
            float InRadius,
            int32 InSegments,
            int32 InRings)
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

                // Reversed winding for correct normals
                Triangles.Add(Current);
                Triangles.Add(Current + 1);
                Triangles.Add(Next);

                Triangles.Add(Current + 1);
                Triangles.Add(Next + 1);
                Triangles.Add(Next);
            }
        }

        InMeshComponent->CreateMeshSection_LinearColor(
            0, Vertices, Triangles, Normals, UVs,
            TArray<FLinearColor>{}, TArray<FProcMeshTangent>{}, true);
    }

    auto
        GenerateDebugShape_Box(
            UProceduralMeshComponent* InMeshComponent,
            const FVector& InExtent)
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

            // Reversed winding for correct normals
            Triangles.Add(BaseIndex + 0);
            Triangles.Add(BaseIndex + 2);
            Triangles.Add(BaseIndex + 1);

            Triangles.Add(BaseIndex + 0);
            Triangles.Add(BaseIndex + 3);
            Triangles.Add(BaseIndex + 2);
        }

        InMeshComponent->CreateMeshSection_LinearColor(
            0, Vertices, Triangles, Normals, UVs,
            TArray<FLinearColor>{}, TArray<FProcMeshTangent>{}, true);
    }

    auto
        GenerateDebugShape_Circle(
            UProceduralMeshComponent* InMeshComponent,
            float InRadius,
            int32 InSegments,
            bool InDrawDirectionLine)
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

        InMeshComponent->CreateMeshSection_LinearColor(
            0, Vertices, Triangles, Normals, UVs,
            TArray<FLinearColor>{}, TArray<FProcMeshTangent>{}, true);
    }

    auto
        GenerateDebugShape_Cone(
            UProceduralMeshComponent* InMeshComponent,
            float InRadius,
            float InHeight,
            int32 InSegments)
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

        InMeshComponent->CreateMeshSection_LinearColor(
            0, Vertices, Triangles, Normals, UVs,
            TArray<FLinearColor>{}, TArray<FProcMeshTangent>{}, true);
    }

    auto
        GenerateDebugShape_Cylinder(
            UProceduralMeshComponent* InMeshComponent,
            float InRadius,
            float InHeight,
            int32 InSegments)
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

        InMeshComponent->CreateMeshSection_LinearColor(
            0, Vertices, Triangles, Normals, UVs,
            TArray<FLinearColor>{}, TArray<FProcMeshTangent>{}, true);
    }

    auto
        GenerateDebugShape_Capsule(
            UProceduralMeshComponent* InMeshComponent,
            float InRadius,
            float InHalfHeight,  // Total half-height of capsule (matching Unreal's DrawDebugCapsule)
            int32 InSegments,
            int32 InRings)
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

        switch (Params.Get_ShapeType())
        {
            case ECk_Pmg_DebugShape_Type::Sphere:
            {
                GenerateDebugShape_Sphere(MeshComponent, Params.Get_Size(),
                    Params.Get_Segments(), Params.Get_Rings());
                break;
            }
            case ECk_Pmg_DebugShape_Type::Box:
            {
                const auto Extent = FVector(Params.Get_Size(), Params.Get_SecondarySize(), Params.Get_TertiarySize());
                GenerateDebugShape_Box(MeshComponent, Extent);
                break;
            }
            case ECk_Pmg_DebugShape_Type::Circle:
            {
                GenerateDebugShape_Circle(MeshComponent, Params.Get_Size(), Params.Get_Segments(), Params.Get_DrawDirectionLine());
                break;
            }
            case ECk_Pmg_DebugShape_Type::Cone:
            {
                GenerateDebugShape_Cone(MeshComponent, Params.Get_Size(),
                    Params.Get_SecondarySize(), Params.Get_Segments());
                break;
            }
            case ECk_Pmg_DebugShape_Type::Cylinder:
            {
                GenerateDebugShape_Cylinder(MeshComponent, Params.Get_Size(),
                    Params.Get_SecondarySize(), Params.Get_Segments());
                break;
            }
            case ECk_Pmg_DebugShape_Type::Capsule:
            {
                GenerateDebugShape_Capsule(MeshComponent, Params.Get_Size(),
                    Params.Get_SecondarySize(), Params.Get_Segments(), Params.Get_Rings());
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
            MeshComponent->SetWorldTransform(Transform.Get_Transform());
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

        if (Duration.Get_Seconds() <= 0.0f)
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
                
                // Draw 1 horizontal ring (equator)
                UCk_Utils_DebugDraw_UE::DrawDebugCircle(
                    World, Center, Radius, 32, LineColor, 0.0f, Thickness);
                
                // Draw 2 vertical rings (longitude arcs perpendicular to each other)
                const auto ArcSegments = 32;
                
                // First longitude arc (XZ plane)
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
                
                // Second longitude arc (YZ plane)
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
                const auto Apex = Center + Transform.GetRotation().RotateVector(FVector(0, 0, Height));

                // Draw base circle
                UCk_Utils_DebugDraw_UE::DrawDebugCircle(
                    World, Center, Radius, Segments, LineColor, 0.0f, Thickness);

                // Draw lines from base to apex
                for (auto i = 0; i < Segments; ++i)
                {
                    const auto Angle = 2.0f * PI * i / Segments;
                    const auto BasePoint = Center + Transform.GetRotation().RotateVector(
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

                const auto TopCenter = Center + Transform.GetRotation().RotateVector(FVector(0, 0, HalfHeight));
                const auto BottomCenter = Center + Transform.GetRotation().RotateVector(FVector(0, 0, -HalfHeight));

                // Draw top and bottom circles
                UCk_Utils_DebugDraw_UE::DrawDebugCircle(
                    World, TopCenter, Radius, Segments, LineColor, 0.0f, Thickness);
                UCk_Utils_DebugDraw_UE::DrawDebugCircle(
                    World, BottomCenter, Radius, Segments, LineColor, 0.0f, Thickness);

                // Draw vertical lines
                for (auto i = 0; i < Segments; i += FMath::Max(1, Segments / 4))
                {
                    const auto Angle = 2.0f * PI * i / Segments;
                    const auto Offset = Transform.GetRotation().RotateVector(
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
