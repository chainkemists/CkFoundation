#include "CkPmg_Utils_SymbolShapes.h"
#include "CkPmg_Utils.h"
#include "CkPmg_Fragment.h"
#include "CkPmg_Fragment_SymbolShapes.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

// --------------------------------------------------------------------------------------------------------------------
// MagnifyingGlass
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Pmg_SymbolShapes::
    Add_MagnifyingGlass(
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

    auto Params = ck::FFragment_Pmg_MagnifyingGlass_Params{};
    Params.Set_Size(InSize);
    Params.Set_Axis(InDefaultAxis);
    InHandle.Add<ck::FFragment_Pmg_MagnifyingGlass_Params>(Params);

    InHandle.Add<ck::FFragment_Pmg_DebugShape_Current>();
    InHandle.Add<ck::FTag_Pmg_DebugShape_NeedsSetup>();

    UCk_Utils_Transform_UE::Add(InHandle, InTransform, ECk_Replication::DoesNotReplicate);

    return UCk_Utils_Pmg_DebugShape_UE::Cast(InHandle);
}

auto
    UCk_Utils_Pmg_SymbolShapes::
    Create_MagnifyingGlass(
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
    return Add_MagnifyingGlass(NewEntity, InTransform, InSize, InColor, InDrawLines, InLineThickness, InDefaultAxis, InDuration);
}

auto
    UCk_Utils_Pmg_SymbolShapes::
    DrawFilledMagnifyingGlass(
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
    const auto Transform = FTransform{FRotator::ZeroRotator, InCenter, FVector::OneVector};
    return Add_MagnifyingGlass(NewEntity, Transform, InSize, InColor, InDrawLines, InLineThickness, InDefaultAxis, InDuration);
}

// --------------------------------------------------------------------------------------------------------------------
// QuestionMark
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Pmg_SymbolShapes::
    Add_QuestionMark(
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

    auto Params = ck::FFragment_Pmg_QuestionMark_Params{};
    Params.Set_Size(InSize);
    Params.Set_Axis(InDefaultAxis);
    InHandle.Add<ck::FFragment_Pmg_QuestionMark_Params>(Params);

    InHandle.Add<ck::FFragment_Pmg_DebugShape_Current>();
    InHandle.Add<ck::FTag_Pmg_DebugShape_NeedsSetup>();

    UCk_Utils_Transform_UE::Add(InHandle, InTransform, ECk_Replication::DoesNotReplicate);

    return UCk_Utils_Pmg_DebugShape_UE::Cast(InHandle);
}

auto
    UCk_Utils_Pmg_SymbolShapes::
    Create_QuestionMark(
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
    return Add_QuestionMark(NewEntity, InTransform, InSize, InColor, InDrawLines, InLineThickness, InDefaultAxis, InDuration);
}

auto
    UCk_Utils_Pmg_SymbolShapes::
    DrawFilledQuestionMark(
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
    const auto Transform = FTransform{FRotator::ZeroRotator, InCenter, FVector::OneVector};
    return Add_QuestionMark(NewEntity, Transform, InSize, InColor, InDrawLines, InLineThickness, InDefaultAxis, InDuration);
}

// --------------------------------------------------------------------------------------------------------------------
// ExclamationMark
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Pmg_SymbolShapes::
    Add_ExclamationMark(
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

    auto Params = ck::FFragment_Pmg_ExclamationMark_Params{};
    Params.Set_Size(InSize);
    Params.Set_Axis(InDefaultAxis);
    InHandle.Add<ck::FFragment_Pmg_ExclamationMark_Params>(Params);

    InHandle.Add<ck::FFragment_Pmg_DebugShape_Current>();
    InHandle.Add<ck::FTag_Pmg_DebugShape_NeedsSetup>();

    UCk_Utils_Transform_UE::Add(InHandle, InTransform, ECk_Replication::DoesNotReplicate);

    return UCk_Utils_Pmg_DebugShape_UE::Cast(InHandle);
}

auto
    UCk_Utils_Pmg_SymbolShapes::
    Create_ExclamationMark(
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
    return Add_ExclamationMark(NewEntity, InTransform, InSize, InColor, InDrawLines, InLineThickness, InDefaultAxis, InDuration);
}

auto
    UCk_Utils_Pmg_SymbolShapes::
    DrawFilledExclamationMark(
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
    const auto Transform = FTransform{FRotator::ZeroRotator, InCenter, FVector::OneVector};
    return Add_ExclamationMark(NewEntity, Transform, InSize, InColor, InDrawLines, InLineThickness, InDefaultAxis, InDuration);
}

// --------------------------------------------------------------------------------------------------------------------
// Flag
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Pmg_SymbolShapes::
    Add_Flag(
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

    auto Params = ck::FFragment_Pmg_Flag_Params{};
    Params.Set_Size(InSize);
    Params.Set_Axis(InDefaultAxis);
    InHandle.Add<ck::FFragment_Pmg_Flag_Params>(Params);

    InHandle.Add<ck::FFragment_Pmg_DebugShape_Current>();
    InHandle.Add<ck::FTag_Pmg_DebugShape_NeedsSetup>();

    UCk_Utils_Transform_UE::Add(InHandle, InTransform, ECk_Replication::DoesNotReplicate);

    return UCk_Utils_Pmg_DebugShape_UE::Cast(InHandle);
}

auto
    UCk_Utils_Pmg_SymbolShapes::
    Create_Flag(
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
    return Add_Flag(NewEntity, InTransform, InSize, InColor, InDrawLines, InLineThickness, InDefaultAxis, InDuration);
}

auto
    UCk_Utils_Pmg_SymbolShapes::
    DrawFilledFlag(
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
    const auto Transform = FTransform{FRotator::ZeroRotator, InCenter, FVector::OneVector};
    return Add_Flag(NewEntity, Transform, InSize, InColor, InDrawLines, InLineThickness, InDefaultAxis, InDuration);
}

// --------------------------------------------------------------------------------------------------------------------
// Pin
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Pmg_SymbolShapes::
    Add_Pin(
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

    auto Params = ck::FFragment_Pmg_Pin_Params{};
    Params.Set_Size(InSize);
    Params.Set_Axis(InDefaultAxis);
    InHandle.Add<ck::FFragment_Pmg_Pin_Params>(Params);

    InHandle.Add<ck::FFragment_Pmg_DebugShape_Current>();
    InHandle.Add<ck::FTag_Pmg_DebugShape_NeedsSetup>();

    UCk_Utils_Transform_UE::Add(InHandle, InTransform, ECk_Replication::DoesNotReplicate);

    return UCk_Utils_Pmg_DebugShape_UE::Cast(InHandle);
}

auto
    UCk_Utils_Pmg_SymbolShapes::
    Create_Pin(
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
    return Add_Pin(NewEntity, InTransform, InSize, InColor, InDrawLines, InLineThickness, InDefaultAxis, InDuration);
}

auto
    UCk_Utils_Pmg_SymbolShapes::
    DrawFilledPin(
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
    const auto Transform = FTransform{FRotator::ZeroRotator, InCenter, FVector::OneVector};
    return Add_Pin(NewEntity, Transform, InSize, InColor, InDrawLines, InLineThickness, InDefaultAxis, InDuration);
}

// --------------------------------------------------------------------------------------------------------------------
