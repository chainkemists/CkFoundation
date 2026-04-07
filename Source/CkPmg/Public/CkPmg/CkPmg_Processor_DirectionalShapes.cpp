#include "CkPmg_Processor_DirectionalShapes.h"

#include "CkCore/Debug/CkDebugDraw_Utils.h"
#include "CkCore/Math/Vector/CkVector_Utils.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

#include "CkPmg/CkPmg_Log.h"
#include "CkPmg/CkPmg_Utils_DirectionalShapes.h"
#include "CkPmg/CkPmg_Utils_FlatShapes.h"

#include <MaterialDomain.h>
#include <ProceduralMeshComponent.h>

// --------------------------------------------------------------------------------------------------------------------
// Shape Generation Functions
// --------------------------------------------------------------------------------------------------------------------

namespace
{
    auto GenerateDebugShape_Arrow(
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

        const auto ShaftLength = InLength * (1.0f - InArrowHeadRatio);
        const auto HeadLength = InLength * InArrowHeadRatio;
        const auto HalfShaftWidth = InShaftWidth * 0.5f;
        const auto HalfHeadWidth = (InShaftWidth * InArrowHeadWidthMultiplier) * 0.5f;

        // Arrow points along +X axis in XY plane
        const auto ShaftStart = 0.0f;
        const auto ShaftEnd = ShaftLength;
        const auto HeadEnd = InLength;

        // Shaft front face
        auto BaseIdx = Vertices.Num();
        Vertices.Add(FVector(ShaftStart, -HalfShaftWidth, 0.0f));
        Vertices.Add(FVector(ShaftEnd, -HalfShaftWidth, 0.0f));
        Vertices.Add(FVector(ShaftEnd, HalfShaftWidth, 0.0f));
        Vertices.Add(FVector(ShaftStart, HalfShaftWidth, 0.0f));
        for (auto i = 0; i < 4; ++i) { Normals.Add(FVector::UpVector); }
        UVs.Add(FVector2D(0, 0)); UVs.Add(FVector2D(1, 0)); UVs.Add(FVector2D(1, 1)); UVs.Add(FVector2D(0, 1));
        Triangles.Add(BaseIdx + 0); Triangles.Add(BaseIdx + 1); Triangles.Add(BaseIdx + 2);
        Triangles.Add(BaseIdx + 0); Triangles.Add(BaseIdx + 2); Triangles.Add(BaseIdx + 3);

        // Shaft back face
        BaseIdx = Vertices.Num();
        Vertices.Add(FVector(ShaftStart, -HalfShaftWidth, 0.0f));
        Vertices.Add(FVector(ShaftEnd, -HalfShaftWidth, 0.0f));
        Vertices.Add(FVector(ShaftEnd, HalfShaftWidth, 0.0f));
        Vertices.Add(FVector(ShaftStart, HalfShaftWidth, 0.0f));
        for (auto i = 0; i < 4; ++i) { Normals.Add(FVector::DownVector); }
        UVs.Add(FVector2D(0, 0)); UVs.Add(FVector2D(1, 0)); UVs.Add(FVector2D(1, 1)); UVs.Add(FVector2D(0, 1));
        Triangles.Add(BaseIdx + 0); Triangles.Add(BaseIdx + 2); Triangles.Add(BaseIdx + 1);
        Triangles.Add(BaseIdx + 0); Triangles.Add(BaseIdx + 3); Triangles.Add(BaseIdx + 2);

        // Arrow head front face (triangle)
        BaseIdx = Vertices.Num();
        Vertices.Add(FVector(ShaftEnd, -HalfHeadWidth, 0.0f));
        Vertices.Add(FVector(HeadEnd, 0.0f, 0.0f));
        Vertices.Add(FVector(ShaftEnd, HalfHeadWidth, 0.0f));
        for (auto i = 0; i < 3; ++i) { Normals.Add(FVector::UpVector); }
        UVs.Add(FVector2D(0, 0)); UVs.Add(FVector2D(0.5f, 1)); UVs.Add(FVector2D(1, 0));
        Triangles.Add(BaseIdx + 0); Triangles.Add(BaseIdx + 1); Triangles.Add(BaseIdx + 2);

        // Arrow head back face (triangle)
        BaseIdx = Vertices.Num();
        Vertices.Add(FVector(ShaftEnd, -HalfHeadWidth, 0.0f));
        Vertices.Add(FVector(HeadEnd, 0.0f, 0.0f));
        Vertices.Add(FVector(ShaftEnd, HalfHeadWidth, 0.0f));
        for (auto i = 0; i < 3; ++i) { Normals.Add(FVector::DownVector); }
        UVs.Add(FVector2D(0, 0)); UVs.Add(FVector2D(0.5f, 1)); UVs.Add(FVector2D(1, 0));
        Triangles.Add(BaseIdx + 0); Triangles.Add(BaseIdx + 2); Triangles.Add(BaseIdx + 1);

        UCk_Utils_Vector3_UE::ApplyPlaneAxisRotation(Vertices, Normals, InAxis);

        InMeshComponent->CreateMeshSection_LinearColor(
            0, Vertices, Triangles, Normals, UVs,
            TArray<FLinearColor>{}, TArray<FProcMeshTangent>{}, true);
    }

    // Pivot now uses Arrow entities - no custom geometry needed

    // --------------------------------------------------------------------------------------------------------------------

    // DashedLine now uses Plane entities - no custom geometry needed
}

// --------------------------------------------------------------------------------------------------------------------
// Common Setup Helpers
// --------------------------------------------------------------------------------------------------------------------

namespace
{
    auto SetupMeshComponent_Directional(
        FCk_Handle_Pmg_DebugShape InHandle,
        const ck::FFragment_Pmg_DebugShape_Common& InCommon,
        ck::FFragment_Pmg_DebugShape_Current& InCurrent,
        float InDeltaT)
        -> UProceduralMeshComponent*
    {
        InHandle.Remove<ck::FTag_Pmg_DebugShape_NeedsSetup>();

        const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
        if (ck::Is_NOT_Valid(World))
        { return nullptr; }

        auto MeshComponent = NewObject<UProceduralMeshComponent>(
            World, UProceduralMeshComponent::StaticClass(), NAME_None, RF_Transient);

        if (ck::Is_NOT_Valid(MeshComponent))
        { return nullptr; }

        MeshComponent->SetWorldLocation(FVector::ZeroVector);
        MeshComponent->RegisterComponentWithWorld(World);

        if (MeshComponent->HasBegunPlay() == false)
        {
            MeshComponent->BeginPlay();
        }

        MeshComponent->SetVisibility(true);
        MeshComponent->SetHiddenInGame(false);
        MeshComponent->SetCastShadow(false);

        return MeshComponent;
    }

    auto FinalizeMeshComponent_Directional(
        UProceduralMeshComponent* InMeshComponent,
        FCk_Handle_Pmg_DebugShape InHandle,
        const ck::FFragment_Pmg_DebugShape_Common& InCommon,
        ck::FFragment_Pmg_DebugShape_Current& InCurrent,
        float InDeltaT)
        -> void
    {
        const auto ShouldBeVisible = InCommon.Get_RenderMode() != ECk_Pmg_RenderMode::Hidden;
        InMeshComponent->SetVisibility(ShouldBeVisible, true);
        InMeshComponent->SetHiddenInGame(!ShouldBeVisible);

        InMeshComponent->SetCollisionEnabled(
            InCommon.Get_EnableCollision() ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);

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

        InMeshComponent->UpdateBounds();
        InMeshComponent->MarkRenderStateDirty();

        InCurrent = ck::FFragment_Pmg_DebugShape_Current{TStrongObjectPtr{InMeshComponent}, FCk_Time{InDeltaT}};

        if (InHandle.Has<ck::FFragment_Transform>())
        {
            const auto& Transform = InHandle.Get<ck::FFragment_Transform>();
            InMeshComponent->SetWorldTransform(Transform.Get_Transform());
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
// Processor Implementations
// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto FProcessor_Pmg_Arrow_Setup::ForEachEntity(
        TimeType InDeltaT, HandleType InHandle,
        const FFragment_Pmg_Arrow_Params& InParams,
        const FFragment_Pmg_DebugShape_Common& InCommon,
        FFragment_Pmg_DebugShape_Current& InCurrent) -> void
    {
        auto MeshComponent = SetupMeshComponent_Directional(InHandle, InCommon, InCurrent, InDeltaT.Get_Seconds());
        if (ck::Is_NOT_Valid(MeshComponent)) { return; }

        GenerateDebugShape_Arrow(MeshComponent, InParams.Get_Length(), InParams.Get_ShaftWidth(),
                                 InParams.Get_ArrowHeadRatio(), InParams.Get_ArrowHeadWidthMultiplier(), InParams.Get_Axis());
        FinalizeMeshComponent_Directional(MeshComponent, InHandle, InCommon, InCurrent, InDeltaT.Get_Seconds());

        if (InCommon.Get_DrawLines())
        {
            const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
            if (ck::Is_NOT_Valid(World)) { return; }

            if (InHandle.Has<ck::FFragment_Transform>())
            {
                const auto& Transform = InHandle.Get<ck::FFragment_Transform>();
                const auto Center = Transform.Get_Transform().GetLocation();
                const auto Rotation = Transform.Get_Transform().GetRotation();
                auto LineColor = InCommon.Get_Color();
                LineColor.A = 1.0f;

                const auto ShaftLength = InParams.Get_Length() * (1.0f - InParams.Get_ArrowHeadRatio());
                const auto HalfShaftWidth = InParams.Get_ShaftWidth() * 0.5f;
                const auto HalfHeadWidth = (InParams.Get_ShaftWidth() * InParams.Get_ArrowHeadWidthMultiplier()) * 0.5f;

                auto AxisRotation = FQuat::Identity;
                switch (InParams.Get_Axis())
                {
                    case ECk_Plane_Axis::XY: AxisRotation = FQuat::Identity; break;
                    case ECk_Plane_Axis::XZ: AxisRotation = FQuat(FVector::ForwardVector, PI * 0.5f); break;
                    case ECk_Plane_Axis::YZ: AxisRotation = FQuat(FVector::RightVector, -PI * 0.5f); break;
                }

                const auto FinalRotation = Rotation * AxisRotation;

                // Shaft outline (4 edges of rectangle)
                const auto P0 = Center + FinalRotation.RotateVector(FVector(0.0f, -HalfShaftWidth, 0.0f));
                const auto P1 = Center + FinalRotation.RotateVector(FVector(ShaftLength, -HalfShaftWidth, 0.0f));
                const auto P2 = Center + FinalRotation.RotateVector(FVector(ShaftLength, HalfShaftWidth, 0.0f));
                const auto P3 = Center + FinalRotation.RotateVector(FVector(0.0f, HalfShaftWidth, 0.0f));

                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, P0, P1, LineColor,
                    InCommon.Get_Duration().Get_Seconds(), InCommon.Get_LineThickness());
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, P1, P2, LineColor,
                    InCommon.Get_Duration().Get_Seconds(), InCommon.Get_LineThickness());
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, P2, P3, LineColor,
                    InCommon.Get_Duration().Get_Seconds(), InCommon.Get_LineThickness());
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, P3, P0, LineColor,
                    InCommon.Get_Duration().Get_Seconds(), InCommon.Get_LineThickness());

                // Arrow head outline (3 edges of triangle)
                const auto H0 = Center + FinalRotation.RotateVector(FVector(ShaftLength, -HalfHeadWidth, 0.0f));
                const auto H1 = Center + FinalRotation.RotateVector(FVector(InParams.Get_Length(), 0.0f, 0.0f));
                const auto H2 = Center + FinalRotation.RotateVector(FVector(ShaftLength, HalfHeadWidth, 0.0f));

                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, H0, H1, LineColor,
                    InCommon.Get_Duration().Get_Seconds(), InCommon.Get_LineThickness());
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, H1, H2, LineColor,
                    InCommon.Get_Duration().Get_Seconds(), InCommon.Get_LineThickness());
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, H2, H0, LineColor,
                    InCommon.Get_Duration().Get_Seconds(), InCommon.Get_LineThickness());
            }
        }
    }

    auto FProcessor_Pmg_Pivot_Setup::ForEachEntity(
        TimeType InDeltaT, HandleType InHandle,
        const FFragment_Pmg_Pivot_Params& InParams,
        const FFragment_Pmg_DebugShape_Common& InCommon,
        FFragment_Pmg_DebugShape_Current& InCurrent) -> void
    {
        InHandle.Remove<ck::FTag_Pmg_DebugShape_NeedsSetup>();

        if (InHandle.Has<ck::FFragment_Transform>() == false) { return; }

        const auto& Transform = InHandle.Get<ck::FFragment_Transform>();
        const auto Center = Transform.Get_Transform().GetLocation();
        const auto Rotation = Transform.Get_Transform().GetRotation();

        const auto ShaftWidth = InParams.Get_ArrowSize() * 0.1f;
        const auto ArrowHeadRatio = 0.2f;
        const auto ArrowHeadWidthMultiplier = 5.0f;

        auto AxisRotation = FQuat::Identity;
        switch (InParams.Get_Axis())
        {
            case ECk_Plane_Axis::XY: AxisRotation = FQuat::Identity; break;
            case ECk_Plane_Axis::XZ: AxisRotation = FQuat(FVector::ForwardVector, PI * 0.5f); break;
            case ECk_Plane_Axis::YZ: AxisRotation = FQuat(FVector::RightVector, -PI * 0.5f); break;
        }

        const auto FinalRotation = Rotation * AxisRotation;

        // Helper to spawn an arrow axis
        auto SpawnAxisArrow = [&](const FVector& InDirection, const FLinearColor& InColor)
        {
            const auto AxisDirection = FinalRotation.RotateVector(InDirection);
            const auto AxisRotator = AxisDirection.Rotation();
            const auto AxisTransform = FTransform{AxisRotator, Center, FVector::OneVector};

            auto NewArrow = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InHandle);
            UCk_Utils_Pmg_DirectionalShapes::Add_Arrow(
                NewArrow, AxisTransform,
                InParams.Get_AxisLength(), ShaftWidth,
                ArrowHeadRatio, ArrowHeadWidthMultiplier,
                InColor, true, InCommon.Get_LineThickness(),
                ECk_Plane_Axis::XY, InCommon.Get_Duration().Get_Seconds());
        };

        // Spawn three axes with RGB colors
        SpawnAxisArrow(FVector::ForwardVector, FLinearColor(1.0f, 0.0f, 0.0f, 0.5f));  // X - Red
        SpawnAxisArrow(FVector::RightVector, FLinearColor(0.0f, 1.0f, 0.0f, 0.5f));    // Y - Green
        SpawnAxisArrow(FVector::UpVector, FLinearColor(0.0f, 0.0f, 1.0f, 0.5f));       // Z - Blue
    }

    auto FProcessor_Pmg_DashedLine_Setup::ForEachEntity(
        TimeType InDeltaT, HandleType InHandle,
        const FFragment_Pmg_DashedLine_Params& InParams,
        const FFragment_Pmg_DebugShape_Common& InCommon,
        FFragment_Pmg_DebugShape_Current& InCurrent) -> void
    {
        InHandle.Remove<ck::FTag_Pmg_DebugShape_NeedsSetup>();

        if (InHandle.Has<ck::FFragment_Transform>() == false) { return; }

        const auto& Transform = InHandle.Get<ck::FFragment_Transform>();
        const auto Start = Transform.Get_Transform().GetLocation();
        const auto Rotation = Transform.Get_Transform().GetRotation();

        auto AxisRotation = FQuat::Identity;
        switch (InParams.Get_Axis())
        {
            case ECk_Plane_Axis::XY: AxisRotation = FQuat::Identity; break;
            case ECk_Plane_Axis::XZ: AxisRotation = FQuat(FVector::ForwardVector, PI * 0.5f); break;
            case ECk_Plane_Axis::YZ: AxisRotation = FQuat(FVector::RightVector, -PI * 0.5f); break;
        }

        const auto FinalRotation = Rotation * AxisRotation;
        const auto SegmentLength = InParams.Get_DashLength() + InParams.Get_GapLength();
        auto CurrentPos = 0.0f;

        while (CurrentPos < InParams.Get_Length())
        {
            const auto DashEnd = FMath::Min(CurrentPos + InParams.Get_DashLength(), InParams.Get_Length());
            const auto DashActualLength = DashEnd - CurrentPos;

            if (DashActualLength > 0.0f)
            {
                // Position at center of dash segment
                const auto DashCenter = (CurrentPos + DashEnd) * 0.5f;
                const auto DashLocalPos = FVector(DashCenter, 0.0f, 0.0f);
                const auto DashWorldPos = Start + FinalRotation.RotateVector(DashLocalPos);
                const auto DashTransform = FTransform{Rotation * AxisRotation, DashWorldPos, FVector::OneVector};

                // Spawn plane for this dash
                auto NewPlane = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InHandle);
                UCk_Utils_Pmg_FlatShapes::Add_Plane(
                    NewPlane, DashTransform,
                    DashActualLength, InParams.Get_Thickness(),
                    InCommon.Get_Color(), true, InCommon.Get_LineThickness(),
                    InParams.Get_Axis(), InCommon.Get_Duration().Get_Seconds());
            }

            CurrentPos += SegmentLength;
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
