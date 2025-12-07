#include "CkPmg_Utils_IconShapes.h"
#include "CkPmg_Utils.h"
#include "CkPmg_Fragment.h"
#include "CkPmg_Fragment_IconShapes.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

// --------------------------------------------------------------------------------------------------------------------
// Add_ functions
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
    auto Common = ck::FFragment_Pmg_DebugShape_Common{};
    Common.Set_Color(InColor);
    Common.Set_DrawLines(InDrawLines);
    Common.Set_LineThickness(InLineThickness);
    Common.Set_Duration(FCk_Time{InDuration});
    InHandle.Add<ck::FFragment_Pmg_DebugShape_Common>(Common);

    auto Params = ck::FFragment_Pmg_Warning_Params{};
    Params.Set_Size(InSize);
    Params.Set_Axis(InDefaultAxis);
    InHandle.Add<ck::FFragment_Pmg_Warning_Params>(Params);

    InHandle.Add<ck::FFragment_Pmg_DebugShape_Current>();
    InHandle.Add<ck::FTag_Pmg_DebugShape_NeedsSetup>();

    UCk_Utils_Transform_UE::Add(InHandle, InTransform, ECk_Replication::DoesNotReplicate);

    return UCk_Utils_Pmg_DebugShape_UE::Cast(InHandle);
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
    auto Common = ck::FFragment_Pmg_DebugShape_Common{};
    Common.Set_Color(InColor);
    Common.Set_DrawLines(InDrawLines);
    Common.Set_LineThickness(InLineThickness);
    Common.Set_Duration(FCk_Time{InDuration});
    InHandle.Add<ck::FFragment_Pmg_DebugShape_Common>(Common);

    auto Params = ck::FFragment_Pmg_Prohibition_Params{};
    Params.Set_Radius(InRadius);
    Params.Set_Segments(InSegments);
    Params.Set_Axis(InDefaultAxis);
    InHandle.Add<ck::FFragment_Pmg_Prohibition_Params>(Params);

    InHandle.Add<ck::FFragment_Pmg_DebugShape_Current>();
    InHandle.Add<ck::FTag_Pmg_DebugShape_NeedsSetup>();

    UCk_Utils_Transform_UE::Add(InHandle, InTransform, ECk_Replication::DoesNotReplicate);

    return UCk_Utils_Pmg_DebugShape_UE::Cast(InHandle);
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
    auto Common = ck::FFragment_Pmg_DebugShape_Common{};
    Common.Set_Color(InColor);
    Common.Set_DrawLines(InDrawLines);
    Common.Set_LineThickness(InLineThickness);
    Common.Set_Duration(FCk_Time{InDuration});
    InHandle.Add<ck::FFragment_Pmg_DebugShape_Common>(Common);

    auto Params = ck::FFragment_Pmg_NoEntry_Params{};
    Params.Set_Radius(InRadius);
    Params.Set_Segments(InSegments);
    Params.Set_Axis(InDefaultAxis);
    InHandle.Add<ck::FFragment_Pmg_NoEntry_Params>(Params);

    InHandle.Add<ck::FFragment_Pmg_DebugShape_Current>();
    InHandle.Add<ck::FTag_Pmg_DebugShape_NeedsSetup>();

    UCk_Utils_Transform_UE::Add(InHandle, InTransform, ECk_Replication::DoesNotReplicate);

    return UCk_Utils_Pmg_DebugShape_UE::Cast(InHandle);
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
    auto Common = ck::FFragment_Pmg_DebugShape_Common{};
    Common.Set_Color(InColor);
    Common.Set_DrawLines(InDrawLines);
    Common.Set_LineThickness(InLineThickness);
    Common.Set_Duration(FCk_Time{InDuration});
    InHandle.Add<ck::FFragment_Pmg_DebugShape_Common>(Common);

    auto Params = ck::FFragment_Pmg_InfoCircle_Params{};
    Params.Set_Radius(InRadius);
    Params.Set_Segments(InSegments);
    Params.Set_Axis(InDefaultAxis);
    InHandle.Add<ck::FFragment_Pmg_InfoCircle_Params>(Params);

    InHandle.Add<ck::FFragment_Pmg_DebugShape_Current>();
    InHandle.Add<ck::FTag_Pmg_DebugShape_NeedsSetup>();

    UCk_Utils_Transform_UE::Add(InHandle, InTransform, ECk_Replication::DoesNotReplicate);

    return UCk_Utils_Pmg_DebugShape_UE::Cast(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------
// Create_ functions
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
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InOwningEntity);
    return Add_Warning(NewEntity, InTransform, InSize, InColor, InDrawLines, InLineThickness, InDefaultAxis, InDuration);
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
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InOwningEntity);
    return Add_Prohibition(NewEntity, InTransform, InRadius, InSegments, InColor, InDrawLines, InLineThickness, InDefaultAxis, InDuration);
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
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InOwningEntity);
    return Add_NoEntry(NewEntity, InTransform, InRadius, InSegments, InColor, InDrawLines, InLineThickness, InDefaultAxis, InDuration);
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
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InOwningEntity);
    return Add_InfoCircle(NewEntity, InTransform, InRadius, InSegments, InColor, InDrawLines, InLineThickness, InDefaultAxis, InDuration);
}

// --------------------------------------------------------------------------------------------------------------------
// DrawFilled_ functions
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
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_TransientOwner(InWorldContextObject);
    const auto Transform = FTransform(FRotator::ZeroRotator, InCenter, FVector::OneVector);
    return Add_Warning(NewEntity, Transform, InSize, InColor, InDrawLines, InLineThickness, InDefaultAxis, InDuration);
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
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_TransientOwner(InWorldContextObject);
    const auto Transform = FTransform(FRotator::ZeroRotator, InCenter, FVector::OneVector);
    return Add_Prohibition(NewEntity, Transform, InRadius, InSegments, InColor, InDrawLines, InLineThickness, InDefaultAxis, InDuration);
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
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_TransientOwner(InWorldContextObject);
    const auto Transform = FTransform(FRotator::ZeroRotator, InCenter, FVector::OneVector);
    return Add_NoEntry(NewEntity, Transform, InRadius, InSegments, InColor, InDrawLines, InLineThickness, InDefaultAxis, InDuration);
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
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_TransientOwner(InWorldContextObject);
    const auto Transform = FTransform(FRotator::ZeroRotator, InCenter, FVector::OneVector);
    return Add_InfoCircle(NewEntity, Transform, InRadius, InSegments, InColor, InDrawLines, InLineThickness, InDefaultAxis, InDuration);
}

// --------------------------------------------------------------------------------------------------------------------
