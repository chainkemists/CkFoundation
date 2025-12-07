#include "CkPmg_Processor_AngularShapes.h"

#include "CkCore/Debug/CkDebugDraw_Utils.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

#include "CkPmg/CkPmg_Log.h"

#include <MaterialDomain.h>
#include <ProceduralMeshComponent.h>

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
// Mesh Component Setup Helpers
// --------------------------------------------------------------------------------------------------------------------

namespace
{
    auto SetupMeshComponent(
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

    auto FinalizeMeshComponent(
        UProceduralMeshComponent* InMeshComponent,
        const FCk_Handle& InHandle,
        const ck::FFragment_Pmg_DebugShape_Common& InCommon,
        ck::FFragment_Pmg_DebugShape_Current& InCurrent,
        float InDeltaSeconds)
        -> void
    {
        CK_ENSURE_IF_NOT(ck::IsValid(InMeshComponent), TEXT("MeshComponent is invalid"))
        { return; }

        InMeshComponent->SetVisibility(true);
        InMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

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

        if (InHandle.Has<ck::FFragment_Transform>())
        {
            const auto& Transform = InHandle.Get<ck::FFragment_Transform>();
            InMeshComponent->SetWorldTransform(Transform.Get_Transform());
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
// Shape Generation Functions
// --------------------------------------------------------------------------------------------------------------------

namespace
{
    auto GenerateDebugShape_Wedge(
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
        const auto AngleRange = EndRad - StartRad;

        Vertices.Add(FVector::ZeroVector);
        Normals.Add(FVector::UpVector);
        UVs.Add(FVector2D(0.5f, 0.5f));

        for (auto i = 0; i <= InSegments; ++i)
        {
            const auto Angle = StartRad + AngleRange * i / InSegments;
            const auto X = InRadius * FMath::Cos(Angle);
            const auto Y = InRadius * FMath::Sin(Angle);

            Vertices.Add(FVector(X, Y, 0.0f));
            Normals.Add(FVector::UpVector);
            UVs.Add(FVector2D(0.5f + 0.5f * FMath::Cos(Angle), 0.5f + 0.5f * FMath::Sin(Angle)));
        }

        for (auto i = 0; i < InSegments; ++i)
        {
            Triangles.Add(0);
            Triangles.Add(i + 1);
            Triangles.Add(i + 2);
        }

        const auto BottomCenterIndex = Vertices.Num();
        Vertices.Add(FVector::ZeroVector);
        Normals.Add(FVector::DownVector);
        UVs.Add(FVector2D(0.5f, 0.5f));

        for (auto i = 0; i <= InSegments; ++i)
        {
            const auto Angle = StartRad + AngleRange * i / InSegments;
            const auto X = InRadius * FMath::Cos(Angle);
            const auto Y = InRadius * FMath::Sin(Angle);

            Vertices.Add(FVector(X, Y, 0.0f));
            Normals.Add(FVector::DownVector);
            UVs.Add(FVector2D(0.5f + 0.5f * FMath::Cos(Angle), 0.5f + 0.5f * FMath::Sin(Angle)));
        }

        for (auto i = 0; i < InSegments; ++i)
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

    // --------------------------------------------------------------------------------------------------------------------

    auto GenerateDebugShape_Arc(
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
        const auto AngleRange = EndRad - StartRad;
        const auto InnerRadius = InRadius - InThickness * 0.5f;
        const auto OuterRadius = InRadius + InThickness * 0.5f;

        for (auto i = 0; i <= InSegments; ++i)
        {
            const auto Angle = StartRad + AngleRange * i / InSegments;
            const auto CosAngle = FMath::Cos(Angle);
            const auto SinAngle = FMath::Sin(Angle);

            const auto InnerX = InnerRadius * CosAngle;
            const auto InnerY = InnerRadius * SinAngle;
            const auto OuterX = OuterRadius * CosAngle;
            const auto OuterY = OuterRadius * SinAngle;

            Vertices.Add(FVector(InnerX, InnerY, 0.0f));
            Vertices.Add(FVector(OuterX, OuterY, 0.0f));

            Normals.Add(FVector::UpVector);
            Normals.Add(FVector::UpVector);

            const auto U = static_cast<float>(i) / InSegments;
            UVs.Add(FVector2D(U, 0.0f));
            UVs.Add(FVector2D(U, 1.0f));
        }

        for (auto i = 0; i < InSegments; ++i)
        {
            const auto BaseIndex = i * 2;

            Triangles.Add(BaseIndex + 0);
            Triangles.Add(BaseIndex + 1);
            Triangles.Add(BaseIndex + 2);

            Triangles.Add(BaseIndex + 1);
            Triangles.Add(BaseIndex + 3);
            Triangles.Add(BaseIndex + 2);
        }

        const auto BottomStartIndex = Vertices.Num();
        for (auto i = 0; i <= InSegments; ++i)
        {
            const auto Angle = StartRad + AngleRange * i / InSegments;
            const auto CosAngle = FMath::Cos(Angle);
            const auto SinAngle = FMath::Sin(Angle);

            const auto InnerX = InnerRadius * CosAngle;
            const auto InnerY = InnerRadius * SinAngle;
            const auto OuterX = OuterRadius * CosAngle;
            const auto OuterY = OuterRadius * SinAngle;

            Vertices.Add(FVector(InnerX, InnerY, 0.0f));
            Vertices.Add(FVector(OuterX, OuterY, 0.0f));

            Normals.Add(FVector::DownVector);
            Normals.Add(FVector::DownVector);

            const auto U = static_cast<float>(i) / InSegments;
            UVs.Add(FVector2D(U, 0.0f));
            UVs.Add(FVector2D(U, 1.0f));
        }

        for (auto i = 0; i < InSegments; ++i)
        {
            const auto BaseIndex = BottomStartIndex + i * 2;

            Triangles.Add(BaseIndex + 0);
            Triangles.Add(BaseIndex + 2);
            Triangles.Add(BaseIndex + 1);

            Triangles.Add(BaseIndex + 1);
            Triangles.Add(BaseIndex + 2);
            Triangles.Add(BaseIndex + 3);
        }

        ApplyAxisRotation(Vertices, Normals, InAxis);

        InMeshComponent->CreateMeshSection_LinearColor(
            0, Vertices, Triangles, Normals, UVs,
            TArray<FLinearColor>{}, TArray<FProcMeshTangent>{}, true);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto GenerateDebugShape_WedgeCone(
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
        const auto AngleRange = EndRad - StartRad;

        const auto Apex = FVector(0, 0, InHeight);
        Vertices.Add(Apex);
        Normals.Add(FVector::UpVector);
        UVs.Add(FVector2D(0.5f, 1.0f));

        for (auto i = 0; i <= InSegments; ++i)
        {
            const auto Angle = StartRad + AngleRange * i / InSegments;
            const auto X = InRadius * FMath::Cos(Angle);
            const auto Y = InRadius * FMath::Sin(Angle);

            Vertices.Add(FVector(X, Y, 0.0f));

            const auto SideNormal = (FVector(X, Y, InHeight * 0.5f) - Apex).GetSafeNormal();
            Normals.Add(SideNormal);
            UVs.Add(FVector2D(static_cast<float>(i) / InSegments, 0.0f));
        }

        for (auto i = 0; i < InSegments; ++i)
        {
            Triangles.Add(0);
            Triangles.Add(i + 1);
            Triangles.Add(i + 2);
        }

        const auto BaseCenterIndex = Vertices.Num();
        Vertices.Add(FVector::ZeroVector);
        Normals.Add(FVector::DownVector);
        UVs.Add(FVector2D(0.5f, 0.5f));

        for (auto i = 0; i <= InSegments; ++i)
        {
            const auto Angle = StartRad + AngleRange * i / InSegments;
            const auto X = InRadius * FMath::Cos(Angle);
            const auto Y = InRadius * FMath::Sin(Angle);

            Vertices.Add(FVector(X, Y, 0.0f));
            Normals.Add(FVector::DownVector);
            UVs.Add(FVector2D(0.5f + 0.5f * FMath::Cos(Angle), 0.5f + 0.5f * FMath::Sin(Angle)));
        }

        for (auto i = 0; i < InSegments; ++i)
        {
            Triangles.Add(BaseCenterIndex);
            Triangles.Add(BaseCenterIndex + i + 2);
            Triangles.Add(BaseCenterIndex + i + 1);
        }

        if (AngleRange < 2.0f * PI - 0.01f)
        {
            const auto SideStart = Vertices.Num();

            const auto Start1 = FVector(InRadius * FMath::Cos(StartRad), InRadius * FMath::Sin(StartRad), 0.0f);
            const auto Start2 = Apex;
            const auto SideNormalStart = FVector::CrossProduct(Start2 - Start1, FVector::UpVector).GetSafeNormal();

            Vertices.Add(Start1); Normals.Add(SideNormalStart); UVs.Add(FVector2D(0, 0));
            Vertices.Add(Start2); Normals.Add(SideNormalStart); UVs.Add(FVector2D(1, 0));
            Vertices.Add(FVector::ZeroVector); Normals.Add(SideNormalStart); UVs.Add(FVector2D(0, 1));

            Triangles.Add(SideStart + 0);
            Triangles.Add(SideStart + 1);
            Triangles.Add(SideStart + 2);

            const auto End1 = FVector(InRadius * FMath::Cos(EndRad), InRadius * FMath::Sin(EndRad), 0.0f);
            const auto End2 = Apex;
            const auto SideNormalEnd = FVector::CrossProduct(FVector::UpVector, End2 - End1).GetSafeNormal();

            Vertices.Add(End1); Normals.Add(SideNormalEnd); UVs.Add(FVector2D(0, 0));
            Vertices.Add(FVector::ZeroVector); Normals.Add(SideNormalEnd); UVs.Add(FVector2D(0, 1));
            Vertices.Add(End2); Normals.Add(SideNormalEnd); UVs.Add(FVector2D(1, 0));

            Triangles.Add(SideStart + 3);
            Triangles.Add(SideStart + 4);
            Triangles.Add(SideStart + 5);
        }

        ApplyAxisRotation(Vertices, Normals, InAxis);

        InMeshComponent->CreateMeshSection_LinearColor(
            0, Vertices, Triangles, Normals, UVs,
            TArray<FLinearColor>{}, TArray<FProcMeshTangent>{}, true);
    }
}

// --------------------------------------------------------------------------------------------------------------------
// Processor Implementations
// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto FProcessor_Pmg_Wedge_Setup::ForEachEntity(
        TimeType InDeltaT, HandleType InHandle,
        const FFragment_Pmg_Wedge_Params& InParams,
        const FFragment_Pmg_DebugShape_Common& InCommon,
        FFragment_Pmg_DebugShape_Current& InCurrent) -> void
    {
        auto MeshComponent = SetupMeshComponent(InHandle, InCommon, InCurrent, InDeltaT.Get_Seconds());
        if (ck::Is_NOT_Valid(MeshComponent)) { return; }

        GenerateDebugShape_Wedge(MeshComponent, InParams.Get_Radius(), InParams.Get_StartAngle(), InParams.Get_EndAngle(), InParams.Get_Segments(), InParams.Get_Axis());
        FinalizeMeshComponent(MeshComponent, InHandle, InCommon, InCurrent, InDeltaT.Get_Seconds());

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

                const auto StartRad = FMath::DegreesToRadians(InParams.Get_StartAngle());
                const auto EndRad = FMath::DegreesToRadians(InParams.Get_EndAngle());
                const auto AngleRange = EndRad - StartRad;
                const auto Segments = InParams.Get_Segments();
                const auto Radius = InParams.Get_Radius();

                auto AxisRotation = FQuat::Identity;
                switch (InParams.Get_Axis())
                {
                    case ECk_Plane_Axis::XY: AxisRotation = FQuat::Identity; break;
                    case ECk_Plane_Axis::XZ: AxisRotation = FQuat(FVector::ForwardVector, PI * 0.5f); break;
                    case ECk_Plane_Axis::YZ: AxisRotation = FQuat(FVector::RightVector, -PI * 0.5f); break;
                }

                const auto FinalRotation = Rotation * AxisRotation;

                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, Center, Center + FinalRotation.RotateVector(FVector(
                    Radius * FMath::Cos(StartRad), Radius * FMath::Sin(StartRad), 0.0f)), LineColor,
                    InCommon.Get_Duration().Get_Seconds(), InCommon.Get_LineThickness());

                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, Center, Center + FinalRotation.RotateVector(FVector(
                    Radius * FMath::Cos(EndRad), Radius * FMath::Sin(EndRad), 0.0f)), LineColor,
                    InCommon.Get_Duration().Get_Seconds(), InCommon.Get_LineThickness());

                for (auto i = 0; i < Segments; ++i)
                {
                    const auto Angle1 = StartRad + AngleRange * i / Segments;
                    const auto Angle2 = StartRad + AngleRange * (i + 1) / Segments;

                    const auto P1 = Center + FinalRotation.RotateVector(FVector(
                        Radius * FMath::Cos(Angle1), Radius * FMath::Sin(Angle1), 0.0f));
                    const auto P2 = Center + FinalRotation.RotateVector(FVector(
                        Radius * FMath::Cos(Angle2), Radius * FMath::Sin(Angle2), 0.0f));

                    UCk_Utils_DebugDraw_UE::DrawDebugLine(World, P1, P2, LineColor,
                        InCommon.Get_Duration().Get_Seconds(), InCommon.Get_LineThickness());
                }
            }
        }
    }

    auto FProcessor_Pmg_Arc_Setup::ForEachEntity(
        TimeType InDeltaT, HandleType InHandle,
        const FFragment_Pmg_Arc_Params& InParams,
        const FFragment_Pmg_DebugShape_Common& InCommon,
        FFragment_Pmg_DebugShape_Current& InCurrent) -> void
    {
        auto MeshComponent = SetupMeshComponent(InHandle, InCommon, InCurrent, InDeltaT.Get_Seconds());
        if (ck::Is_NOT_Valid(MeshComponent)) { return; }

        GenerateDebugShape_Arc(MeshComponent, InParams.Get_Radius(), InParams.Get_StartAngle(), InParams.Get_EndAngle(), InParams.Get_Thickness(), InParams.Get_Segments(), InParams.Get_Axis());
        FinalizeMeshComponent(MeshComponent, InHandle, InCommon, InCurrent, InDeltaT.Get_Seconds());

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

                const auto StartRad = FMath::DegreesToRadians(InParams.Get_StartAngle());
                const auto EndRad = FMath::DegreesToRadians(InParams.Get_EndAngle());
                const auto AngleRange = EndRad - StartRad;
                const auto Segments = InParams.Get_Segments();
                const auto InnerRadius = InParams.Get_Radius() - InParams.Get_Thickness() * 0.5f;
                const auto OuterRadius = InParams.Get_Radius() + InParams.Get_Thickness() * 0.5f;

                auto AxisRotation = FQuat::Identity;
                switch (InParams.Get_Axis())
                {
                    case ECk_Plane_Axis::XY: AxisRotation = FQuat::Identity; break;
                    case ECk_Plane_Axis::XZ: AxisRotation = FQuat(FVector::ForwardVector, PI * 0.5f); break;
                    case ECk_Plane_Axis::YZ: AxisRotation = FQuat(FVector::RightVector, -PI * 0.5f); break;
                }

                const auto FinalRotation = Rotation * AxisRotation;

                for (auto i = 0; i < Segments; ++i)
                {
                    const auto Angle1 = StartRad + AngleRange * i / Segments;
                    const auto Angle2 = StartRad + AngleRange * (i + 1) / Segments;

                    const auto InnerP1 = Center + FinalRotation.RotateVector(FVector(
                        InnerRadius * FMath::Cos(Angle1), InnerRadius * FMath::Sin(Angle1), 0.0f));
                    const auto InnerP2 = Center + FinalRotation.RotateVector(FVector(
                        InnerRadius * FMath::Cos(Angle2), InnerRadius * FMath::Sin(Angle2), 0.0f));

                    const auto OuterP1 = Center + FinalRotation.RotateVector(FVector(
                        OuterRadius * FMath::Cos(Angle1), OuterRadius * FMath::Sin(Angle1), 0.0f));
                    const auto OuterP2 = Center + FinalRotation.RotateVector(FVector(
                        OuterRadius * FMath::Cos(Angle2), OuterRadius * FMath::Sin(Angle2), 0.0f));

                    UCk_Utils_DebugDraw_UE::DrawDebugLine(World, InnerP1, InnerP2, LineColor,
                        InCommon.Get_Duration().Get_Seconds(), InCommon.Get_LineThickness());
                    UCk_Utils_DebugDraw_UE::DrawDebugLine(World, OuterP1, OuterP2, LineColor,
                        InCommon.Get_Duration().Get_Seconds(), InCommon.Get_LineThickness());
                }

                const auto StartInner = Center + FinalRotation.RotateVector(FVector(
                    InnerRadius * FMath::Cos(StartRad), InnerRadius * FMath::Sin(StartRad), 0.0f));
                const auto StartOuter = Center + FinalRotation.RotateVector(FVector(
                    OuterRadius * FMath::Cos(StartRad), OuterRadius * FMath::Sin(StartRad), 0.0f));
                const auto EndInner = Center + FinalRotation.RotateVector(FVector(
                    InnerRadius * FMath::Cos(EndRad), InnerRadius * FMath::Sin(EndRad), 0.0f));
                const auto EndOuter = Center + FinalRotation.RotateVector(FVector(
                    OuterRadius * FMath::Cos(EndRad), OuterRadius * FMath::Sin(EndRad), 0.0f));

                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, StartInner, StartOuter, LineColor,
                    InCommon.Get_Duration().Get_Seconds(), InCommon.Get_LineThickness());
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, EndInner, EndOuter, LineColor,
                    InCommon.Get_Duration().Get_Seconds(), InCommon.Get_LineThickness());
            }
        }
    }

    auto FProcessor_Pmg_WedgeCone_Setup::ForEachEntity(
        TimeType InDeltaT, HandleType InHandle,
        const FFragment_Pmg_WedgeCone_Params& InParams,
        const FFragment_Pmg_DebugShape_Common& InCommon,
        FFragment_Pmg_DebugShape_Current& InCurrent) -> void
    {
        auto MeshComponent = SetupMeshComponent(InHandle, InCommon, InCurrent, InDeltaT.Get_Seconds());
        if (ck::Is_NOT_Valid(MeshComponent)) { return; }

        GenerateDebugShape_WedgeCone(MeshComponent, InParams.Get_Radius(), InParams.Get_Height(), InParams.Get_StartAngle(), InParams.Get_EndAngle(), InParams.Get_Segments(), InParams.Get_Axis());
        FinalizeMeshComponent(MeshComponent, InHandle, InCommon, InCurrent, InDeltaT.Get_Seconds());

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

                const auto StartRad = FMath::DegreesToRadians(InParams.Get_StartAngle());
                const auto EndRad = FMath::DegreesToRadians(InParams.Get_EndAngle());
                const auto AngleRange = EndRad - StartRad;
                const auto Segments = InParams.Get_Segments();
                const auto Radius = InParams.Get_Radius();
                const auto Height = InParams.Get_Height();

                auto AxisRotation = FQuat::Identity;
                switch (InParams.Get_Axis())
                {
                    case ECk_Plane_Axis::XY: AxisRotation = FQuat::Identity; break;
                    case ECk_Plane_Axis::XZ: AxisRotation = FQuat(FVector::ForwardVector, PI * 0.5f); break;
                    case ECk_Plane_Axis::YZ: AxisRotation = FQuat(FVector::RightVector, -PI * 0.5f); break;
                }

                const auto FinalRotation = Rotation * AxisRotation;
                const auto Apex = Center + FinalRotation.RotateVector(FVector(0, 0, Height));

                for (auto i = 0; i < Segments; ++i)
                {
                    const auto Angle1 = StartRad + AngleRange * i / Segments;
                    const auto Angle2 = StartRad + AngleRange * (i + 1) / Segments;

                    const auto BaseP1 = Center + FinalRotation.RotateVector(FVector(
                        Radius * FMath::Cos(Angle1), Radius * FMath::Sin(Angle1), 0.0f));
                    const auto BaseP2 = Center + FinalRotation.RotateVector(FVector(
                        Radius * FMath::Cos(Angle2), Radius * FMath::Sin(Angle2), 0.0f));

                    UCk_Utils_DebugDraw_UE::DrawDebugLine(World, BaseP1, BaseP2, LineColor,
                        InCommon.Get_Duration().Get_Seconds(), InCommon.Get_LineThickness());
                    UCk_Utils_DebugDraw_UE::DrawDebugLine(World, BaseP1, Apex, LineColor,
                        InCommon.Get_Duration().Get_Seconds(), InCommon.Get_LineThickness());
                }

                const auto LastBase = Center + FinalRotation.RotateVector(FVector(
                    Radius * FMath::Cos(EndRad), Radius * FMath::Sin(EndRad), 0.0f));
                UCk_Utils_DebugDraw_UE::DrawDebugLine(World, LastBase, Apex, LineColor,
                    InCommon.Get_Duration().Get_Seconds(), InCommon.Get_LineThickness());

                if (AngleRange < 2.0f * PI - 0.01f)
                {
                    const auto StartBase = Center + FinalRotation.RotateVector(FVector(
                        Radius * FMath::Cos(StartRad), Radius * FMath::Sin(StartRad), 0.0f));
                    const auto EndBase = Center + FinalRotation.RotateVector(FVector(
                        Radius * FMath::Cos(EndRad), Radius * FMath::Sin(EndRad), 0.0f));

                    UCk_Utils_DebugDraw_UE::DrawDebugLine(World, Center, StartBase, LineColor,
                        InCommon.Get_Duration().Get_Seconds(), InCommon.Get_LineThickness());
                    UCk_Utils_DebugDraw_UE::DrawDebugLine(World, Center, EndBase, LineColor,
                        InCommon.Get_Duration().Get_Seconds(), InCommon.Get_LineThickness());
                    UCk_Utils_DebugDraw_UE::DrawDebugLine(World, StartBase, Apex, LineColor,
                        InCommon.Get_Duration().Get_Seconds(), InCommon.Get_LineThickness());
                    UCk_Utils_DebugDraw_UE::DrawDebugLine(World, EndBase, Apex, LineColor,
                        InCommon.Get_Duration().Get_Seconds(), InCommon.Get_LineThickness());
                }
            }
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
