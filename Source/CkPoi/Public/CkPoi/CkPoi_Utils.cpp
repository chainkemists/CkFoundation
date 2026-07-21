#include "CkPoi_Utils.h"

#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/Handle/CkHandle_Utils.h"
#include "CkEcs/Handle/CkDebugCallstack_Macros.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkPoi/CkPoi_Fragment.h"
#include "CkPoi/CkPoi_Log.h"

#include <NativeGameplayTags.h>

// --------------------------------------------------------------------------------------------------------------------

UE_DEFINE_GAMEPLAY_TAG(Tag_Poi_CategoryName, TEXT("Poi.Category"))

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Poi_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_Poi_ParamsData& InParams)
    -> FCk_Handle_Poi
{
    CK_ENSURE_IF_NOT(ck::IsValid(InHandle), TEXT("Invalid Handle supplied to Poi Add"))
    { return {}; }

    CK_ENSURE_IF_NOT(NOT Has(InHandle),
        TEXT("Handle [{}] already has the Poi feature. An entity hosts at most ONE POI — spawn one entity per POI"),
        InHandle)
    { return Cast(InHandle); }

    CK_ENSURE_IF_NOT(UCk_Utils_Transform_UE::Has(InHandle),
        TEXT("Poi Add requires the Entity [{}] to have the Transform feature — the POI's world position is "
             "derived from the entity's transform"), InHandle)
    { return {}; }

    CK_ENSURE_IF_NOT(ck::IsValid(InParams.Get_Category()),
        TEXT("Poi Add on Entity [{}] requires a valid Category tag (Poi.Category.*)"), InHandle)
    { return {}; }

    InHandle.Add<ck::FFragment_Poi_Params>(InParams);
    InHandle.Add<ck::FFragment_Poi_Current>();
    InHandle.Add<ck::FTag_Poi_NeedsSetup>();

    return Cast(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_Poi_UE, FCk_Handle_Poi, ck::FFragment_Poi_Current, ck::FFragment_Poi_Params);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Poi_UE::
    Get_Category(
        const FCk_Handle_Poi& InPoi)
    -> FGameplayTag
{
    return InPoi.Get<ck::FFragment_Poi_Params>().Get_Category();
}

auto
    UCk_Utils_Poi_UE::
    Get_DisplayName(
        const FCk_Handle_Poi& InPoi)
    -> FText
{
    return InPoi.Get<ck::FFragment_Poi_Params>().Get_DisplayName();
}

auto
    UCk_Utils_Poi_UE::
    Get_Priority(
        const FCk_Handle_Poi& InPoi)
    -> int32
{
    return InPoi.Get<ck::FFragment_Poi_Params>().Get_Priority();
}

auto
    UCk_Utils_Poi_UE::
    Get_MaxVisibleRange(
        const FCk_Handle_Poi& InPoi)
    -> float
{
    return InPoi.Get<ck::FFragment_Poi_Params>().Get_MaxVisibleRange();
}

auto
    UCk_Utils_Poi_UE::
    Get_MinVisibleRange(
        const FCk_Handle_Poi& InPoi)
    -> float
{
    return InPoi.Get<ck::FFragment_Poi_Params>().Get_MinVisibleRange();
}

auto
    UCk_Utils_Poi_UE::
    Get_OffscreenPolicy(
        const FCk_Handle_Poi& InPoi)
    -> ECk_Poi_OffscreenPolicy
{
    return InPoi.Get<ck::FFragment_Poi_Params>().Get_OffscreenPolicy();
}

auto
    UCk_Utils_Poi_UE::
    Get_DisplayAsset(
        const FCk_Handle_Poi& InPoi)
    -> TSoftObjectPtr<UCk_Poi_DisplayDefinition_PDA>
{
    return InPoi.Get<ck::FFragment_Poi_Params>().Get_DisplayAsset();
}

auto
    UCk_Utils_Poi_UE::
    Get_EnableDisable(
        const FCk_Handle_Poi& InPoi)
    -> ECk_EnableDisable
{
    return InPoi.Has<ck::FTag_Poi_Disabled>()
        ? ECk_EnableDisable::Disable
        : ECk_EnableDisable::Enable;
}

auto
    UCk_Utils_Poi_UE::
    Get_StateTags(
        const FCk_Handle_Poi& InPoi)
    -> FGameplayTagContainer
{
    return InPoi.Get<ck::FFragment_Poi_Current>().Get_StateTags();
}

auto
    UCk_Utils_Poi_UE::
    Get_WorldLocation(
        const FCk_Handle_Poi& InPoi)
    -> FVector
{
    const auto& EntityTransform = UCk_Utils_Transform_TypeUnsafe_UE::Get_EntityCurrentTransform(InPoi);

    return EntityTransform.TransformPosition(InPoi.Get<ck::FFragment_Poi_Params>().Get_RelativeLocation());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Poi_UE::
    Request_EnableDisable(
        FCk_Handle_Poi& InPoi,
        const FCk_Request_Poi_EnableDisable& InRequest)
    -> FCk_Handle_Poi
{
    CK_CALLSTACK_RECORD(ck::FFragment_Poi_Requests, InPoi);

    InPoi.AddOrGet<ck::FFragment_Poi_Requests>()._Requests.Emplace(InRequest);

    return InPoi;
}

auto
    UCk_Utils_Poi_UE::
    Request_AddStateTag(
        FCk_Handle_Poi& InPoi,
        const FCk_Request_Poi_AddStateTag& InRequest)
    -> FCk_Handle_Poi
{
    CK_CALLSTACK_RECORD(ck::FFragment_Poi_Requests, InPoi);

    InPoi.AddOrGet<ck::FFragment_Poi_Requests>()._Requests.Emplace(InRequest);

    return InPoi;
}

auto
    UCk_Utils_Poi_UE::
    Request_RemoveStateTag(
        FCk_Handle_Poi& InPoi,
        const FCk_Request_Poi_RemoveStateTag& InRequest)
    -> FCk_Handle_Poi
{
    CK_CALLSTACK_RECORD(ck::FFragment_Poi_Requests, InPoi);

    InPoi.AddOrGet<ck::FFragment_Poi_Requests>()._Requests.Emplace(InRequest);

    return InPoi;
}

auto
    UCk_Utils_Poi_UE::
    Request_SetStateTags(
        FCk_Handle_Poi& InPoi,
        const FCk_Request_Poi_SetStateTags& InRequest)
    -> FCk_Handle_Poi
{
    CK_CALLSTACK_RECORD(ck::FFragment_Poi_Requests, InPoi);

    InPoi.AddOrGet<ck::FFragment_Poi_Requests>()._Requests.Emplace(InRequest);

    return InPoi;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Poi_UE::
    BindTo_OnStateChanged(
        FCk_Handle_Poi& InPoi,
        const FCk_Delegate_Poi_StateChanged& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_Poi
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnPoiStateChanged, InPoi, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InPoi;
}

auto
    UCk_Utils_Poi_UE::
    BindTo_OnEnableDisable(
        FCk_Handle_Poi& InPoi,
        const FCk_Delegate_Poi_EnableDisable& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_Poi
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnPoiEnableDisable, InPoi, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InPoi;
}

auto
    UCk_Utils_Poi_UE::
    UnbindFrom_OnStateChanged(
        FCk_Handle_Poi& InPoi,
        const FCk_Delegate_Poi_StateChanged& InDelegate)
    -> FCk_Handle_Poi
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnPoiStateChanged, InPoi, InDelegate);
    return InPoi;
}

auto
    UCk_Utils_Poi_UE::
    UnbindFrom_OnEnableDisable(
        FCk_Handle_Poi& InPoi,
        const FCk_Delegate_Poi_EnableDisable& InDelegate)
    -> FCk_Handle_Poi
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnPoiEnableDisable, InPoi, InDelegate);
    return InPoi;
}

//--------------------------------------------------------------------------------------------------------------------
