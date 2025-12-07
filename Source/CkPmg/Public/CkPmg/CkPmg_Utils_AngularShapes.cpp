#include "CkPmg_Utils_AngularShapes.h"
#include "CkPmg_Utils.h"
#include "CkPmg_Fragment.h"
#include "CkPmg_Fragment_AngularShapes.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

// --------------------------------------------------------------------------------------------------------------------
// Wedge
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Pmg_AngularShapes::
    Add_Wedge(
        FCk_Handle& InHandle,
        FTransform InTransform,
        float InRadius,
        float InStartAngle,
        float InEndAngle,
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

    auto Params = ck::FFragment_Pmg_Wedge_Params{};
    Params.Set_Radius(InRadius);
    Params.Set_StartAngle(InStartAngle);
    Params.Set_EndAngle(InEndAngle);
    Params.Set_Segments(InSegments);
    Params.Set_Axis(InDefaultAxis);
    InHandle.Add<ck::FFragment_Pmg_Wedge_Params>(Params);

    InHandle.Add<ck::FFragment_Pmg_DebugShape_Current>();
    InHandle.Add<ck::FTag_Pmg_DebugShape_NeedsSetup>();

    UCk_Utils_Transform_UE::Add(InHandle, InTransform, ECk_Replication::DoesNotReplicate);

    return UCk_Utils_Pmg_DebugShape_UE::Cast(InHandle);
}

auto
    UCk_Utils_Pmg_AngularShapes::
    Create_Wedge(
        FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InRadius,
        float InStartAngle,
        float InEndAngle,
        int32 InSegments,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InOwningEntity);
    return Add_Wedge(NewEntity, InTransform, InRadius, InStartAngle, InEndAngle, InSegments, InColor, InDrawLines, InLineThickness, InDefaultAxis, InDuration);
}

auto
    UCk_Utils_Pmg_AngularShapes::
    DrawFilledWedge(
        const UObject* InWorldContextObject,
        FVector InCenter,
        float InRadius,
        float InStartAngle,
        float InEndAngle,
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
    return Add_Wedge(NewEntity, Transform, InRadius, InStartAngle, InEndAngle, InSegments, InColor, InDrawLines, InLineThickness, InDefaultAxis, InDuration);
}

// --------------------------------------------------------------------------------------------------------------------
// Arc
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Pmg_AngularShapes::
    Add_Arc(
        FCk_Handle& InHandle,
        FTransform InTransform,
        float InRadius,
        float InStartAngle,
        float InEndAngle,
        float InThickness,
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

    auto Params = ck::FFragment_Pmg_Arc_Params{};
    Params.Set_Radius(InRadius);
    Params.Set_StartAngle(InStartAngle);
    Params.Set_EndAngle(InEndAngle);
    Params.Set_Thickness(InThickness);
    Params.Set_Segments(InSegments);
    Params.Set_Axis(InDefaultAxis);
    InHandle.Add<ck::FFragment_Pmg_Arc_Params>(Params);

    InHandle.Add<ck::FFragment_Pmg_DebugShape_Current>();
    InHandle.Add<ck::FTag_Pmg_DebugShape_NeedsSetup>();

    UCk_Utils_Transform_UE::Add(InHandle, InTransform, ECk_Replication::DoesNotReplicate);

    return UCk_Utils_Pmg_DebugShape_UE::Cast(InHandle);
}

auto
    UCk_Utils_Pmg_AngularShapes::
    Create_Arc(
        FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InRadius,
        float InStartAngle,
        float InEndAngle,
        float InThickness,
        int32 InSegments,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InOwningEntity);
    return Add_Arc(NewEntity, InTransform, InRadius, InStartAngle, InEndAngle, InThickness, InSegments, InColor, InDrawLines, InLineThickness, InDefaultAxis, InDuration);
}

auto
    UCk_Utils_Pmg_AngularShapes::
    DrawFilledArc(
        const UObject* InWorldContextObject,
        FVector InCenter,
        float InRadius,
        float InStartAngle,
        float InEndAngle,
        float InThickness,
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
    return Add_Arc(NewEntity, Transform, InRadius, InStartAngle, InEndAngle, InThickness, InSegments, InColor, InDrawLines, InLineThickness, InDefaultAxis, InDuration);
}

// --------------------------------------------------------------------------------------------------------------------
// WedgeCone
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Pmg_AngularShapes::
    Add_WedgeCone(
        FCk_Handle& InHandle,
        FTransform InTransform,
        float InRadius,
        float InHeight,
        float InStartAngle,
        float InEndAngle,
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

    auto Params = ck::FFragment_Pmg_WedgeCone_Params{};
    Params.Set_Radius(InRadius);
    Params.Set_Height(InHeight);
    Params.Set_StartAngle(InStartAngle);
    Params.Set_EndAngle(InEndAngle);
    Params.Set_Segments(InSegments);
    Params.Set_Axis(InDefaultAxis);
    InHandle.Add<ck::FFragment_Pmg_WedgeCone_Params>(Params);

    InHandle.Add<ck::FFragment_Pmg_DebugShape_Current>();
    InHandle.Add<ck::FTag_Pmg_DebugShape_NeedsSetup>();

    UCk_Utils_Transform_UE::Add(InHandle, InTransform, ECk_Replication::DoesNotReplicate);

    return UCk_Utils_Pmg_DebugShape_UE::Cast(InHandle);
}

auto
    UCk_Utils_Pmg_AngularShapes::
    Create_WedgeCone(
        FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InRadius,
        float InHeight,
        float InStartAngle,
        float InEndAngle,
        int32 InSegments,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InOwningEntity);
    return Add_WedgeCone(NewEntity, InTransform, InRadius, InHeight, InStartAngle, InEndAngle, InSegments, InColor, InDrawLines, InLineThickness, InDefaultAxis, InDuration);
}

auto
    UCk_Utils_Pmg_AngularShapes::
    DrawFilledWedgeCone(
        const UObject* InWorldContextObject,
        FVector InOrigin,
        FRotator InRotation,
        float InRadius,
        float InHeight,
        float InStartAngle,
        float InEndAngle,
        int32 InSegments,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_TransientOwner(InWorldContextObject);
    const auto Transform = FTransform{InRotation, InOrigin, FVector::OneVector};
    return Add_WedgeCone(NewEntity, Transform, InRadius, InHeight, InStartAngle, InEndAngle, InSegments, InColor, InDrawLines, InLineThickness, InDefaultAxis, InDuration);
}

// --------------------------------------------------------------------------------------------------------------------
