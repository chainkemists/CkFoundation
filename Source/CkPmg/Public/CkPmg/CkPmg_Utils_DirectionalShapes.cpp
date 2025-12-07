#include "CkPmg_Utils_DirectionalShapes.h"
#include "CkPmg_Utils.h"
#include "CkPmg_Fragment.h"
#include "CkPmg_Fragment_DirectionalShapes.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

// --------------------------------------------------------------------------------------------------------------------
// Arrow
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
    auto Common = ck::FFragment_Pmg_DebugShape_Common{};
    Common.Set_Color(InColor);
    Common.Set_DrawLines(InDrawLines);
    Common.Set_LineThickness(InLineThickness);
    Common.Set_Duration(FCk_Time{InDuration});
    InHandle.Add<ck::FFragment_Pmg_DebugShape_Common>(Common);

    auto Params = ck::FFragment_Pmg_Arrow_Params{};
    Params.Set_Length(InLength);
    Params.Set_ShaftWidth(InShaftWidth);
    Params.Set_ArrowHeadRatio(InArrowHeadRatio);
    Params.Set_ArrowHeadWidthMultiplier(InArrowHeadWidthMultiplier);
    Params.Set_Axis(InDefaultAxis);
    InHandle.Add<ck::FFragment_Pmg_Arrow_Params>(Params);

    InHandle.Add<ck::FFragment_Pmg_DebugShape_Current>();
    InHandle.Add<ck::FTag_Pmg_DebugShape_NeedsSetup>();

    UCk_Utils_Transform_UE::Add(InHandle, InTransform, ECk_Replication::DoesNotReplicate);

    return UCk_Utils_Pmg_DebugShape_UE::Cast(InHandle);
}

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
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InOwningEntity);
    return Add_Arrow(NewEntity, InTransform, InLength, InShaftWidth, InArrowHeadRatio, InArrowHeadWidthMultiplier, InColor, InDrawLines, InLineThickness, InDefaultAxis, InDuration);
}

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
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_TransientOwner(InWorldContextObject);
    const auto Transform = FTransform{InDirection, InOrigin, FVector::OneVector};
    return Add_Arrow(NewEntity, Transform, InLength, InShaftWidth, InArrowHeadRatio, InArrowHeadWidthMultiplier, InColor, InDrawLines, InLineThickness, InDefaultAxis, InDuration);
}

// --------------------------------------------------------------------------------------------------------------------
// Pivot
// --------------------------------------------------------------------------------------------------------------------

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
    auto Common = ck::FFragment_Pmg_DebugShape_Common{};
    Common.Set_Color(FLinearColor::White);
    Common.Set_DrawLines(true);
    Common.Set_LineThickness(2.0f);
    Common.Set_Duration(FCk_Time{InDuration});
    InHandle.Add<ck::FFragment_Pmg_DebugShape_Common>(Common);

    auto Params = ck::FFragment_Pmg_Pivot_Params{};
    Params.Set_AxisLength(InAxisLength);
    Params.Set_ArrowSize(InArrowSize);
    Params.Set_Axis(InDefaultAxis);
    InHandle.Add<ck::FFragment_Pmg_Pivot_Params>(Params);

    InHandle.Add<ck::FFragment_Pmg_DebugShape_Current>();
    InHandle.Add<ck::FTag_Pmg_DebugShape_NeedsSetup>();

    UCk_Utils_Transform_UE::Add(InHandle, InTransform, ECk_Replication::DoesNotReplicate);

    return UCk_Utils_Pmg_DebugShape_UE::Cast(InHandle);
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
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InOwningEntity);
    return Add_Pivot(NewEntity, InTransform, InAxisLength, InArrowSize, InDefaultAxis, InDuration);
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
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_TransientOwner(InWorldContextObject);
    const auto Transform = FTransform{InRotation, InOrigin, FVector::OneVector};
    return Add_Pivot(NewEntity, Transform, InAxisLength, InArrowSize, InDefaultAxis, InDuration);
}

// --------------------------------------------------------------------------------------------------------------------
// DashedLine
// --------------------------------------------------------------------------------------------------------------------

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
    auto Common = ck::FFragment_Pmg_DebugShape_Common{};
    Common.Set_Color(InColor);
    Common.Set_DrawLines(true);
    Common.Set_LineThickness(InThickness);
    Common.Set_Duration(FCk_Time{InDuration});
    InHandle.Add<ck::FFragment_Pmg_DebugShape_Common>(Common);

    auto Params = ck::FFragment_Pmg_DashedLine_Params{};
    Params.Set_Length(InLength);
    Params.Set_DashLength(InDashLength);
    Params.Set_GapLength(InGapLength);
    Params.Set_Thickness(InThickness);
    Params.Set_Axis(InDefaultAxis);
    InHandle.Add<ck::FFragment_Pmg_DashedLine_Params>(Params);

    InHandle.Add<ck::FFragment_Pmg_DebugShape_Current>();
    InHandle.Add<ck::FTag_Pmg_DebugShape_NeedsSetup>();

    UCk_Utils_Transform_UE::Add(InHandle, InTransform, ECk_Replication::DoesNotReplicate);

    return UCk_Utils_Pmg_DebugShape_UE::Cast(InHandle);
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
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InOwningEntity);
    return Add_DashedLine(NewEntity, InTransform, InLength, InDashLength, InGapLength, InThickness, InColor, InDefaultAxis, InDuration);
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
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_TransientOwner(InWorldContextObject);
    const auto Direction = (InEnd - InStart).GetSafeNormal();
    const auto Length = (InEnd - InStart).Length();
    const auto Rotation = Direction.Rotation();
    const auto Transform = FTransform{Rotation, InStart, FVector::OneVector};
    return Add_DashedLine(NewEntity, Transform, Length, InDashLength, InGapLength, InThickness, InColor, InDefaultAxis, InDuration);
}

// --------------------------------------------------------------------------------------------------------------------
