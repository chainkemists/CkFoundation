#include "CkPmg_Utils_IconShapes.h"
#include "CkPmg_Utils.h"

#include <ProceduralMeshComponent.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::pmg
{
    auto
        GenerateDebugShape_Warning(
            UProceduralMeshComponent* InMeshComponent,
            float InSize)
        -> void
    {
        auto Vertices = TArray<FVector>{};
        auto Triangles = TArray<int32>{};
        auto Normals = TArray<FVector>{};
        auto UVs = TArray<FVector2D>{};

        const auto HalfSize = InSize * 0.5f;
        const auto Height = InSize * 0.866f;

        const auto Top = FVector(0.0f, 0.0f, Height * 0.667f);
        const auto BottomLeft = FVector(-HalfSize, 0.0f, -Height * 0.333f);
        const auto BottomRight = FVector(HalfSize, 0.0f, -Height * 0.333f);

        Vertices.Add(Top);
        Vertices.Add(BottomRight);
        Vertices.Add(BottomLeft);
        Normals.Add(FVector::ForwardVector);
        Normals.Add(FVector::ForwardVector);
        Normals.Add(FVector::ForwardVector);
        UVs.Add(FVector2D(0.5f, 1.0f));
        UVs.Add(FVector2D(1.0f, 0.0f));
        UVs.Add(FVector2D(0.0f, 0.0f));
        Triangles.Add(0); Triangles.Add(1); Triangles.Add(2);

        const auto BottomStart = Vertices.Num();
        Vertices.Add(Top);
        Vertices.Add(BottomRight);
        Vertices.Add(BottomLeft);
        Normals.Add(FVector::BackwardVector);
        Normals.Add(FVector::BackwardVector);
        Normals.Add(FVector::BackwardVector);
        UVs.Add(FVector2D(0.5f, 1.0f));
        UVs.Add(FVector2D(1.0f, 0.0f));
        UVs.Add(FVector2D(0.0f, 0.0f));
        Triangles.Add(BottomStart + 0); Triangles.Add(BottomStart + 2); Triangles.Add(BottomStart + 1);

        const auto BarWidth = InSize * 0.1f;
        const auto BarHeight = InSize * 0.4f;
        const auto DotSize = InSize * 0.08f;
        const auto BarBottom = -Height * 0.15f;

        auto BarIdx = Vertices.Num();
        Vertices.Add(FVector(-BarWidth, 0.0f, BarBottom));
        Vertices.Add(FVector(BarWidth, 0.0f, BarBottom));
        Vertices.Add(FVector(BarWidth, 0.0f, BarBottom + BarHeight));
        Vertices.Add(FVector(-BarWidth, 0.0f, BarBottom + BarHeight));
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

        InMeshComponent->CreateMeshSection_LinearColor(
            0, Vertices, Triangles, Normals, UVs,
            TArray<FLinearColor>{}, TArray<FProcMeshTangent>{}, true);
    }

    auto
        GenerateDebugShape_Prohibition(
            UProceduralMeshComponent* InMeshComponent,
            float InRadius,
            int32 InSegments)
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

        InMeshComponent->CreateMeshSection_LinearColor(
            0, Vertices, Triangles, Normals, UVs,
            TArray<FLinearColor>{}, TArray<FProcMeshTangent>{}, true);
    }

    auto
        GenerateDebugShape_NoEntry(
            UProceduralMeshComponent* InMeshComponent,
            float InRadius,
            int32 InSegments)
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

        InMeshComponent->CreateMeshSection_LinearColor(
            0, Vertices, Triangles, Normals, UVs,
            TArray<FLinearColor>{}, TArray<FProcMeshTangent>{}, true);
    }

    auto
        GenerateDebugShape_InfoCircle(
            UProceduralMeshComponent* InMeshComponent,
            float InRadius,
            int32 InSegments)
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

        InMeshComponent->CreateMeshSection_LinearColor(
            0, Vertices, Triangles, Normals, UVs,
            TArray<FLinearColor>{}, TArray<FProcMeshTangent>{}, true);
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Pmg_IconShapes::
    Add_Warning(
        FCk_Handle& InHandle,
        FTransform InTransform,
        float InSize,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Warning);
    Params.Set_Size(InSize);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    return UCk_Utils_Pmg_DebugShape_UE::Add(InHandle, Params, InTransform);
}

auto
    UCk_Utils_Pmg_IconShapes::
    Add_Prohibition(
        FCk_Handle& InHandle,
        FTransform InTransform,
        float InRadius,
        int32 InSegments,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Prohibition);
    Params.Set_Size(InRadius);
    Params.Set_Segments(InSegments);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    return UCk_Utils_Pmg_DebugShape_UE::Add(InHandle, Params, InTransform);
}

auto
    UCk_Utils_Pmg_IconShapes::
    Add_NoEntry(
        FCk_Handle& InHandle,
        FTransform InTransform,
        float InRadius,
        int32 InSegments,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::NoEntry);
    Params.Set_Size(InRadius);
    Params.Set_Segments(InSegments);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    return UCk_Utils_Pmg_DebugShape_UE::Add(InHandle, Params, InTransform);
}

auto
    UCk_Utils_Pmg_IconShapes::
    Add_InfoCircle(
        FCk_Handle& InHandle,
        FTransform InTransform,
        float InRadius,
        int32 InSegments,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::InfoCircle);
    Params.Set_Size(InRadius);
    Params.Set_Segments(InSegments);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    return UCk_Utils_Pmg_DebugShape_UE::Add(InHandle, Params, InTransform);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Pmg_IconShapes::
    Create_Warning(
        FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InSize,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Warning);
    Params.Set_Size(InSize);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    return UCk_Utils_Pmg_DebugShape_UE::Create(InOwningEntity, Params, InTransform);
}

auto
    UCk_Utils_Pmg_IconShapes::
    Create_Prohibition(
        FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InRadius,
        int32 InSegments,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Prohibition);
    Params.Set_Size(InRadius);
    Params.Set_Segments(InSegments);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    return UCk_Utils_Pmg_DebugShape_UE::Create(InOwningEntity, Params, InTransform);
}

auto
    UCk_Utils_Pmg_IconShapes::
    Create_NoEntry(
        FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InRadius,
        int32 InSegments,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::NoEntry);
    Params.Set_Size(InRadius);
    Params.Set_Segments(InSegments);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    return UCk_Utils_Pmg_DebugShape_UE::Create(InOwningEntity, Params, InTransform);
}

auto
    UCk_Utils_Pmg_IconShapes::
    Create_InfoCircle(
        FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InRadius,
        int32 InSegments,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::InfoCircle);
    Params.Set_Size(InRadius);
    Params.Set_Segments(InSegments);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    return UCk_Utils_Pmg_DebugShape_UE::Create(InOwningEntity, Params, InTransform);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Pmg_IconShapes::
    DrawFilledWarning(
        const UObject* InWorldContextObject,
        FVector InCenter,
        float InSize,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Warning);
    Params.Set_Size(InSize);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    const auto Transform = FTransform(FRotator::ZeroRotator, InCenter, FVector::OneVector);
    return UCk_Utils_Pmg_DebugShape_UE::Create_TransientOwner(InWorldContextObject, Params, Transform);
}

auto
    UCk_Utils_Pmg_IconShapes::
    DrawFilledProhibition(
        const UObject* InWorldContextObject,
        FVector InCenter,
        float InRadius,
        int32 InSegments,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Prohibition);
    Params.Set_Size(InRadius);
    Params.Set_Segments(InSegments);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    const auto Transform = FTransform(FRotator::ZeroRotator, InCenter, FVector::OneVector);
    return UCk_Utils_Pmg_DebugShape_UE::Create_TransientOwner(InWorldContextObject, Params, Transform);
}

auto
    UCk_Utils_Pmg_IconShapes::
    DrawFilledNoEntry(
        const UObject* InWorldContextObject,
        FVector InCenter,
        float InRadius,
        int32 InSegments,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::NoEntry);
    Params.Set_Size(InRadius);
    Params.Set_Segments(InSegments);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    const auto Transform = FTransform(FRotator::ZeroRotator, InCenter, FVector::OneVector);
    return UCk_Utils_Pmg_DebugShape_UE::Create_TransientOwner(InWorldContextObject, Params, Transform);
}

auto
    UCk_Utils_Pmg_IconShapes::
    DrawFilledInfoCircle(
        const UObject* InWorldContextObject,
        FVector InCenter,
        float InRadius,
        int32 InSegments,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::InfoCircle);
    Params.Set_Size(InRadius);
    Params.Set_Segments(InSegments);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    const auto Transform = FTransform(FRotator::ZeroRotator, InCenter, FVector::OneVector);
    return UCk_Utils_Pmg_DebugShape_UE::Create_TransientOwner(InWorldContextObject, Params, Transform);
}

// --------------------------------------------------------------------------------------------------------------------
