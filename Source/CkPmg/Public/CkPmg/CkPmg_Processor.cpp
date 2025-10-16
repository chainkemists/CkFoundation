#include "CkPmg_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
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
