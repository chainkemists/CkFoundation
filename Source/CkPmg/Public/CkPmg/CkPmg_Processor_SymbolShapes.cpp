#include "CkPmg_Processor_SymbolShapes.h"

#include "CkCore/Debug/CkDebugDraw_Utils.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

#include "CkPmg/CkPmg_Log.h"

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
    auto GenerateDebugShape_MagnifyingGlass(
        UProceduralMeshComponent* InMeshComponent,
        float InSize,
        ECk_Plane_Axis InAxis)
        -> void
    {
        auto Vertices = TArray<FVector>{};
        auto Triangles = TArray<int32>{};
        auto Normals = TArray<FVector>{};
        auto UVs = TArray<FVector2D>{};

        const auto LensRadius = InSize * 0.4f;
        const auto HandleLength = InSize * 0.5f;
        const auto HandleWidth = InSize * 0.1f;
        const auto Segments = 32;

        Vertices.Add(FVector::ZeroVector);
        Normals.Add(FVector::ForwardVector);
        UVs.Add(FVector2D(0.5f, 0.5f));

        for (auto i = 0; i <= Segments; ++i)
        {
            const auto Angle = 2.0f * PI * i / Segments;
            Vertices.Add(FVector(LensRadius * FMath::Cos(Angle), LensRadius * FMath::Sin(Angle), 0.0f));
            Normals.Add(FVector::ForwardVector);
            UVs.Add(FVector2D(0.5f + 0.5f * FMath::Cos(Angle), 0.5f + 0.5f * FMath::Sin(Angle)));
        }

        for (auto i = 0; i < Segments; ++i)
        {
            Triangles.Add(0); Triangles.Add(i + 1); Triangles.Add(i + 2);
        }

        const auto HandleAngle = -PI * 0.25f;
        const auto HandleStart = FVector(
            LensRadius * FMath::Cos(HandleAngle),
            LensRadius * FMath::Sin(HandleAngle),
            0.0f);
        const auto HandleEnd = HandleStart + FVector(
            HandleLength * FMath::Cos(HandleAngle),
            HandleLength * FMath::Sin(HandleAngle),
            0.0f);

        const auto HandlePerp = FVector(-FMath::Sin(HandleAngle), FMath::Cos(HandleAngle), 0.0f) * HandleWidth * 0.5f;

        auto HandleIdx = Vertices.Num();
        Vertices.Add(HandleStart - HandlePerp);
        Vertices.Add(HandleStart + HandlePerp);
        Vertices.Add(HandleEnd + HandlePerp);
        Vertices.Add(HandleEnd - HandlePerp);
        for (auto i = 0; i < 4; ++i) { Normals.Add(FVector::ForwardVector); }
        UVs.Add(FVector2D(0, 0)); UVs.Add(FVector2D(1, 0)); UVs.Add(FVector2D(1, 1)); UVs.Add(FVector2D(0, 1));
        Triangles.Add(HandleIdx + 0); Triangles.Add(HandleIdx + 1); Triangles.Add(HandleIdx + 2);
        Triangles.Add(HandleIdx + 0); Triangles.Add(HandleIdx + 2); Triangles.Add(HandleIdx + 3);

        ApplyAxisRotation(Vertices, Normals, InAxis);

        InMeshComponent->CreateMeshSection_LinearColor(
            0, Vertices, Triangles, Normals, UVs,
            TArray<FLinearColor>{}, TArray<FProcMeshTangent>{}, true);
    }

    auto GenerateDebugShape_QuestionMark(
        UProceduralMeshComponent* InMeshComponent,
        float InSize,
        ECk_Plane_Axis InAxis)
        -> void
    {
        auto Vertices = TArray<FVector>{};
        auto Triangles = TArray<int32>{};
        auto Normals = TArray<FVector>{};
        auto UVs = TArray<FVector2D>{};

        const auto CurveRadius = InSize * 0.25f;
        const auto Thickness = InSize * 0.12f;
        const auto Segments = 16;

        const auto CurveCenter = FVector(0.0f, 0.0f, InSize * 0.15f);
        for (auto i = 0; i <= Segments; ++i)
        {
            const auto T = static_cast<float>(i) / Segments;
            const auto Angle = PI * T;
            const auto Inner = CurveCenter + FVector(CurveRadius * FMath::Cos(Angle), 0.0f, CurveRadius * FMath::Sin(Angle));
            const auto Outer = CurveCenter + FVector((CurveRadius + Thickness) * FMath::Cos(Angle), 0.0f, (CurveRadius + Thickness) * FMath::Sin(Angle));

            if (i < Segments)
            {
                const auto BaseIdx = Vertices.Num();
                Vertices.Add(Inner);
                Vertices.Add(Outer);
                Normals.Add(FVector::ForwardVector);
                Normals.Add(FVector::ForwardVector);
                UVs.Add(FVector2D(T, 0));
                UVs.Add(FVector2D(T, 1));

                if (i > 0)
                {
                    Triangles.Add(BaseIdx - 2); Triangles.Add(BaseIdx - 1); Triangles.Add(BaseIdx + 1);
                    Triangles.Add(BaseIdx - 2); Triangles.Add(BaseIdx + 1); Triangles.Add(BaseIdx + 0);
                }
            }
        }

        const auto StemTop = CurveCenter + FVector(0.0f, 0.0f, -CurveRadius);
        const auto StemBottom = StemTop + FVector(0.0f, 0.0f, -InSize * 0.2f);
        const auto HalfThick = Thickness * 0.5f;

        auto StemIdx = Vertices.Num();
        Vertices.Add(StemTop + FVector(-HalfThick, 0.0f, 0.0f));
        Vertices.Add(StemTop + FVector(HalfThick, 0.0f, 0.0f));
        Vertices.Add(StemBottom + FVector(HalfThick, 0.0f, 0.0f));
        Vertices.Add(StemBottom + FVector(-HalfThick, 0.0f, 0.0f));
        for (auto i = 0; i < 4; ++i) { Normals.Add(FVector::ForwardVector); }
        UVs.Add(FVector2D(0, 0)); UVs.Add(FVector2D(1, 0)); UVs.Add(FVector2D(1, 1)); UVs.Add(FVector2D(0, 1));
        Triangles.Add(StemIdx + 0); Triangles.Add(StemIdx + 1); Triangles.Add(StemIdx + 2);
        Triangles.Add(StemIdx + 0); Triangles.Add(StemIdx + 2); Triangles.Add(StemIdx + 3);

        const auto DotSize = InSize * 0.08f;
        const auto DotCenter = StemBottom + FVector(0.0f, 0.0f, -DotSize * 1.5f);

        auto DotIdx = Vertices.Num();
        Vertices.Add(DotCenter);
        Vertices.Add(DotCenter + FVector(DotSize, 0.0f, -DotSize * 0.5f));
        Vertices.Add(DotCenter + FVector(0.0f, 0.0f, -DotSize));
        Vertices.Add(DotCenter + FVector(-DotSize, 0.0f, -DotSize * 0.5f));
        for (auto i = 0; i < 4; ++i) { Normals.Add(FVector::ForwardVector); }
        UVs.Add(FVector2D(0.5f, 0.5f)); UVs.Add(FVector2D(1, 0.5f)); UVs.Add(FVector2D(0.5f, 0)); UVs.Add(FVector2D(0, 0.5f));
        Triangles.Add(DotIdx + 0); Triangles.Add(DotIdx + 1); Triangles.Add(DotIdx + 2);
        Triangles.Add(DotIdx + 0); Triangles.Add(DotIdx + 2); Triangles.Add(DotIdx + 3);
        Triangles.Add(DotIdx + 0); Triangles.Add(DotIdx + 3); Triangles.Add(DotIdx + 1);

        ApplyAxisRotation(Vertices, Normals, InAxis);

        InMeshComponent->CreateMeshSection_LinearColor(
            0, Vertices, Triangles, Normals, UVs,
            TArray<FLinearColor>{}, TArray<FProcMeshTangent>{}, true);
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
        const auto DotSize = InSize * 0.1f;

        const auto BarTop = InSize * 0.35f;
        const auto BarBottom = BarTop - BarHeight;

        auto BarIdx = Vertices.Num();
        Vertices.Add(FVector(-BarWidth, 0.0f, BarBottom));
        Vertices.Add(FVector(BarWidth, 0.0f, BarBottom));
        Vertices.Add(FVector(BarWidth, 0.0f, BarTop));
        Vertices.Add(FVector(-BarWidth, 0.0f, BarTop));
        for (auto i = 0; i < 4; ++i) { Normals.Add(FVector::ForwardVector); }
        UVs.Add(FVector2D(0, 0)); UVs.Add(FVector2D(1, 0)); UVs.Add(FVector2D(1, 1)); UVs.Add(FVector2D(0, 1));
        Triangles.Add(BarIdx + 0); Triangles.Add(BarIdx + 1); Triangles.Add(BarIdx + 2);
        Triangles.Add(BarIdx + 0); Triangles.Add(BarIdx + 2); Triangles.Add(BarIdx + 3);

        const auto DotCenter = BarBottom - DotSize * 1.5f;
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
            TArray<FLinearColor>{}, TArray<FProcMeshTangent>{}, true);
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

        const auto PoleRadius = InSize * 0.04f;
        const auto PoleHeight = InSize;
        const auto FlagWidth = InSize * 0.6f;
        const auto FlagHeight = InSize * 0.4f;

        auto PoleIdx = Vertices.Num();
        Vertices.Add(FVector(-PoleRadius, 0.0f, 0.0f));
        Vertices.Add(FVector(PoleRadius, 0.0f, 0.0f));
        Vertices.Add(FVector(PoleRadius, 0.0f, PoleHeight));
        Vertices.Add(FVector(-PoleRadius, 0.0f, PoleHeight));
        for (auto i = 0; i < 4; ++i) { Normals.Add(FVector::ForwardVector); }
        UVs.Add(FVector2D(0, 0)); UVs.Add(FVector2D(1, 0)); UVs.Add(FVector2D(1, 1)); UVs.Add(FVector2D(0, 1));
        Triangles.Add(PoleIdx + 0); Triangles.Add(PoleIdx + 1); Triangles.Add(PoleIdx + 2);
        Triangles.Add(PoleIdx + 0); Triangles.Add(PoleIdx + 2); Triangles.Add(PoleIdx + 3);

        const auto FlagBase = PoleHeight * 0.7f;
        auto FlagIdx = Vertices.Num();
        Vertices.Add(FVector(PoleRadius, 0.0f, FlagBase + FlagHeight));
        Vertices.Add(FVector(FlagWidth, 0.0f, FlagBase + FlagHeight * 0.5f));
        Vertices.Add(FVector(PoleRadius, 0.0f, FlagBase));
        for (auto i = 0; i < 3; ++i) { Normals.Add(FVector::ForwardVector); }
        UVs.Add(FVector2D(0, 1)); UVs.Add(FVector2D(1, 0.5f)); UVs.Add(FVector2D(0, 0));
        Triangles.Add(FlagIdx + 0); Triangles.Add(FlagIdx + 1); Triangles.Add(FlagIdx + 2);

        ApplyAxisRotation(Vertices, Normals, InAxis);

        InMeshComponent->CreateMeshSection_LinearColor(
            0, Vertices, Triangles, Normals, UVs,
            TArray<FLinearColor>{}, TArray<FProcMeshTangent>{}, true);
    }

    auto GenerateDebugShape_Pin(
        UProceduralMeshComponent* InMeshComponent,
        float InSize,
        ECk_Plane_Axis InAxis)
        -> void
    {
        auto Vertices = TArray<FVector>{};
        auto Triangles = TArray<int32>{};
        auto Normals = TArray<FVector>{};
        auto UVs = TArray<FVector2D>{};

        const auto HeadRadius = InSize * 0.3f;
        const auto ShaftRadius = InSize * 0.05f;
        const auto ShaftLength = InSize * 0.6f;
        const auto Segments = 16;

        Vertices.Add(FVector(0.0f, 0.0f, InSize * 0.5f));
        Normals.Add(FVector::ForwardVector);
        UVs.Add(FVector2D(0.5f, 0.5f));

        for (auto i = 0; i <= Segments; ++i)
        {
            const auto Angle = 2.0f * PI * i / Segments;
            const auto X = HeadRadius * FMath::Cos(Angle);
            const auto Y = HeadRadius * FMath::Sin(Angle);
            Vertices.Add(FVector(X, Y, InSize * 0.5f));
            Normals.Add(FVector::ForwardVector);
            UVs.Add(FVector2D(0.5f + 0.5f * FMath::Cos(Angle), 0.5f + 0.5f * FMath::Sin(Angle)));
        }

        for (auto i = 0; i < Segments; ++i)
        {
            Triangles.Add(0); Triangles.Add(i + 1); Triangles.Add(i + 2);
        }

        const auto ShaftTop = InSize * 0.5f - HeadRadius;
        const auto ShaftBottom = ShaftTop - ShaftLength;

        auto ShaftIdx = Vertices.Num();
        Vertices.Add(FVector(-ShaftRadius, 0.0f, ShaftTop));
        Vertices.Add(FVector(ShaftRadius, 0.0f, ShaftTop));
        Vertices.Add(FVector(ShaftRadius, 0.0f, ShaftBottom));
        Vertices.Add(FVector(-ShaftRadius, 0.0f, ShaftBottom));
        for (auto i = 0; i < 4; ++i) { Normals.Add(FVector::ForwardVector); }
        UVs.Add(FVector2D(0, 0)); UVs.Add(FVector2D(1, 0)); UVs.Add(FVector2D(1, 1)); UVs.Add(FVector2D(0, 1));
        Triangles.Add(ShaftIdx + 0); Triangles.Add(ShaftIdx + 1); Triangles.Add(ShaftIdx + 2);
        Triangles.Add(ShaftIdx + 0); Triangles.Add(ShaftIdx + 2); Triangles.Add(ShaftIdx + 3);

        auto PointIdx = Vertices.Num();
        Vertices.Add(FVector(-ShaftRadius, 0.0f, ShaftBottom));
        Vertices.Add(FVector(ShaftRadius, 0.0f, ShaftBottom));
        Vertices.Add(FVector(0.0f, 0.0f, ShaftBottom - ShaftRadius * 2.0f));
        for (auto i = 0; i < 3; ++i) { Normals.Add(FVector::ForwardVector); }
        UVs.Add(FVector2D(0, 1)); UVs.Add(FVector2D(1, 1)); UVs.Add(FVector2D(0.5f, 0));
        Triangles.Add(PointIdx + 0); Triangles.Add(PointIdx + 1); Triangles.Add(PointIdx + 2);

        ApplyAxisRotation(Vertices, Normals, InAxis);

        InMeshComponent->CreateMeshSection_LinearColor(
            0, Vertices, Triangles, Normals, UVs,
            TArray<FLinearColor>{}, TArray<FProcMeshTangent>{}, true);
    }
}

// --------------------------------------------------------------------------------------------------------------------
// Processor ForEachEntity Implementations
// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto FProcessor_Pmg_MagnifyingGlass_Setup::ForEachEntity(
        TimeType InDeltaT,
        HandleType InHandle,
        const FFragment_Pmg_MagnifyingGlass_Params& InParams,
        const FFragment_Pmg_DebugShape_Common& InCommon,
        FFragment_Pmg_DebugShape_Current& InCurrent)
        -> void
    {
        auto MeshComponent = SetupMeshComponent(InHandle, InCommon, InCurrent, InDeltaT.Get_Seconds());
        if (ck::Is_NOT_Valid(MeshComponent)) { return; }

        GenerateDebugShape_MagnifyingGlass(MeshComponent, InParams.Get_Size(), InParams.Get_Axis());
        FinalizeMeshComponent(MeshComponent, InHandle, InCommon, InCurrent, InDeltaT.Get_Seconds());

        if (InCommon.Get_DrawLines())
        {
            const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
            if (ck::Is_NOT_Valid(World)) { return; }

            if (InHandle.Has<FFragment_Transform>())
            {
                const auto& Transform = InHandle.Get<FFragment_Transform>();
                const auto Center = Transform.Get_Transform().GetLocation();
                const auto Rotation = Transform.Get_Transform().GetRotation();
                auto LineColor = InCommon.Get_Color();
                LineColor.A = 1.0f;

                const auto Size = InParams.Get_Size();
                const auto LensRadius = Size * 0.4f;
                const auto HandleLength = Size * 0.5f;
                const auto HandleWidth = Size * 0.1f;
                const auto HandleAngle = -PI * 0.25f;

                auto AxisRotation = FQuat::Identity;
                switch (InParams.Get_Axis())
                {
                    case ECk_Plane_Axis::XY: AxisRotation = FQuat::Identity; break;
                    case ECk_Plane_Axis::XZ: AxisRotation = FQuat(FVector::ForwardVector, PI * 0.5f); break;
                    case ECk_Plane_Axis::YZ: AxisRotation = FQuat(FVector::RightVector, -PI * 0.5f); break;
                }

                const auto FinalRotation = Rotation * AxisRotation;

                UCk_Utils_DebugDraw_UE::DrawDebugCircle_PlaneAxis(
                    World, Center, LensRadius, InParams.Get_Axis(), 32, LineColor,
                    InCommon.Get_Duration().Get_Seconds(), InCommon.Get_LineThickness());

                const auto HandleStart = Center + FinalRotation.RotateVector(FVector(
                    LensRadius * FMath::Cos(HandleAngle),
                    LensRadius * FMath::Sin(HandleAngle),
                    0.0f));
                const auto HandleEnd = HandleStart + FinalRotation.RotateVector(FVector(
                    HandleLength * FMath::Cos(HandleAngle),
                    HandleLength * FMath::Sin(HandleAngle),
                    0.0f));

                const auto HandlePerp = FinalRotation.RotateVector(
                    FVector(-FMath::Sin(HandleAngle), FMath::Cos(HandleAngle), 0.0f) * HandleWidth * 0.5f);

                const auto H1 = HandleStart - HandlePerp;
                const auto H2 = HandleStart + HandlePerp;
                const auto H3 = HandleEnd + HandlePerp;
                const auto H4 = HandleEnd - HandlePerp;

                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, H1, H2, LineColor, InCommon.Get_Duration().Get_Seconds(), InCommon.Get_LineThickness());
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, H2, H3, LineColor, InCommon.Get_Duration().Get_Seconds(), InCommon.Get_LineThickness());
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, H3, H4, LineColor, InCommon.Get_Duration().Get_Seconds(), InCommon.Get_LineThickness());
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, H4, H1, LineColor, InCommon.Get_Duration().Get_Seconds(), InCommon.Get_LineThickness());
            }
        }
    }

    auto FProcessor_Pmg_QuestionMark_Setup::ForEachEntity(
        TimeType InDeltaT,
        HandleType InHandle,
        const FFragment_Pmg_QuestionMark_Params& InParams,
        const FFragment_Pmg_DebugShape_Common& InCommon,
        FFragment_Pmg_DebugShape_Current& InCurrent)
        -> void
    {
        auto MeshComponent = SetupMeshComponent(InHandle, InCommon, InCurrent, InDeltaT.Get_Seconds());
        if (ck::Is_NOT_Valid(MeshComponent)) { return; }

        GenerateDebugShape_QuestionMark(MeshComponent, InParams.Get_Size(), InParams.Get_Axis());
        FinalizeMeshComponent(MeshComponent, InHandle, InCommon, InCurrent, InDeltaT.Get_Seconds());

        if (InCommon.Get_DrawLines())
        {
            const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
            if (ck::Is_NOT_Valid(World)) { return; }

            if (InHandle.Has<FFragment_Transform>())
            {
                const auto& Transform = InHandle.Get<FFragment_Transform>();
                const auto Center = Transform.Get_Transform().GetLocation();
                const auto Rotation = Transform.Get_Transform().GetRotation();
                auto LineColor = InCommon.Get_Color();
                LineColor.A = 1.0f;

                const auto Size = InParams.Get_Size();
                const auto CurveRadius = Size * 0.25f;
                const auto Thickness = Size * 0.12f;
                const auto DotSize = Size * 0.08f;
                const auto Segments = 16;

                auto AxisRotation = FQuat::Identity;
                switch (InParams.Get_Axis())
                {
                    case ECk_Plane_Axis::XY: AxisRotation = FQuat::Identity; break;
                    case ECk_Plane_Axis::XZ: AxisRotation = FQuat(FVector::ForwardVector, PI * 0.5f); break;
                    case ECk_Plane_Axis::YZ: AxisRotation = FQuat(FVector::RightVector, -PI * 0.5f); break;
                }

                const auto FinalRotation = Rotation * AxisRotation;
                const auto CurveCenter = Center + FinalRotation.RotateVector(FVector(0.0f, 0.0f, Size * 0.15f));

                for (auto i = 0; i <= Segments; ++i)
                {
                    const auto T = static_cast<float>(i) / Segments;
                    const auto Angle = PI * T;
                    const auto P = CurveCenter + FinalRotation.RotateVector(FVector(
                        CurveRadius * FMath::Cos(Angle),
                        0.0f,
                        CurveRadius * FMath::Sin(Angle)));

                    if (i > 0)
                    {
                        const auto PrevT = static_cast<float>(i - 1) / Segments;
                        const auto PrevAngle = PI * PrevT;
                        const auto PrevP = CurveCenter + FinalRotation.RotateVector(FVector(
                            CurveRadius * FMath::Cos(PrevAngle),
                            0.0f,
                            CurveRadius * FMath::Sin(PrevAngle)));
                        UCk_Utils_DebugDraw_UE::DrawDebugLine(World, PrevP, P, LineColor,
                            InCommon.Get_Duration().Get_Seconds(), InCommon.Get_LineThickness());
                    }
                }

                const auto StemTop = CurveCenter + FinalRotation.RotateVector(FVector(0.0f, 0.0f, -CurveRadius));
                const auto StemBottom = StemTop + FinalRotation.RotateVector(FVector(0.0f, 0.0f, -Size * 0.2f));
                const auto HalfThick = Thickness * 0.5f;

                const auto S1 = StemTop + FinalRotation.RotateVector(FVector(-HalfThick, 0.0f, 0.0f));
                const auto S2 = StemTop + FinalRotation.RotateVector(FVector(HalfThick, 0.0f, 0.0f));
                const auto S3 = StemBottom + FinalRotation.RotateVector(FVector(HalfThick, 0.0f, 0.0f));
                const auto S4 = StemBottom + FinalRotation.RotateVector(FVector(-HalfThick, 0.0f, 0.0f));

                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, S1, S2, LineColor, InCommon.Get_Duration().Get_Seconds(), InCommon.Get_LineThickness());
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, S2, S3, LineColor, InCommon.Get_Duration().Get_Seconds(), InCommon.Get_LineThickness());
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, S3, S4, LineColor, InCommon.Get_Duration().Get_Seconds(), InCommon.Get_LineThickness());
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, S4, S1, LineColor, InCommon.Get_Duration().Get_Seconds(), InCommon.Get_LineThickness());

                const auto DotCenter = StemBottom + FinalRotation.RotateVector(FVector(0.0f, 0.0f, -DotSize * 1.5f));
                const auto D1 = DotCenter + FinalRotation.RotateVector(FVector(DotSize, 0.0f, -DotSize * 0.5f));
                const auto D2 = DotCenter + FinalRotation.RotateVector(FVector(0.0f, 0.0f, -DotSize));
                const auto D3 = DotCenter + FinalRotation.RotateVector(FVector(-DotSize, 0.0f, -DotSize * 0.5f));

                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, D1, D2, LineColor, InCommon.Get_Duration().Get_Seconds(), InCommon.Get_LineThickness());
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, D2, D3, LineColor, InCommon.Get_Duration().Get_Seconds(), InCommon.Get_LineThickness());
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, D3, D1, LineColor, InCommon.Get_Duration().Get_Seconds(), InCommon.Get_LineThickness());
            }
        }
    }

    auto FProcessor_Pmg_ExclamationMark_Setup::ForEachEntity(
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

        if (InCommon.Get_DrawLines())
        {
            const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
            if (ck::Is_NOT_Valid(World)) { return; }

            if (InHandle.Has<FFragment_Transform>())
            {
                const auto& Transform = InHandle.Get<FFragment_Transform>();
                const auto Center = Transform.Get_Transform().GetLocation();
                const auto Rotation = Transform.Get_Transform().GetRotation();
                auto LineColor = InCommon.Get_Color();
                LineColor.A = 1.0f;

                const auto Size = InParams.Get_Size();
                const auto BarWidth = Size * 0.12f;
                const auto BarHeight = Size * 0.6f;
                const auto DotSize = Size * 0.1f;
                const auto BarTop = Size * 0.35f;
                const auto BarBottom = BarTop - BarHeight;

                auto AxisRotation = FQuat::Identity;
                switch (InParams.Get_Axis())
                {
                    case ECk_Plane_Axis::XY: AxisRotation = FQuat::Identity; break;
                    case ECk_Plane_Axis::XZ: AxisRotation = FQuat(FVector::ForwardVector, PI * 0.5f); break;
                    case ECk_Plane_Axis::YZ: AxisRotation = FQuat(FVector::RightVector, -PI * 0.5f); break;
                }

                const auto FinalRotation = Rotation * AxisRotation;

                const auto B1 = Center + FinalRotation.RotateVector(FVector(-BarWidth, 0.0f, BarBottom));
                const auto B2 = Center + FinalRotation.RotateVector(FVector(BarWidth, 0.0f, BarBottom));
                const auto B3 = Center + FinalRotation.RotateVector(FVector(BarWidth, 0.0f, BarTop));
                const auto B4 = Center + FinalRotation.RotateVector(FVector(-BarWidth, 0.0f, BarTop));

                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, B1, B2, LineColor, InCommon.Get_Duration().Get_Seconds(), InCommon.Get_LineThickness());
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, B2, B3, LineColor, InCommon.Get_Duration().Get_Seconds(), InCommon.Get_LineThickness());
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, B3, B4, LineColor, InCommon.Get_Duration().Get_Seconds(), InCommon.Get_LineThickness());
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, B4, B1, LineColor, InCommon.Get_Duration().Get_Seconds(), InCommon.Get_LineThickness());

                const auto DotCenter = BarBottom - DotSize * 1.5f;
                const auto D1 = Center + FinalRotation.RotateVector(FVector(DotSize, 0.0f, DotCenter - DotSize * 0.5f));
                const auto D2 = Center + FinalRotation.RotateVector(FVector(0.0f, 0.0f, DotCenter - DotSize));
                const auto D3 = Center + FinalRotation.RotateVector(FVector(-DotSize, 0.0f, DotCenter - DotSize * 0.5f));

                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, D1, D2, LineColor, InCommon.Get_Duration().Get_Seconds(), InCommon.Get_LineThickness());
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, D2, D3, LineColor, InCommon.Get_Duration().Get_Seconds(), InCommon.Get_LineThickness());
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, D3, D1, LineColor, InCommon.Get_Duration().Get_Seconds(), InCommon.Get_LineThickness());
            }
        }
    }

    auto FProcessor_Pmg_Flag_Setup::ForEachEntity(
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

        if (InCommon.Get_DrawLines())
        {
            const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
            if (ck::Is_NOT_Valid(World)) { return; }

            if (InHandle.Has<FFragment_Transform>())
            {
                const auto& Transform = InHandle.Get<FFragment_Transform>();
                const auto Center = Transform.Get_Transform().GetLocation();
                const auto Rotation = Transform.Get_Transform().GetRotation();
                auto LineColor = InCommon.Get_Color();
                LineColor.A = 1.0f;

                const auto Size = InParams.Get_Size();
                const auto PoleRadius = Size * 0.04f;
                const auto PoleHeight = Size;
                const auto FlagWidth = Size * 0.6f;
                const auto FlagHeight = Size * 0.4f;
                const auto FlagBase = PoleHeight * 0.7f;

                auto AxisRotation = FQuat::Identity;
                switch (InParams.Get_Axis())
                {
                    case ECk_Plane_Axis::XY: AxisRotation = FQuat::Identity; break;
                    case ECk_Plane_Axis::XZ: AxisRotation = FQuat(FVector::ForwardVector, PI * 0.5f); break;
                    case ECk_Plane_Axis::YZ: AxisRotation = FQuat(FVector::RightVector, -PI * 0.5f); break;
                }

                const auto FinalRotation = Rotation * AxisRotation;

                const auto P1 = Center + FinalRotation.RotateVector(FVector(-PoleRadius, 0.0f, 0.0f));
                const auto P2 = Center + FinalRotation.RotateVector(FVector(PoleRadius, 0.0f, 0.0f));
                const auto P3 = Center + FinalRotation.RotateVector(FVector(PoleRadius, 0.0f, PoleHeight));
                const auto P4 = Center + FinalRotation.RotateVector(FVector(-PoleRadius, 0.0f, PoleHeight));

                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, P1, P2, LineColor, InCommon.Get_Duration().Get_Seconds(), InCommon.Get_LineThickness());
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, P2, P3, LineColor, InCommon.Get_Duration().Get_Seconds(), InCommon.Get_LineThickness());
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, P3, P4, LineColor, InCommon.Get_Duration().Get_Seconds(), InCommon.Get_LineThickness());
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, P4, P1, LineColor, InCommon.Get_Duration().Get_Seconds(), InCommon.Get_LineThickness());

                const auto F1 = Center + FinalRotation.RotateVector(FVector(PoleRadius, 0.0f, FlagBase + FlagHeight));
                const auto F2 = Center + FinalRotation.RotateVector(FVector(FlagWidth, 0.0f, FlagBase + FlagHeight * 0.5f));
                const auto F3 = Center + FinalRotation.RotateVector(FVector(PoleRadius, 0.0f, FlagBase));

                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, F1, F2, LineColor, InCommon.Get_Duration().Get_Seconds(), InCommon.Get_LineThickness());
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, F2, F3, LineColor, InCommon.Get_Duration().Get_Seconds(), InCommon.Get_LineThickness());
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, F3, F1, LineColor, InCommon.Get_Duration().Get_Seconds(), InCommon.Get_LineThickness());
            }
        }
    }

    auto FProcessor_Pmg_Pin_Setup::ForEachEntity(
        TimeType InDeltaT,
        HandleType InHandle,
        const FFragment_Pmg_Pin_Params& InParams,
        const FFragment_Pmg_DebugShape_Common& InCommon,
        FFragment_Pmg_DebugShape_Current& InCurrent)
        -> void
    {
        auto MeshComponent = SetupMeshComponent(InHandle, InCommon, InCurrent, InDeltaT.Get_Seconds());
        if (ck::Is_NOT_Valid(MeshComponent)) { return; }

        GenerateDebugShape_Pin(MeshComponent, InParams.Get_Size(), InParams.Get_Axis());
        FinalizeMeshComponent(MeshComponent, InHandle, InCommon, InCurrent, InDeltaT.Get_Seconds());

        if (InCommon.Get_DrawLines())
        {
            const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
            if (ck::Is_NOT_Valid(World)) { return; }

            if (InHandle.Has<FFragment_Transform>())
            {
                const auto& Transform = InHandle.Get<FFragment_Transform>();
                const auto Center = Transform.Get_Transform().GetLocation();
                const auto Rotation = Transform.Get_Transform().GetRotation();
                auto LineColor = InCommon.Get_Color();
                LineColor.A = 1.0f;

                const auto Size = InParams.Get_Size();
                const auto HeadRadius = Size * 0.3f;
                const auto ShaftRadius = Size * 0.05f;
                const auto ShaftLength = Size * 0.6f;
                const auto ShaftTop = Size * 0.5f - HeadRadius;
                const auto ShaftBottom = ShaftTop - ShaftLength;

                auto AxisRotation = FQuat::Identity;
                switch (InParams.Get_Axis())
                {
                    case ECk_Plane_Axis::XY: AxisRotation = FQuat::Identity; break;
                    case ECk_Plane_Axis::XZ: AxisRotation = FQuat(FVector::ForwardVector, PI * 0.5f); break;
                    case ECk_Plane_Axis::YZ: AxisRotation = FQuat(FVector::RightVector, -PI * 0.5f); break;
                }

                const auto FinalRotation = Rotation * AxisRotation;
                const auto HeadCenter = Center + FinalRotation.RotateVector(FVector(0.0f, 0.0f, Size * 0.5f));

                UCk_Utils_DebugDraw_UE::DrawDebugCircle_PlaneAxis(
                    World, HeadCenter, HeadRadius, InParams.Get_Axis(), 16, LineColor,
                    InCommon.Get_Duration().Get_Seconds(), InCommon.Get_LineThickness());

                const auto S1 = Center + FinalRotation.RotateVector(FVector(-ShaftRadius, 0.0f, ShaftTop));
                const auto S2 = Center + FinalRotation.RotateVector(FVector(ShaftRadius, 0.0f, ShaftTop));
                const auto S3 = Center + FinalRotation.RotateVector(FVector(ShaftRadius, 0.0f, ShaftBottom));
                const auto S4 = Center + FinalRotation.RotateVector(FVector(-ShaftRadius, 0.0f, ShaftBottom));

                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, S1, S2, LineColor, InCommon.Get_Duration().Get_Seconds(), InCommon.Get_LineThickness());
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, S2, S3, LineColor, InCommon.Get_Duration().Get_Seconds(), InCommon.Get_LineThickness());
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, S3, S4, LineColor, InCommon.Get_Duration().Get_Seconds(), InCommon.Get_LineThickness());
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, S4, S1, LineColor, InCommon.Get_Duration().Get_Seconds(), InCommon.Get_LineThickness());

                const auto P1 = Center + FinalRotation.RotateVector(FVector(-ShaftRadius, 0.0f, ShaftBottom));
                const auto P2 = Center + FinalRotation.RotateVector(FVector(ShaftRadius, 0.0f, ShaftBottom));
                const auto P3 = Center + FinalRotation.RotateVector(FVector(0.0f, 0.0f, ShaftBottom - ShaftRadius * 2.0f));

                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, P1, P2, LineColor, InCommon.Get_Duration().Get_Seconds(), InCommon.Get_LineThickness());
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, P2, P3, LineColor, InCommon.Get_Duration().Get_Seconds(), InCommon.Get_LineThickness());
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, P3, P1, LineColor, InCommon.Get_Duration().Get_Seconds(), InCommon.Get_LineThickness());
            }
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
