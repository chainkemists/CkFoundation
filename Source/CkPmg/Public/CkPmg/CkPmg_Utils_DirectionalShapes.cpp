#include "CkPmg_Utils_DirectionalShapes.h"
#include "CkPmg_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Pmg_DirectionalShapes::
    Add_Arrow(
        FCk_Handle& InHandle,
        FTransform InTransform,
        float InLength,
        float InShaftWidth,
        float InArrowHeadRatio,
        float InArrowHeadWidthMultiplier,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Arrow);
    Params.Set_Size(InLength);
    Params.Set_SecondarySize(InShaftWidth);
    Params.Set_ArrowHeadRatio(InArrowHeadRatio);
    Params.Set_ArrowHeadWidthMultiplier(InArrowHeadWidthMultiplier);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    return UCk_Utils_Pmg_DebugShape_UE::Add(InHandle, Params, InTransform);
}

auto
    UCk_Utils_Pmg_DirectionalShapes::
    Add_Pivot(
        FCk_Handle& InHandle,
        FTransform InTransform,
        float InAxisLength,
        float InArrowSize,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Pivot);
    Params.Set_Size(InAxisLength);
    Params.Set_SecondarySize(InArrowSize);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    return UCk_Utils_Pmg_DebugShape_UE::Add(InHandle, Params, InTransform);
}

auto
    UCk_Utils_Pmg_DirectionalShapes::
    Add_DashedLine(
        FCk_Handle& InHandle,
        FTransform InTransform,
        float InLength,
        float InDashLength,
        float InGapLength,
        float InThickness,
        FLinearColor InColor,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::DashedLine);
    Params.Set_Size(InLength);
    Params.Set_SecondarySize(InThickness);
    Params.Set_DashLength(InDashLength);
    Params.Set_GapLength(InGapLength);
    Params.Set_Color(InColor);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    return UCk_Utils_Pmg_DebugShape_UE::Add(InHandle, Params, InTransform);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Pmg_DirectionalShapes::
    Create_Arrow(
        FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InLength,
        float InShaftWidth,
        float InArrowHeadRatio,
        float InArrowHeadWidthMultiplier,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Arrow);
    Params.Set_Size(InLength);
    Params.Set_SecondarySize(InShaftWidth);
    Params.Set_ArrowHeadRatio(InArrowHeadRatio);
    Params.Set_ArrowHeadWidthMultiplier(InArrowHeadWidthMultiplier);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    return UCk_Utils_Pmg_DebugShape_UE::Create(InOwningEntity, Params, InTransform);
}

auto
    UCk_Utils_Pmg_DirectionalShapes::
    Create_Pivot(
        FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InAxisLength,
        float InArrowSize,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Pivot);
    Params.Set_Size(InAxisLength);
    Params.Set_SecondarySize(InArrowSize);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    return UCk_Utils_Pmg_DebugShape_UE::Create(InOwningEntity, Params, InTransform);
}

auto
    UCk_Utils_Pmg_DirectionalShapes::
    Create_DashedLine(
        FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InLength,
        float InDashLength,
        float InGapLength,
        float InThickness,
        FLinearColor InColor,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::DashedLine);
    Params.Set_Size(InLength);
    Params.Set_SecondarySize(InThickness);
    Params.Set_DashLength(InDashLength);
    Params.Set_GapLength(InGapLength);
    Params.Set_Color(InColor);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    return UCk_Utils_Pmg_DebugShape_UE::Create(InOwningEntity, Params, InTransform);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Pmg_DirectionalShapes::
    DrawFilledArrow(
        const UObject* InWorldContextObject,
        FVector InOrigin,
        FRotator InDirection,
        float InLength,
        float InShaftWidth,
        float InArrowHeadRatio,
        float InArrowHeadWidthMultiplier,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Arrow);
    Params.Set_Size(InLength);
    Params.Set_SecondarySize(InShaftWidth);
    Params.Set_ArrowHeadRatio(InArrowHeadRatio);
    Params.Set_ArrowHeadWidthMultiplier(InArrowHeadWidthMultiplier);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    const auto Transform = FTransform(InDirection, InOrigin, FVector::OneVector);
    return UCk_Utils_Pmg_DebugShape_UE::Create_TransientOwner(InWorldContextObject, Params, Transform);
}

auto
    UCk_Utils_Pmg_DirectionalShapes::
    DrawPivot(
        const UObject* InWorldContextObject,
        FVector InOrigin,
        FRotator InRotation,
        float InAxisLength,
        float InArrowSize,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Pivot);
    Params.Set_Size(InAxisLength);
    Params.Set_SecondarySize(InArrowSize);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    const auto Transform = FTransform(InRotation, InOrigin, FVector::OneVector);
    return UCk_Utils_Pmg_DebugShape_UE::Create_TransientOwner(InWorldContextObject, Params, Transform);
}

auto
    UCk_Utils_Pmg_DirectionalShapes::
    DrawDashedLine(
        const UObject* InWorldContextObject,
        FVector InStart,
        FVector InEnd,
        float InDashLength,
        float InGapLength,
        float InThickness,
        FLinearColor InColor,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    const auto Direction = InEnd - InStart;
    const auto Distance = Direction.Size();
    const auto Rotation = Direction.Rotation();

    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::DashedLine);
    Params.Set_Size(Distance);
    Params.Set_SecondarySize(InThickness);
    Params.Set_DashLength(InDashLength);
    Params.Set_GapLength(InGapLength);
    Params.Set_Color(InColor);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    const auto Transform = FTransform(Rotation, InStart, FVector::OneVector);
    return UCk_Utils_Pmg_DebugShape_UE::Create_TransientOwner(InWorldContextObject, Params, Transform);
}

// --------------------------------------------------------------------------------------------------------------------
