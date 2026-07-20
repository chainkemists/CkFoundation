#include "CkPoi_Utils.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Handle/CkHandle_Utils.h"
#include "CkEcs/Handle/CkDebugCallstack_Macros.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkPoi/CkPoi_EntityScript.h"
#include "CkPoi/CkPoi_Fragment.h"
#include "CkPoi/CkPoi_Log.h"

#include "CkTimer/CkTimer_Fragment.h"
#include "CkTimer/CkTimer_Utils.h"

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

    CK_ENSURE_IF_NOT(UCk_Utils_Transform_UE::Has(InHandle),
        TEXT("Poi Add requires the OWNER Entity [{}] to have the Transform feature — the POI's world position is "
             "derived from the owner's transform"), InHandle)
    { return {}; }

    CK_ENSURE_IF_NOT(ck::IsValid(InParams.Get_Category()),
        TEXT("Poi Add on Entity [{}] requires a valid Category tag (Poi.Category.*)"), InHandle)
    { return {}; }

    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InHandle, [&](FCk_Handle InNewEntity)
    {
        UCk_Utils_GameplayLabel_UE::Add(InNewEntity, InParams.Get_Category());

        InNewEntity.Add<ck::FFragment_Poi_Params>(InParams);
        InNewEntity.Add<ck::FFragment_Poi_Current>();

        InNewEntity.Add<ck::FTag_Poi_NeedsSetup>();
    });

    auto NewPoiEntity = ck::StaticCast<FCk_Handle_Poi>(NewEntity);

    RecordOfPois_Utils::AddIfMissing(InHandle, ECk_Record_EntryHandlingPolicy::Default);
    RecordOfPois_Utils::Request_Connect(InHandle, NewPoiEntity, ECk_Record_LabelRequirementPolicy::Optional);

    return NewPoiEntity;
}

auto
    UCk_Utils_Poi_UE::
    Create(
        const UObject* InWorldContextObject,
        const FTransform& InTransform,
        const FCk_Fragment_Poi_ParamsData& InParams,
        float InTtlSeconds,
        ECk_Replication InReplication)
    -> FCk_Handle_Poi
{
    CK_ENSURE_VALID_UNREAL_WORLD_IF_NOT(InWorldContextObject)
    { return {}; }

    auto HostEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_TransientOwner(InWorldContextObject);

    UCk_Utils_Transform_UE::Add(HostEntity, InTransform, InReplication);

    auto NewPoiEntity = Add(HostEntity, InParams);

    if (ck::Is_NOT_Valid(NewPoiEntity))
    { return {}; }

    if (InTtlSeconds > 0.0f)
    {
        NewPoiEntity.Add<ck::FTag_Poi_TransientTtl>();

        const auto TimerParams = FCk_Fragment_Timer_ParamsData{FCk_Time{InTtlSeconds}}
            .Set_Behavior(ECk_Timer_Behavior::StopOnDone)
            .Set_StartingState(ECk_Timer_State::Running);

        auto TtlTimer = UCk_Utils_Timer_UE::Add(HostEntity, TimerParams);

        ck::UUtils_Signal_OnTimerDone::Bind<&ThisType::OnTtlTimerDone>(
            TtlTimer, ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame, ECk_Signal_PostFireBehavior::Unbind);
    }

    return NewPoiEntity;
}

auto
    UCk_Utils_Poi_UE::
    Create_Durable(
        const UObject* InWorldContextObject,
        const FTransform& InTransform,
        const FCk_Fragment_Poi_ParamsData& InParams)
    -> FCk_Handle_PendingEntityScript
{
    CK_ENSURE_VALID_UNREAL_WORLD_IF_NOT(InWorldContextObject)
    { return {}; }

    auto TransientEntity = UCk_Utils_EcsWorld_Subsystem_UE::Get_TransientEntity_FromContextObject(InWorldContextObject);

    CK_ENSURE_IF_NOT(ck::IsValid(TransientEntity),
        TEXT("Create_Durable could not resolve the TransientEntity for the current world"))
    { return {}; }

    const auto SpawnParams = FCk_Poi_SpawnParams{InParams, InTransform};

    return UCk_Utils_EntityScript_UE::Request_SpawnEntity(
        TransientEntity, UCk_Poi_EntityScript::StaticClass(), FInstancedStruct::Make(SpawnParams));
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Poi_UE::
    Has_Any(
        const FCk_Handle& InPoiOwnerEntity)
    -> bool
{
    return RecordOfPois_Utils::Has(InPoiOwnerEntity);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_Poi_UE, FCk_Handle_Poi, ck::FFragment_Poi_Current, ck::FFragment_Poi_Params);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Poi_UE::
    TryGet_Poi_ByCategory(
        const FCk_Handle& InPoiOwnerEntity,
        FGameplayTag InCategory)
    -> FCk_Handle_Poi
{
    return RecordOfPois_Utils::Get_ValidEntry_ByTag(InPoiOwnerEntity, InCategory);
}

auto
    UCk_Utils_Poi_UE::
    ForEach_Poi(
        const FCk_Handle& InPoiOwnerEntity,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate)
    -> TArray<FCk_Handle_Poi>
{
    auto Pois = TArray<FCk_Handle_Poi>{};

    ForEach_Poi(InPoiOwnerEntity, [&](FCk_Handle_Poi InPoi)
    {
        if (InDelegate.IsBound())
        { InDelegate.Execute(InPoi, InOptionalPayload); }
        else
        { Pois.Emplace(InPoi); }
    });

    return Pois;
}

auto
    UCk_Utils_Poi_UE::
    ForEach_Poi(
        const FCk_Handle& InPoiOwnerEntity,
        const TFunction<void(FCk_Handle_Poi)>& InFunc)
    -> void
{
    RecordOfPois_Utils::ForEach_ValidEntry(InPoiOwnerEntity, InFunc);
}

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
    return InPoi.Get<ck::FFragment_Poi_Current>().Get_EnableDisable();
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
    const auto& Owner = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InPoi);

    CK_ENSURE_IF_NOT(ck::IsValid(Owner), TEXT("Poi [{}] has no valid lifetime owner to derive a world location from"), InPoi)
    { return FVector::ZeroVector; }

    const auto& OwnerTransform = UCk_Utils_Transform_TypeUnsafe_UE::Get_EntityCurrentTransform(Owner);

    return OwnerTransform.TransformPosition(InPoi.Get<ck::FFragment_Poi_Params>().Get_RelativeLocation());
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

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Poi_UE::
    OnTtlTimerDone(
        FCk_Handle_Timer InTimer,
        FCk_Chrono InChrono,
        FCk_Time InDeltaT)
    -> void
{
    // The TTL timer is a child of the transient POI's HOST entity — destroying the host tears down the POI child
    // (and this timer) through the standard entity-lifetime pipeline.
    auto HostEntity = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InTimer);

    if (ck::Is_NOT_Valid(HostEntity))
    { return; }

    UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(HostEntity);
}

//--------------------------------------------------------------------------------------------------------------------
