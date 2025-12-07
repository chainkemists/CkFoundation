#include "CkPmg_Processor_IconShapes.h"

#include "CkCore/Debug/CkDebugDraw_Utils.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

#include "CkPmg/CkPmg_Log.h"
#include "CkPmg_Utils.h"
#include "CkPmg_Utils_BasicShapes.h"
#include "CkPmg_Utils_FlatShapes.h"

#include <MaterialDomain.h>
#include <ProceduralMeshComponent.h>

// --------------------------------------------------------------------------------------------------------------------
// Shared Setup/Finalize Helpers
// --------------------------------------------------------------------------------------------------------------------

namespace
{
    auto SetupMeshComponent(
        FCk_Handle InHandle,
        const ck::FFragment_Pmg_DebugShape_Common& InCommon,
        ck::FFragment_Pmg_DebugShape_Current& InCurrent,
        float InDeltaT)
        -> UProceduralMeshComponent*
    {
        const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
        CK_ENSURE_IF_NOT(ck::IsValid(World), TEXT("Could not get valid World for entity [{}]"), InHandle)
        { return nullptr; }

        auto MeshComponent = NewObject<UProceduralMeshComponent>(
            World,
            UProceduralMeshComponent::StaticClass(),
            NAME_None,
            RF_Transient);

        CK_ENSURE_IF_NOT(ck::IsValid(MeshComponent), TEXT("Failed to create ProceduralMeshComponent for entity [{}]"), InHandle)
        { return nullptr; }

        MeshComponent->SetWorldLocation(FVector::ZeroVector);
        MeshComponent->RegisterComponentWithWorld(World);

        if (MeshComponent->HasBegunPlay() == false)
        {
            MeshComponent->BeginPlay();
        }

        MeshComponent->SetVisibility(true);
        MeshComponent->SetHiddenInGame(false);
        MeshComponent->SetCastShadow(true);

        return MeshComponent;
    }

    auto FinalizeMeshComponent(
        UProceduralMeshComponent* InMeshComponent,
        FCk_Handle InHandle,
        const ck::FFragment_Pmg_DebugShape_Common& InCommon,
        ck::FFragment_Pmg_DebugShape_Current& InCurrent,
        float InDeltaT)
        -> void
    {
        CK_ENSURE_IF_NOT(ck::IsValid(InMeshComponent), TEXT("Invalid mesh component"))
        { return; }

        const auto ShouldBeVisible = InCommon.Get_RenderMode() != ECk_Pmg_RenderMode::Hidden;
        InMeshComponent->SetVisibility(ShouldBeVisible, true);
        InMeshComponent->SetHiddenInGame(!ShouldBeVisible);
        InMeshComponent->bHiddenInGame = !ShouldBeVisible;
        InMeshComponent->SetRenderInMainPass(true);
        InMeshComponent->SetRenderInDepthPass(true);

        static auto TranslucentMaterial = LoadObject<UMaterial>(
            nullptr,
            TEXT("/Engine/EngineDebugMaterials/M_SimpleUnlitTranslucent.M_SimpleUnlitTranslucent"));

        if (ck::IsValid(TranslucentMaterial))
        {
            auto DynamicMaterial = UMaterialInstanceDynamic::Create(TranslucentMaterial, InMeshComponent);
            if (ck::IsValid(DynamicMaterial))
            {
                DynamicMaterial->SetVectorParameterValue(FName("Color"), InCommon.Get_Color());
                InMeshComponent->SetMaterial(0, DynamicMaterial);
            }
        }
        else
        {
            ck::pmg::Warning(TEXT("Failed to load M_SimpleUnlitTranslucent for Pmg DebugShape [{}]"), InHandle);
        }

        InMeshComponent->SetCollisionEnabled(
            InCommon.Get_EnableCollision() ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);

        InMeshComponent->UpdateBounds();
        InMeshComponent->MarkRenderStateDirty();

        InCurrent = ck::FFragment_Pmg_DebugShape_Current{TStrongObjectPtr{InMeshComponent}, FCk_Time{InDeltaT}};
        InHandle.Remove<ck::FTag_Pmg_DebugShape_NeedsSetup>();

        if (InHandle.Has<ck::FFragment_Transform>())
        {
            const auto& Transform = InHandle.Get<ck::FFragment_Transform>();
            const auto& CurrentTransform = Transform.Get_Transform();
            InMeshComponent->SetWorldTransform(CurrentTransform);
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
// Axis Rotation Helpers
// --------------------------------------------------------------------------------------------------------------------

namespace
{
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
}

// --------------------------------------------------------------------------------------------------------------------
// Shape Generation Functions
// --------------------------------------------------------------------------------------------------------------------

namespace
{
    auto GenerateDebugShape_Prohibition(
        UProceduralMeshComponent* InMeshComponent,
        float InRadius,
        int32 InSegments,
        ECk_Plane_Axis InAxis)
        -> void
    {
        auto Vertices = TArray<FVector>{};
        auto Triangles = TArray<int32>{};
        auto Normals = TArray<FVector>{};
        auto UVs = TArray<FVector2D>{};

        Vertices.Add(FVector::ZeroVector);
        Normals.Add(FVector::ForwardVector);
        UVs.Add(FVector2D(0.5f, 0.5f));

        for (auto i = 0; i <= InSegments; ++i)
        {
            const auto Angle = 2.0f * PI * i / InSegments;
            Vertices.Add(FVector(InRadius * FMath::Cos(Angle), InRadius * FMath::Sin(Angle), 0.0f));
            Normals.Add(FVector::ForwardVector);
            UVs.Add(FVector2D(0.5f + 0.5f * FMath::Cos(Angle), 0.5f + 0.5f * FMath::Sin(Angle)));
        }

        for (auto i = 0; i < InSegments; ++i)
        {
            Triangles.Add(0); Triangles.Add(i + 1); Triangles.Add(i + 2);
        }

        const auto BarWidth = InRadius * 0.15f;
        const auto BarLength = InRadius * 1.3f;

        auto Bar1Idx = Vertices.Num();
        const auto Angle1 = PI * 0.25f;
        const auto Perp1 = FVector(-FMath::Sin(Angle1), FMath::Cos(Angle1), 0.0f) * BarWidth;
        const auto Dir1 = FVector(FMath::Cos(Angle1), FMath::Sin(Angle1), 0.0f) * BarLength * 0.5f;

        Vertices.Add(-Dir1 - Perp1);
        Vertices.Add(-Dir1 + Perp1);
        Vertices.Add(Dir1 + Perp1);
        Vertices.Add(Dir1 - Perp1);
        for (auto i = 0; i < 4; ++i) { Normals.Add(FVector::ForwardVector); }
        UVs.Add(FVector2D(0, 0)); UVs.Add(FVector2D(1, 0)); UVs.Add(FVector2D(1, 1)); UVs.Add(FVector2D(0, 1));
        Triangles.Add(Bar1Idx + 0); Triangles.Add(Bar1Idx + 1); Triangles.Add(Bar1Idx + 2);
        Triangles.Add(Bar1Idx + 0); Triangles.Add(Bar1Idx + 2); Triangles.Add(Bar1Idx + 3);

        auto Bar2Idx = Vertices.Num();
        const auto Angle2 = -PI * 0.25f;
        const auto Perp2 = FVector(-FMath::Sin(Angle2), FMath::Cos(Angle2), 0.0f) * BarWidth;
        const auto Dir2 = FVector(FMath::Cos(Angle2), FMath::Sin(Angle2), 0.0f) * BarLength * 0.5f;

        Vertices.Add(-Dir2 - Perp2);
        Vertices.Add(-Dir2 + Perp2);
        Vertices.Add(Dir2 + Perp2);
        Vertices.Add(Dir2 - Perp2);
        for (auto i = 0; i < 4; ++i) { Normals.Add(FVector::ForwardVector); }
        UVs.Add(FVector2D(0, 0)); UVs.Add(FVector2D(1, 0)); UVs.Add(FVector2D(1, 1)); UVs.Add(FVector2D(0, 1));
        Triangles.Add(Bar2Idx + 0); Triangles.Add(Bar2Idx + 1); Triangles.Add(Bar2Idx + 2);
        Triangles.Add(Bar2Idx + 0); Triangles.Add(Bar2Idx + 2); Triangles.Add(Bar2Idx + 3);

        ApplyAxisRotation(Vertices, Normals, InAxis);

        InMeshComponent->CreateMeshSection_LinearColor(
            0, Vertices, Triangles, Normals, UVs,
            TArray<FLinearColor>{}, TArray<FProcMeshTangent>{}, false);
    }

    auto GenerateDebugShape_NoEntry(
        UProceduralMeshComponent* InMeshComponent,
        float InRadius,
        int32 InSegments,
        ECk_Plane_Axis InAxis)
        -> void
    {
        auto Vertices = TArray<FVector>{};
        auto Triangles = TArray<int32>{};
        auto Normals = TArray<FVector>{};
        auto UVs = TArray<FVector2D>{};

        Vertices.Add(FVector::ZeroVector);
        Normals.Add(FVector::ForwardVector);
        UVs.Add(FVector2D(0.5f, 0.5f));

        for (auto i = 0; i <= InSegments; ++i)
        {
            const auto Angle = 2.0f * PI * i / InSegments;
            Vertices.Add(FVector(InRadius * FMath::Cos(Angle), InRadius * FMath::Sin(Angle), 0.0f));
            Normals.Add(FVector::ForwardVector);
            UVs.Add(FVector2D(0.5f + 0.5f * FMath::Cos(Angle), 0.5f + 0.5f * FMath::Sin(Angle)));
        }

        for (auto i = 0; i < InSegments; ++i)
        {
            Triangles.Add(0); Triangles.Add(i + 1); Triangles.Add(i + 2);
        }

        const auto BarWidth = InRadius * 1.3f;
        const auto BarHeight = InRadius * 0.25f;

        auto BarIdx = Vertices.Num();
        Vertices.Add(FVector(-BarWidth * 0.5f, -BarHeight, 0.0f));
        Vertices.Add(FVector(BarWidth * 0.5f, -BarHeight, 0.0f));
        Vertices.Add(FVector(BarWidth * 0.5f, BarHeight, 0.0f));
        Vertices.Add(FVector(-BarWidth * 0.5f, BarHeight, 0.0f));
        for (auto i = 0; i < 4; ++i) { Normals.Add(FVector::ForwardVector); }
        UVs.Add(FVector2D(0, 0)); UVs.Add(FVector2D(1, 0)); UVs.Add(FVector2D(1, 1)); UVs.Add(FVector2D(0, 1));
        Triangles.Add(BarIdx + 0); Triangles.Add(BarIdx + 1); Triangles.Add(BarIdx + 2);
        Triangles.Add(BarIdx + 0); Triangles.Add(BarIdx + 2); Triangles.Add(BarIdx + 3);

        ApplyAxisRotation(Vertices, Normals, InAxis);

        InMeshComponent->CreateMeshSection_LinearColor(
            0, Vertices, Triangles, Normals, UVs,
            TArray<FLinearColor>{}, TArray<FProcMeshTangent>{}, false);
    }

    auto GenerateDebugShape_MagnifyingGlass(
        UProceduralMeshComponent* InMeshComponent,
        float InRadius,
        int32 InSegments,
        ECk_Plane_Axis InAxis)
        -> void
    {
        auto Vertices = TArray<FVector>{};
        auto Triangles = TArray<int32>{};
        auto Normals = TArray<FVector>{};
        auto UVs = TArray<FVector2D>{};

        Vertices.Add(FVector::ZeroVector);
        Normals.Add(FVector::ForwardVector);
        UVs.Add(FVector2D(0.5f, 0.5f));

        for (auto i = 0; i <= InSegments; ++i)
        {
            const auto Angle = 2.0f * PI * i / InSegments;
            Vertices.Add(FVector(InRadius * FMath::Cos(Angle), InRadius * FMath::Sin(Angle), 0.0f));
            Normals.Add(FVector::ForwardVector);
            UVs.Add(FVector2D(0.5f + 0.5f * FMath::Cos(Angle), 0.5f + 0.5f * FMath::Sin(Angle)));
        }

        for (auto i = 0; i < InSegments; ++i)
        {
            Triangles.Add(0); Triangles.Add(i + 1); Triangles.Add(i + 2);
        }

        const auto HandleWidth = InRadius * 0.2f;
        const auto HandleLength = InRadius * 1.2f;
        const auto HandleAngle = PI * 0.75f;
        const auto HandleDir = FVector(FMath::Cos(HandleAngle), FMath::Sin(HandleAngle), 0.0f);
        const auto HandlePerp = FVector(-HandleDir.Y, HandleDir.X, 0.0f);
        const auto HandleStart = HandleDir * InRadius * 0.707f;

        auto HandleIdx = Vertices.Num();
        Vertices.Add(HandleStart - HandlePerp * HandleWidth * 0.5f);
        Vertices.Add(HandleStart + HandlePerp * HandleWidth * 0.5f);
        Vertices.Add(HandleStart + HandleDir * HandleLength + HandlePerp * HandleWidth * 0.5f);
        Vertices.Add(HandleStart + HandleDir * HandleLength - HandlePerp * HandleWidth * 0.5f);
        for (auto i = 0; i < 4; ++i) { Normals.Add(FVector::ForwardVector); }
        UVs.Add(FVector2D(0, 0)); UVs.Add(FVector2D(1, 0)); UVs.Add(FVector2D(1, 1)); UVs.Add(FVector2D(0, 1));
        Triangles.Add(HandleIdx + 0); Triangles.Add(HandleIdx + 1); Triangles.Add(HandleIdx + 2);
        Triangles.Add(HandleIdx + 0); Triangles.Add(HandleIdx + 2); Triangles.Add(HandleIdx + 3);

        ApplyAxisRotation(Vertices, Normals, InAxis);

        InMeshComponent->CreateMeshSection_LinearColor(
            0, Vertices, Triangles, Normals, UVs,
            TArray<FLinearColor>{}, TArray<FProcMeshTangent>{}, false);
    }

    auto GenerateDebugShape_QuestionMark(
        UProceduralMeshComponent* InMeshComponent,
        float InSize,
        int32 InSegments,
        ECk_Plane_Axis InAxis)
        -> void
    {
        auto Vertices = TArray<FVector>{};
        auto Triangles = TArray<int32>{};
        auto Normals = TArray<FVector>{};
        auto UVs = TArray<FVector2D>{};

        const auto CurveRadius = InSize * 0.25f;
        const auto CurveWidth = InSize * 0.12f;
        const auto CurveCenter = FVector(0.0f, 0.0f, InSize * 0.25f);
        const auto StemLength = InSize * 0.15f;
        const auto DotSize = InSize * 0.08f;

        const auto CurveSegs = InSegments / 2;
        for (auto i = 0; i <= CurveSegs; ++i)
        {
            const auto Angle = PI * (0.5f + i * 0.8f / CurveSegs);
            const auto InnerR = CurveRadius - CurveWidth * 0.5f;
            const auto OuterR = CurveRadius + CurveWidth * 0.5f;
            const auto Cos = FMath::Cos(Angle);
            const auto Sin = FMath::Sin(Angle);

            const auto InnerVert = CurveCenter + FVector(InnerR * Cos, InnerR * Sin, 0.0f);
            const auto OuterVert = CurveCenter + FVector(OuterR * Cos, OuterR * Sin, 0.0f);

            Vertices.Add(InnerVert);
            Vertices.Add(OuterVert);
            Normals.Add(FVector::ForwardVector);
            Normals.Add(FVector::ForwardVector);
            UVs.Add(FVector2D(0, float(i) / CurveSegs));
            UVs.Add(FVector2D(1, float(i) / CurveSegs));

            if (i < CurveSegs)
            {
                const auto BaseIdx = i * 2;
                Triangles.Add(BaseIdx + 0); Triangles.Add(BaseIdx + 1); Triangles.Add(BaseIdx + 3);
                Triangles.Add(BaseIdx + 0); Triangles.Add(BaseIdx + 3); Triangles.Add(BaseIdx + 2);
            }
        }

        const auto StemBottom = CurveCenter.Z - CurveRadius - StemLength;
        auto StemIdx = Vertices.Num();
        Vertices.Add(FVector(-CurveWidth * 0.5f, 0.0f, CurveCenter.Z - CurveRadius));
        Vertices.Add(FVector(CurveWidth * 0.5f, 0.0f, CurveCenter.Z - CurveRadius));
        Vertices.Add(FVector(CurveWidth * 0.5f, 0.0f, StemBottom));
        Vertices.Add(FVector(-CurveWidth * 0.5f, 0.0f, StemBottom));
        for (auto i = 0; i < 4; ++i) { Normals.Add(FVector::ForwardVector); }
        UVs.Add(FVector2D(0, 0)); UVs.Add(FVector2D(1, 0)); UVs.Add(FVector2D(1, 1)); UVs.Add(FVector2D(0, 1));
        Triangles.Add(StemIdx + 0); Triangles.Add(StemIdx + 1); Triangles.Add(StemIdx + 2);
        Triangles.Add(StemIdx + 0); Triangles.Add(StemIdx + 2); Triangles.Add(StemIdx + 3);

        const auto DotCenter = StemBottom - DotSize * 1.5f;
        auto DotIdx = Vertices.Num();
        Vertices.Add(FVector(0.0f, 0.0f, DotCenter));
        Vertices.Add(FVector(DotSize, 0.0f, DotCenter - DotSize * 0.5f));
        Vertices.Add(FVector(0.0f, 0.0f, DotCenter - DotSize));
        Vertices.Add(FVector(-DotSize, 0.0f, DotCenter - DotSize * 0.5f));
        for (auto i = 0; i < 4; ++i) { Normals.Add(FVector::ForwardVector); }
        UVs.Add(FVector2D(0.5f, 0.5f)); UVs.Add(FVector2D(1, 0.5f)); UVs.Add(FVector2D(0.5f, 0)); UVs.Add(FVector2D(0, 0.5f));
        Triangles.Add(DotIdx + 0); Triangles.Add(DotIdx + 1); Triangles.Add(DotIdx + 2);
        Triangles.Add(DotIdx + 0); Triangles.Add(DotIdx + 2); Triangles.Add(DotIdx + 3);
        Triangles.Add(DotIdx + 0); Triangles.Add(DotIdx + 3); Triangles.Add(DotIdx + 1);

        ApplyAxisRotation(Vertices, Normals, InAxis);

        InMeshComponent->CreateMeshSection_LinearColor(
            0, Vertices, Triangles, Normals, UVs,
            TArray<FLinearColor>{}, TArray<FProcMeshTangent>{}, false);
    }

    auto GenerateDebugShape_ExclamationMark(
        UProceduralMeshComponent* InMeshComponent,
        float InSize,
        ECk_Plane_Axis InAxis)
        -> void
    {
        auto Vertices = TArray<FVector>{};
        auto Triangles = TArray<int32>{};
        auto Normals = TArray<FVector>{};
        auto UVs = TArray<FVector2D>{};

        const auto BarWidth = InSize * 0.12f;
        const auto BarHeight = InSize * 0.6f;
        const auto DotSize = InSize * 0.08f;
        const auto TopY = InSize * 0.3f;

        auto BarIdx = Vertices.Num();
        Vertices.Add(FVector(-BarWidth, 0.0f, TopY - BarHeight));
        Vertices.Add(FVector(BarWidth, 0.0f, TopY - BarHeight));
        Vertices.Add(FVector(BarWidth, 0.0f, TopY));
        Vertices.Add(FVector(-BarWidth, 0.0f, TopY));
        for (auto i = 0; i < 4; ++i) { Normals.Add(FVector::ForwardVector); }
        UVs.Add(FVector2D(0, 0)); UVs.Add(FVector2D(1, 0)); UVs.Add(FVector2D(1, 1)); UVs.Add(FVector2D(0, 1));
        Triangles.Add(BarIdx + 0); Triangles.Add(BarIdx + 1); Triangles.Add(BarIdx + 2);
        Triangles.Add(BarIdx + 0); Triangles.Add(BarIdx + 2); Triangles.Add(BarIdx + 3);

        const auto DotCenter = TopY - BarHeight - DotSize * 2.0f;
        auto DotIdx = Vertices.Num();
        Vertices.Add(FVector(0.0f, 0.0f, DotCenter));
        Vertices.Add(FVector(DotSize, 0.0f, DotCenter - DotSize * 0.5f));
        Vertices.Add(FVector(0.0f, 0.0f, DotCenter - DotSize));
        Vertices.Add(FVector(-DotSize, 0.0f, DotCenter - DotSize * 0.5f));
        for (auto i = 0; i < 4; ++i) { Normals.Add(FVector::ForwardVector); }
        UVs.Add(FVector2D(0.5f, 0.5f)); UVs.Add(FVector2D(1, 0.5f)); UVs.Add(FVector2D(0.5f, 0)); UVs.Add(FVector2D(0, 0.5f));
        Triangles.Add(DotIdx + 0); Triangles.Add(DotIdx + 1); Triangles.Add(DotIdx + 2);
        Triangles.Add(DotIdx + 0); Triangles.Add(DotIdx + 2); Triangles.Add(DotIdx + 3);
        Triangles.Add(DotIdx + 0); Triangles.Add(DotIdx + 3); Triangles.Add(DotIdx + 1);

        ApplyAxisRotation(Vertices, Normals, InAxis);

        InMeshComponent->CreateMeshSection_LinearColor(
            0, Vertices, Triangles, Normals, UVs,
            TArray<FLinearColor>{}, TArray<FProcMeshTangent>{}, false);
    }

    auto GenerateDebugShape_Flag(
        UProceduralMeshComponent* InMeshComponent,
        float InSize,
        ECk_Plane_Axis InAxis)
        -> void
    {
        auto Vertices = TArray<FVector>{};
        auto Triangles = TArray<int32>{};
        auto Normals = TArray<FVector>{};
        auto UVs = TArray<FVector2D>{};

        const auto PoleWidth = InSize * 0.05f;
        const auto PoleHeight = InSize;
        const auto FlagWidth = InSize * 0.6f;
        const auto FlagHeight = InSize * 0.4f;
        const auto FlagTop = PoleHeight * 0.5f;

        auto PoleIdx = Vertices.Num();
        Vertices.Add(FVector(-PoleWidth, 0.0f, -PoleHeight * 0.5f));
        Vertices.Add(FVector(PoleWidth, 0.0f, -PoleHeight * 0.5f));
        Vertices.Add(FVector(PoleWidth, 0.0f, PoleHeight * 0.5f));
        Vertices.Add(FVector(-PoleWidth, 0.0f, PoleHeight * 0.5f));
        for (auto i = 0; i < 4; ++i) { Normals.Add(FVector::ForwardVector); }
        UVs.Add(FVector2D(0, 0)); UVs.Add(FVector2D(1, 0)); UVs.Add(FVector2D(1, 1)); UVs.Add(FVector2D(0, 1));
        Triangles.Add(PoleIdx + 0); Triangles.Add(PoleIdx + 1); Triangles.Add(PoleIdx + 2);
        Triangles.Add(PoleIdx + 0); Triangles.Add(PoleIdx + 2); Triangles.Add(PoleIdx + 3);

        auto FlagIdx = Vertices.Num();
        Vertices.Add(FVector(PoleWidth, 0.0f, FlagTop));
        Vertices.Add(FVector(PoleWidth + FlagWidth, 0.0f, FlagTop - FlagHeight * 0.5f));
        Vertices.Add(FVector(PoleWidth, 0.0f, FlagTop - FlagHeight));
        for (auto i = 0; i < 3; ++i) { Normals.Add(FVector::ForwardVector); }
        UVs.Add(FVector2D(0, 1)); UVs.Add(FVector2D(1, 0.5f)); UVs.Add(FVector2D(0, 0));
        Triangles.Add(FlagIdx + 0); Triangles.Add(FlagIdx + 1); Triangles.Add(FlagIdx + 2);

        ApplyAxisRotation(Vertices, Normals, InAxis);

        InMeshComponent->CreateMeshSection_LinearColor(
            0, Vertices, Triangles, Normals, UVs,
            TArray<FLinearColor>{}, TArray<FProcMeshTangent>{}, false);
    }

    auto GenerateDebugShape_InfoCircle(
        UProceduralMeshComponent* InMeshComponent,
        float InRadius,
        int32 InSegments,
        ECk_Plane_Axis InAxis)
        -> void
    {
        auto Vertices = TArray<FVector>{};
        auto Triangles = TArray<int32>{};
        auto Normals = TArray<FVector>{};
        auto UVs = TArray<FVector2D>{};

        Vertices.Add(FVector::ZeroVector);
        Normals.Add(FVector::ForwardVector);
        UVs.Add(FVector2D(0.5f, 0.5f));

        for (auto i = 0; i <= InSegments; ++i)
        {
            const auto Angle = 2.0f * PI * i / InSegments;
            Vertices.Add(FVector(InRadius * FMath::Cos(Angle), InRadius * FMath::Sin(Angle), 0.0f));
            Normals.Add(FVector::ForwardVector);
            UVs.Add(FVector2D(0.5f + 0.5f * FMath::Cos(Angle), 0.5f + 0.5f * FMath::Sin(Angle)));
        }

        for (auto i = 0; i < InSegments; ++i)
        {
            Triangles.Add(0); Triangles.Add(i + 1); Triangles.Add(i + 2);
        }

        const auto DotSize = InRadius * 0.12f;
        const auto DotY = InRadius * 0.3f;
        const auto BarWidth = InRadius * 0.12f;
        const auto BarHeight = InRadius * 0.5f;
        const auto BarTop = -InRadius * 0.05f;

        auto DotIdx = Vertices.Num();
        Vertices.Add(FVector(0.0f, 0.0f, DotY));
        Vertices.Add(FVector(DotSize, 0.0f, DotY - DotSize * 0.5f));
        Vertices.Add(FVector(0.0f, 0.0f, DotY - DotSize));
        Vertices.Add(FVector(-DotSize, 0.0f, DotY - DotSize * 0.5f));
        for (auto i = 0; i < 4; ++i) { Normals.Add(FVector::ForwardVector); }
        UVs.Add(FVector2D(0.5f, 0.5f)); UVs.Add(FVector2D(1, 0.5f)); UVs.Add(FVector2D(0.5f, 0)); UVs.Add(FVector2D(0, 0.5f));
        Triangles.Add(DotIdx + 0); Triangles.Add(DotIdx + 1); Triangles.Add(DotIdx + 2);
        Triangles.Add(DotIdx + 0); Triangles.Add(DotIdx + 2); Triangles.Add(DotIdx + 3);
        Triangles.Add(DotIdx + 0); Triangles.Add(DotIdx + 3); Triangles.Add(DotIdx + 1);

        auto BarIdx = Vertices.Num();
        Vertices.Add(FVector(-BarWidth, 0.0f, BarTop - BarHeight));
        Vertices.Add(FVector(BarWidth, 0.0f, BarTop - BarHeight));
        Vertices.Add(FVector(BarWidth, 0.0f, BarTop));
        Vertices.Add(FVector(-BarWidth, 0.0f, BarTop));
        for (auto i = 0; i < 4; ++i) { Normals.Add(FVector::ForwardVector); }
        UVs.Add(FVector2D(0, 0)); UVs.Add(FVector2D(1, 0)); UVs.Add(FVector2D(1, 1)); UVs.Add(FVector2D(0, 1));
        Triangles.Add(BarIdx + 0); Triangles.Add(BarIdx + 1); Triangles.Add(BarIdx + 2);
        Triangles.Add(BarIdx + 0); Triangles.Add(BarIdx + 2); Triangles.Add(BarIdx + 3);

        ApplyAxisRotation(Vertices, Normals, InAxis);

        InMeshComponent->CreateMeshSection_LinearColor(
            0, Vertices, Triangles, Normals, UVs,
            TArray<FLinearColor>{}, TArray<FProcMeshTangent>{}, false);
    }

    auto GenerateDebugShape_Pin(
        UProceduralMeshComponent* InMeshComponent,
        float InSize,
        int32 InSegments,
        ECk_Plane_Axis InAxis)
        -> void
    {
        auto Vertices = TArray<FVector>{};
        auto Triangles = TArray<int32>{};
        auto Normals = TArray<FVector>{};
        auto UVs = TArray<FVector2D>{};

        const auto CircleRadius = InSize * 0.3f;
        const auto CircleCenter = FVector(0.0f, 0.0f, InSize * 0.2f);

        Vertices.Add(CircleCenter);
        Normals.Add(FVector::ForwardVector);
        UVs.Add(FVector2D(0.5f, 0.5f));

        for (auto i = 0; i <= InSegments; ++i)
        {
            const auto Angle = 2.0f * PI * i / InSegments;
            Vertices.Add(CircleCenter + FVector(CircleRadius * FMath::Cos(Angle), CircleRadius * FMath::Sin(Angle), 0.0f));
            Normals.Add(FVector::ForwardVector);
            UVs.Add(FVector2D(0.5f + 0.5f * FMath::Cos(Angle), 0.5f + 0.5f * FMath::Sin(Angle)));
        }

        for (auto i = 0; i < InSegments; ++i)
        {
            Triangles.Add(0); Triangles.Add(i + 1); Triangles.Add(i + 2);
        }

        const auto PointTop = CircleCenter.Z - CircleRadius;
        const auto PointBottom = -InSize * 0.5f;
        const auto PointWidth = InSize * 0.15f;

        auto PointIdx = Vertices.Num();
        Vertices.Add(FVector(-PointWidth, 0.0f, PointTop));
        Vertices.Add(FVector(PointWidth, 0.0f, PointTop));
        Vertices.Add(FVector(0.0f, 0.0f, PointBottom));
        for (auto i = 0; i < 3; ++i) { Normals.Add(FVector::ForwardVector); }
        UVs.Add(FVector2D(0, 1)); UVs.Add(FVector2D(1, 1)); UVs.Add(FVector2D(0.5f, 0));
        Triangles.Add(PointIdx + 0); Triangles.Add(PointIdx + 1); Triangles.Add(PointIdx + 2);

        ApplyAxisRotation(Vertices, Normals, InAxis);

        InMeshComponent->CreateMeshSection_LinearColor(
            0, Vertices, Triangles, Normals, UVs,
            TArray<FLinearColor>{}, TArray<FProcMeshTangent>{}, false);
    }
}

// --------------------------------------------------------------------------------------------------------------------
// Processor Implementations
// --------------------------------------------------------------------------------------------------------------------

auto
    ck::FProcessor_Pmg_Warning_Setup::
    ForEachEntity(
        TimeType InDeltaT,
        HandleType InHandle,
        const FFragment_Pmg_Warning_Params& InParams,
        const FFragment_Pmg_DebugShape_Common& InCommon,
        FFragment_Pmg_DebugShape_Current& InCurrent)
    -> void
{
    InHandle.Remove<FTag_Pmg_DebugShape_NeedsSetup>();

    const auto Size = InParams.Get_Size();
    const auto Axis = InParams.Get_Axis();
    const auto Color = InCommon.Get_Color();
    const auto DrawLines = InCommon.Get_DrawLines();
    const auto LineThickness = InCommon.Get_LineThickness();
    const auto Duration = InCommon.Get_Duration().Get_Seconds();

    const auto OutlineColor = FLinearColor(Color.R, Color.G, Color.B, 0.0f);
    const auto FillColor = Color;

    const auto Height = Size * 0.866f;
    const auto TriangleTop = Height * 0.667f;
    const auto TriangleBottom = -Height * 0.333f;
    const auto TriangleHeight = TriangleTop - TriangleBottom;

    const auto TopMargin = TriangleHeight * 0.15f;
    const auto BottomMargin = TriangleHeight * 0.12f;
    const auto ExclamationTop = TriangleTop - TopMargin;
    const auto ExclamationBottom = TriangleBottom + BottomMargin;

    const auto BarHalfWidth = Size * 0.08f;
    const auto DotRadius = Size * 0.07f;
    const auto BarDotGap = Size * 0.06f;

    const auto DotCenterZ = ExclamationBottom + DotRadius;
    const auto BarBottom = DotCenterZ + DotRadius + BarDotGap;
    const auto BarTop = ExclamationTop;
    const auto BarHeight = BarTop - BarBottom;
    const auto BarCenterZ = (BarTop + BarBottom) * 0.5f;

    const auto TriangleRadius = Size * 0.5f;
    const auto TriangleRotation = FRotator(0.0f, 0.0f, 30.0f);
    UCk_Utils_Pmg_FlatShapes::Create_Circle(
        InHandle,
        FTransform(TriangleRotation, FVector::ZeroVector, FVector::OneVector),
        TriangleRadius,
        3,
        OutlineColor,
        DrawLines,
        LineThickness,
        false,
        Axis,
        Duration);

    const auto BarExtent = FVector(BarHalfWidth, 1.0f, BarHeight * 0.5f);
    //UCk_Utils_Pmg_BasicShapes::Create_Box(
    //    InHandle,
    //    FTransform(FRotator::ZeroRotator, FVector(0.0f, 0.0f, BarCenterZ), FVector::OneVector),
    //    BarExtent,
    //    FillColor,
    //    false,
    //    LineThickness,
    //    Duration);

    UCk_Utils_Pmg_FlatShapes::Create_Circle(
        InHandle,
        FTransform(FRotator::ZeroRotator, FVector(0.0f, 0.0f, DotCenterZ), FVector::OneVector),
        DotRadius,
        16,
        FillColor,
        false,
        LineThickness,
        false,
        Axis,
        Duration);
}

auto
    ck::FProcessor_Pmg_Prohibition_Setup::
    ForEachEntity(
        TimeType InDeltaT,
        HandleType InHandle,
        const FFragment_Pmg_Prohibition_Params& InParams,
        const FFragment_Pmg_DebugShape_Common& InCommon,
        FFragment_Pmg_DebugShape_Current& InCurrent)
    -> void
{
    auto MeshComponent = SetupMeshComponent(InHandle, InCommon, InCurrent, InDeltaT.Get_Seconds());
    if (ck::Is_NOT_Valid(MeshComponent)) { return; }

    GenerateDebugShape_Prohibition(MeshComponent, InParams.Get_Radius(), InParams.Get_Segments(), InParams.Get_Axis());
    FinalizeMeshComponent(MeshComponent, InHandle, InCommon, InCurrent, InDeltaT.Get_Seconds());
}

auto
    ck::FProcessor_Pmg_NoEntry_Setup::
    ForEachEntity(
        TimeType InDeltaT,
        HandleType InHandle,
        const FFragment_Pmg_NoEntry_Params& InParams,
        const FFragment_Pmg_DebugShape_Common& InCommon,
        FFragment_Pmg_DebugShape_Current& InCurrent)
    -> void
{
    auto MeshComponent = SetupMeshComponent(InHandle, InCommon, InCurrent, InDeltaT.Get_Seconds());
    if (ck::Is_NOT_Valid(MeshComponent)) { return; }

    GenerateDebugShape_NoEntry(MeshComponent, InParams.Get_Radius(), InParams.Get_Segments(), InParams.Get_Axis());
    FinalizeMeshComponent(MeshComponent, InHandle, InCommon, InCurrent, InDeltaT.Get_Seconds());
}

auto
    ck::FProcessor_Pmg_MagnifyingGlass_Setup::
    ForEachEntity(
        TimeType InDeltaT,
        HandleType InHandle,
        const FFragment_Pmg_MagnifyingGlass_Params& InParams,
        const FFragment_Pmg_DebugShape_Common& InCommon,
        FFragment_Pmg_DebugShape_Current& InCurrent)
    -> void
{
    auto MeshComponent = SetupMeshComponent(InHandle, InCommon, InCurrent, InDeltaT.Get_Seconds());
    if (ck::Is_NOT_Valid(MeshComponent)) { return; }

    GenerateDebugShape_MagnifyingGlass(MeshComponent, InParams.Get_Radius(), InParams.Get_Segments(), InParams.Get_Axis());
    FinalizeMeshComponent(MeshComponent, InHandle, InCommon, InCurrent, InDeltaT.Get_Seconds());
}

auto
    ck::FProcessor_Pmg_QuestionMark_Setup::
    ForEachEntity(
        TimeType InDeltaT,
        HandleType InHandle,
        const FFragment_Pmg_QuestionMark_Params& InParams,
        const FFragment_Pmg_DebugShape_Common& InCommon,
        FFragment_Pmg_DebugShape_Current& InCurrent)
    -> void
{
    auto MeshComponent = SetupMeshComponent(InHandle, InCommon, InCurrent, InDeltaT.Get_Seconds());
    if (ck::Is_NOT_Valid(MeshComponent)) { return; }

    GenerateDebugShape_QuestionMark(MeshComponent, InParams.Get_Size(), InParams.Get_Segments(), InParams.Get_Axis());
    FinalizeMeshComponent(MeshComponent, InHandle, InCommon, InCurrent, InDeltaT.Get_Seconds());
}

auto
    ck::FProcessor_Pmg_ExclamationMark_Setup::
    ForEachEntity(
        TimeType InDeltaT,
        HandleType InHandle,
        const FFragment_Pmg_ExclamationMark_Params& InParams,
        const FFragment_Pmg_DebugShape_Common& InCommon,
        FFragment_Pmg_DebugShape_Current& InCurrent)
    -> void
{
    auto MeshComponent = SetupMeshComponent(InHandle, InCommon, InCurrent, InDeltaT.Get_Seconds());
    if (ck::Is_NOT_Valid(MeshComponent)) { return; }

    GenerateDebugShape_ExclamationMark(MeshComponent, InParams.Get_Size(), InParams.Get_Axis());
    FinalizeMeshComponent(MeshComponent, InHandle, InCommon, InCurrent, InDeltaT.Get_Seconds());
}

auto
    ck::FProcessor_Pmg_Flag_Setup::
    ForEachEntity(
        TimeType InDeltaT,
        HandleType InHandle,
        const FFragment_Pmg_Flag_Params& InParams,
        const FFragment_Pmg_DebugShape_Common& InCommon,
        FFragment_Pmg_DebugShape_Current& InCurrent)
    -> void
{
    auto MeshComponent = SetupMeshComponent(InHandle, InCommon, InCurrent, InDeltaT.Get_Seconds());
    if (ck::Is_NOT_Valid(MeshComponent)) { return; }

    GenerateDebugShape_Flag(MeshComponent, InParams.Get_Size(), InParams.Get_Axis());
    FinalizeMeshComponent(MeshComponent, InHandle, InCommon, InCurrent, InDeltaT.Get_Seconds());
}

auto
    ck::FProcessor_Pmg_InfoCircle_Setup::
    ForEachEntity(
        TimeType InDeltaT,
        HandleType InHandle,
        const FFragment_Pmg_InfoCircle_Params& InParams,
        const FFragment_Pmg_DebugShape_Common& InCommon,
        FFragment_Pmg_DebugShape_Current& InCurrent)
    -> void
{
    auto MeshComponent = SetupMeshComponent(InHandle, InCommon, InCurrent, InDeltaT.Get_Seconds());
    if (ck::Is_NOT_Valid(MeshComponent)) { return; }

    GenerateDebugShape_InfoCircle(MeshComponent, InParams.Get_Radius(), InParams.Get_Segments(), InParams.Get_Axis());
    FinalizeMeshComponent(MeshComponent, InHandle, InCommon, InCurrent, InDeltaT.Get_Seconds());
}

auto
    ck::FProcessor_Pmg_Pin_Setup::
    ForEachEntity(
        TimeType InDeltaT,
        HandleType InHandle,
        const FFragment_Pmg_Pin_Params& InParams,
        const FFragment_Pmg_DebugShape_Common& InCommon,
        FFragment_Pmg_DebugShape_Current& InCurrent)
    -> void
{
    auto MeshComponent = SetupMeshComponent(InHandle, InCommon, InCurrent, InDeltaT.Get_Seconds());
    if (ck::Is_NOT_Valid(MeshComponent)) { return; }

    GenerateDebugShape_Pin(MeshComponent, InParams.Get_Size(), InParams.Get_Segments(), InParams.Get_Axis());
    FinalizeMeshComponent(MeshComponent, InHandle, InCommon, InCurrent, InDeltaT.Get_Seconds());
}

// --------------------------------------------------------------------------------------------------------------------
