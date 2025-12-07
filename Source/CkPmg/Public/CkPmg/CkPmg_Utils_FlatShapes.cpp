#include "CkPmg_Utils_FlatShapes.h"
#include "CkPmg_Utils.h"
#include "CkPmg_Fragment.h"
#include "CkPmg_Fragment_FlatShapes.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

// --------------------------------------------------------------------------------------------------------------------
// Circle
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Pmg_FlatShapes::
    Add_Circle(
        FCk_Handle& InHandle,
        FTransform InTransform,
        float InRadius,
        int32 InSegments,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        bool InDrawDirectionLine,
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

    auto Params = ck::FFragment_Pmg_Circle_Params{};
    Params.Set_Radius(InRadius);
    Params.Set_Segments(InSegments);
    Params.Set_DrawDirectionLine(InDrawDirectionLine);
    Params.Set_Axis(InDefaultAxis);
    InHandle.Add<ck::FFragment_Pmg_Circle_Params>(Params);

    InHandle.Add<ck::FFragment_Pmg_DebugShape_Current>();
    InHandle.Add<ck::FTag_Pmg_DebugShape_NeedsSetup>();

    UCk_Utils_Transform_UE::Add(InHandle, InTransform, ECk_Replication::DoesNotReplicate);

    return UCk_Utils_Pmg_DebugShape_UE::Cast(InHandle);
}

auto
    UCk_Utils_Pmg_FlatShapes::
    Create_Circle(
        FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InRadius,
        int32 InSegments,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        bool InDrawDirectionLine,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InOwningEntity);
    return Add_Circle(NewEntity, InTransform, InRadius, InSegments, InColor, InDrawLines, InLineThickness, InDrawDirectionLine, InDefaultAxis, InDuration);
}

auto
    UCk_Utils_Pmg_FlatShapes::
    DrawFilledCircle(
        const UObject* InWorldContextObject,
        FVector InCenter,
        float InRadius,
        int32 InSegments,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        bool InDrawDirectionLine,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_TransientOwner(InWorldContextObject);
    const auto Transform = FTransform{FRotator::ZeroRotator, InCenter, FVector::OneVector};
    return Add_Circle(NewEntity, Transform, InRadius, InSegments, InColor, InDrawLines, InLineThickness, InDrawDirectionLine, InDefaultAxis, InDuration);
}

// --------------------------------------------------------------------------------------------------------------------
// Plane
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Pmg_FlatShapes::
    Add_Plane(
        FCk_Handle& InHandle,
        FTransform InTransform,
        float InWidth,
        float InHeight,
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

    auto Params = ck::FFragment_Pmg_Plane_Params{};
    Params.Set_Width(InWidth);
    Params.Set_Height(InHeight);
    Params.Set_Axis(InDefaultAxis);
    InHandle.Add<ck::FFragment_Pmg_Plane_Params>(Params);

    InHandle.Add<ck::FFragment_Pmg_DebugShape_Current>();
    InHandle.Add<ck::FTag_Pmg_DebugShape_NeedsSetup>();

    UCk_Utils_Transform_UE::Add(InHandle, InTransform, ECk_Replication::DoesNotReplicate);

    return UCk_Utils_Pmg_DebugShape_UE::Cast(InHandle);
}

auto
    UCk_Utils_Pmg_FlatShapes::
    Create_Plane(
        FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InWidth,
        float InHeight,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InOwningEntity);
    return Add_Plane(NewEntity, InTransform, InWidth, InHeight, InColor, InDrawLines, InLineThickness, InDefaultAxis, InDuration);
}

auto
    UCk_Utils_Pmg_FlatShapes::
    DrawFilledPlane(
        const UObject* InWorldContextObject,
        FVector InCenter,
        float InWidth,
        float InHeight,
        FRotator InRotation,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_TransientOwner(InWorldContextObject);
    const auto Transform = FTransform{InRotation, InCenter, FVector::OneVector};
    return Add_Plane(NewEntity, Transform, InWidth, InHeight, InColor, InDrawLines, InLineThickness, InDefaultAxis, InDuration);
}

// --------------------------------------------------------------------------------------------------------------------
// Ring
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Pmg_FlatShapes::
    Add_Ring(
        FCk_Handle& InHandle,
        FTransform InTransform,
        float InOuterRadius,
        float InInnerRadius,
        int32 InSegments,
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

    auto Params = ck::FFragment_Pmg_Ring_Params{};
    Params.Set_OuterRadius(InOuterRadius);
    Params.Set_InnerRadius(InInnerRadius);
    Params.Set_Segments(InSegments);
    Params.Set_Axis(InDefaultAxis);
    InHandle.Add<ck::FFragment_Pmg_Ring_Params>(Params);

    InHandle.Add<ck::FFragment_Pmg_DebugShape_Current>();
    InHandle.Add<ck::FTag_Pmg_DebugShape_NeedsSetup>();

    UCk_Utils_Transform_UE::Add(InHandle, InTransform, ECk_Replication::DoesNotReplicate);

    return UCk_Utils_Pmg_DebugShape_UE::Cast(InHandle);
}

auto
    UCk_Utils_Pmg_FlatShapes::
    Create_Ring(
        FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InOuterRadius,
        float InInnerRadius,
        int32 InSegments,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InOwningEntity);
    return Add_Ring(NewEntity, InTransform, InOuterRadius, InInnerRadius, InSegments, InColor, InDrawLines, InLineThickness, InDefaultAxis, InDuration);
}

auto
    UCk_Utils_Pmg_FlatShapes::
    DrawFilledRing(
        const UObject* InWorldContextObject,
        FVector InCenter,
        float InOuterRadius,
        float InInnerRadius,
        int32 InSegments,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_TransientOwner(InWorldContextObject);
    const auto Transform = FTransform{FRotator::ZeroRotator, InCenter, FVector::OneVector};
    return Add_Ring(NewEntity, Transform, InOuterRadius, InInnerRadius, InSegments, InColor, InDrawLines, InLineThickness, InDefaultAxis, InDuration);
}

// --------------------------------------------------------------------------------------------------------------------
// Cross
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Pmg_FlatShapes::
    Add_Cross(
        FCk_Handle& InHandle,
        FTransform InTransform,
        float InSize,
        float InThickness,
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

    auto Params = ck::FFragment_Pmg_Cross_Params{};
    Params.Set_Size(InSize);
    Params.Set_Thickness(InThickness);
    Params.Set_Axis(InDefaultAxis);
    InHandle.Add<ck::FFragment_Pmg_Cross_Params>(Params);

    InHandle.Add<ck::FFragment_Pmg_DebugShape_Current>();
    InHandle.Add<ck::FTag_Pmg_DebugShape_NeedsSetup>();

    UCk_Utils_Transform_UE::Add(InHandle, InTransform, ECk_Replication::DoesNotReplicate);

    return UCk_Utils_Pmg_DebugShape_UE::Cast(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------
// Star
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Pmg_FlatShapes::
    Add_Star(
        FCk_Handle& InHandle,
        FTransform InTransform,
        float InOuterRadius,
        int32 InPoints,
        float InInnerRadiusRatio,
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

    auto Params = ck::FFragment_Pmg_Star_Params{};
    Params.Set_OuterRadius(InOuterRadius);
    Params.Set_Points(InPoints);
    Params.Set_InnerRadiusRatio(InInnerRadiusRatio);
    Params.Set_Axis(InDefaultAxis);
    InHandle.Add<ck::FFragment_Pmg_Star_Params>(Params);

    InHandle.Add<ck::FFragment_Pmg_DebugShape_Current>();
    InHandle.Add<ck::FTag_Pmg_DebugShape_NeedsSetup>();

    UCk_Utils_Transform_UE::Add(InHandle, InTransform, ECk_Replication::DoesNotReplicate);

    return UCk_Utils_Pmg_DebugShape_UE::Cast(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------
// Checkmark
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Pmg_FlatShapes::
    Add_Checkmark(
        FCk_Handle& InHandle,
        FTransform InTransform,
        float InSize,
        float InThickness,
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

    auto Params = ck::FFragment_Pmg_Checkmark_Params{};
    Params.Set_Size(InSize);
    Params.Set_Thickness(InThickness);
    Params.Set_Axis(InDefaultAxis);
    InHandle.Add<ck::FFragment_Pmg_Checkmark_Params>(Params);

    InHandle.Add<ck::FFragment_Pmg_DebugShape_Current>();
    InHandle.Add<ck::FTag_Pmg_DebugShape_NeedsSetup>();

    UCk_Utils_Transform_UE::Add(InHandle, InTransform, ECk_Replication::DoesNotReplicate);

    return UCk_Utils_Pmg_DebugShape_UE::Cast(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------
// Diamond
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Pmg_FlatShapes::
    Add_Diamond(
        FCk_Handle& InHandle,
        FTransform InTransform,
        float InSize,
        float InThickness,
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

    auto Params = ck::FFragment_Pmg_Diamond_Params{};
    Params.Set_Width(InSize);
    Params.Set_Height(InThickness);
    Params.Set_Axis(InDefaultAxis);
    InHandle.Add<ck::FFragment_Pmg_Diamond_Params>(Params);

    InHandle.Add<ck::FFragment_Pmg_DebugShape_Current>();
    InHandle.Add<ck::FTag_Pmg_DebugShape_NeedsSetup>();

    UCk_Utils_Transform_UE::Add(InHandle, InTransform, ECk_Replication::DoesNotReplicate);

    return UCk_Utils_Pmg_DebugShape_UE::Cast(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Pmg_FlatShapes::
    Create_Cross(
        FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InSize,
        float InThickness,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InOwningEntity);
    return Add_Cross(NewEntity, InTransform, InSize, InThickness, InColor, InDrawLines, InLineThickness, InDefaultAxis, InDuration);
}

auto
    UCk_Utils_Pmg_FlatShapes::
    Create_Star(
        FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InOuterRadius,
        int32 InPoints,
        float InInnerRadiusRatio,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InOwningEntity);
    return Add_Star(NewEntity, InTransform, InOuterRadius, InPoints, InInnerRadiusRatio, InColor, InDrawLines, InLineThickness, InDefaultAxis, InDuration);
}

auto
    UCk_Utils_Pmg_FlatShapes::
    Create_Checkmark(
        FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InSize,
        float InThickness,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InOwningEntity);
    return Add_Checkmark(NewEntity, InTransform, InSize, InThickness, InColor, InDrawLines, InLineThickness, InDefaultAxis, InDuration);
}

auto
    UCk_Utils_Pmg_FlatShapes::
    Create_Diamond(
        FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InSize,
        float InThickness,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InOwningEntity);
    return Add_Diamond(NewEntity, InTransform, InSize, InThickness, InColor, InDrawLines, InLineThickness, InDefaultAxis, InDuration);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Pmg_FlatShapes::
    DrawFilledCross(
        const UObject* InWorldContextObject,
        FVector InCenter,
        float InSize,
        float InThickness,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_TransientOwner(InWorldContextObject);
    const auto Transform = FTransform{FRotator::ZeroRotator, InCenter, FVector::OneVector};
    return Add_Cross(NewEntity, Transform, InSize, InThickness, InColor, InDrawLines, InLineThickness, InDefaultAxis, InDuration);
}

auto
    UCk_Utils_Pmg_FlatShapes::
    DrawFilledStar(
        const UObject* InWorldContextObject,
        FVector InCenter,
        float InOuterRadius,
        int32 InPoints,
        float InInnerRadiusRatio,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_TransientOwner(InWorldContextObject);
    const auto Transform = FTransform{FRotator::ZeroRotator, InCenter, FVector::OneVector};
    return Add_Star(NewEntity, Transform, InOuterRadius, InPoints, InInnerRadiusRatio, InColor, InDrawLines, InLineThickness, InDefaultAxis, InDuration);
}

auto
    UCk_Utils_Pmg_FlatShapes::
    DrawFilledCheckmark(
        const UObject* InWorldContextObject,
        FVector InCenter,
        float InSize,
        float InThickness,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_TransientOwner(InWorldContextObject);
    const auto Transform = FTransform{FRotator::ZeroRotator, InCenter, FVector::OneVector};
    return Add_Checkmark(NewEntity, Transform, InSize, InThickness, InColor, InDrawLines, InLineThickness, InDefaultAxis, InDuration);
}

auto
    UCk_Utils_Pmg_FlatShapes::
    DrawFilledDiamond(
        const UObject* InWorldContextObject,
        FVector InCenter,
        float InSize,
        float InThickness,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_TransientOwner(InWorldContextObject);
    const auto Transform = FTransform{FRotator::ZeroRotator, InCenter, FVector::OneVector};
    return Add_Diamond(NewEntity, Transform, InSize, InThickness, InColor, InDrawLines, InLineThickness, InDefaultAxis, InDuration);
}

// --------------------------------------------------------------------------------------------------------------------
