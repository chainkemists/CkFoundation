#include "CkTargetable_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Targetable_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_Targetable_ParamsData& InParams)
    -> FCk_Handle_Targetable
{
     auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InHandle, [&](FCk_Handle InNewEntity)
     {
        UCk_Utils_GameplayLabel_UE::Add(InNewEntity, InParams.Get_Name());

        InNewEntity.Add<ck::FFragment_Targetable_Params>(InParams);
        InNewEntity.Add<ck::FFragment_Targetable_Current>(InParams.Get_StartingState());
        InNewEntity.Add<ck::FTag_Targetable_NeedsSetup>();
    });

    auto NewTargetableEntity = Cast(NewEntity);

    RecordOfTargetables_Utils::AddIfMissing(InHandle, ECk_Record_EntryHandlingPolicy::DisallowDuplicateNames);
    RecordOfTargetables_Utils::Request_Connect(InHandle, NewTargetableEntity);

    return NewTargetableEntity;
}

auto
    UCk_Utils_Targetable_UE::
    AddMultiple(
        FCk_Handle& InHandle,
        const FCk_Fragment_MultipleTargetable_ParamsData& InParams)
    -> TArray<FCk_Handle_Targetable>
{
    return ck::algo::Transform<TArray<FCk_Handle_Targetable>>(InParams.Get_TargetableParams(),
    [&](const FCk_Fragment_Targetable_ParamsData& InTargetableParams)
    {
        return Add(InHandle, InTargetableParams);
    });
}

auto
    UCk_Utils_Targetable_UE::
    Has_Any(
        const FCk_Handle& InHandle)
    -> bool
{
    return RecordOfTargetables_Utils::Has(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(Targetable, UCk_Utils_Targetable_UE, FCk_Handle_Targetable, ck::FFragment_Targetable_Current, ck::FFragment_Targetable_Params);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Targetable_UE::
    IsValid(
        const FCk_Targetable_BasicInfo& InTargetableInfo)
    -> bool
{
    return ck::IsValid(InTargetableInfo);
}

auto
    UCk_Utils_Targetable_UE::
    IsEqual(
        const FCk_Targetable_BasicInfo& InTargetableInfoA,
        const FCk_Targetable_BasicInfo& InTargetableInfoB)
    -> bool
{
    return InTargetableInfoA == InTargetableInfoB;
}

auto
    UCk_Utils_Targetable_UE::
    IsNotEqual(
        const FCk_Targetable_BasicInfo& InTargetableInfoA,
        const FCk_Targetable_BasicInfo& InTargetableInfoB)
    -> bool
{
    return InTargetableInfoA != InTargetableInfoB;
}

auto
    UCk_Utils_Targetable_UE::
    TryGet_Targetable(
        const FCk_Handle& InTargetableOwnerEntity,
        FGameplayTag InTargetableName)
    -> FCk_Handle_Targetable
{
    return RecordOfTargetables_Utils::Get_ValidEntry_If(InTargetableOwnerEntity, ck::algo::MatchesGameplayLabelExact{InTargetableName});
}

auto
    UCk_Utils_Targetable_UE::
    Get_EnableDisable(
        const FCk_Handle_Targetable& InTargetable)
    -> ECk_EnableDisable
{
    return InTargetable.Get<ck::FFragment_Targetable_Current>().Get_EnableDisable();
}

auto
    UCk_Utils_Targetable_UE::
    Get_TargetabilityTags(
        const FCk_Handle_Targetable& InTargetable)
    -> FGameplayTagContainer
{
    return InTargetable.Get<ck::FFragment_Targetable_Params>().Get_Params().Get_TargetabilityTags();
}

auto
    UCk_Utils_Targetable_UE::
    Get_AttachmentParams(
        const FCk_Handle_Targetable& InTargetable)
    -> FCk_Targetable_AttachmentParams
{
    return InTargetable.Get<ck::FFragment_Targetable_Params>().Get_Params().Get_AttachmentParams();
}

auto
    UCk_Utils_Targetable_UE::
    Get_AttachmentNode(
        const FCk_Handle_Targetable& InTargetable)
    -> USceneComponent*
{
    return InTargetable.Get<ck::FFragment_Targetable_Current>().Get_AttachmentNode().Get();
}

auto
    UCk_Utils_Targetable_UE::
    ForEach_Targetable(
        FCk_Handle& InTargetableOwnerEntity,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate)
    -> TArray<FCk_Handle_Targetable>
{
    auto Targetables = TArray<FCk_Handle_Targetable>{};

    ForEach_Targetable(InTargetableOwnerEntity, [&](FCk_Handle_Targetable InTargetable)
    {
        Targetables.Emplace(InTargetable);

        std::ignore = InDelegate.ExecuteIfBound(InTargetable, InOptionalPayload);
    });

    return Targetables;
}

auto
    UCk_Utils_Targetable_UE::
    ForEach_Targetable(
        FCk_Handle& InTargetableOwnerEntity,
        const TFunction<void(FCk_Handle_Targetable)>& InFunc)
    -> void
{
    RecordOfTargetables_Utils::ForEach_ValidEntry(InTargetableOwnerEntity, InFunc);
}

auto
    UCk_Utils_Targetable_UE::
    Request_EnableDisable(
        FCk_Handle_Targetable& InTargetable,
        const FCk_Request_Targetable_EnableDisable& InRequest,
        const FCk_Delegate_Targetable_OnEnableDisable& InDelegate)
    -> FCk_Handle_Targetable
{
    CK_SIGNAL_BIND_REQUEST_FULFILLED(ck::UUtils_Signal_OnTargetableEnableDisable, InTargetable, InDelegate);

    InTargetable.AddOrGet<ck::FFragment_Targetable_Requests>()._Requests.Emplace(InRequest);

    return InTargetable;
}

auto
    UCk_Utils_Targetable_UE::
    BindTo_OnEnableDisable(
        FCk_Handle_Targetable& InTargetable,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_Targetable_OnEnableDisable& InDelegate)
    -> FCk_Handle_Targetable
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnTargetableEnableDisable, InTargetable, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InTargetable;
}

auto
    UCk_Utils_Targetable_UE::
    UnbindFrom_OnEnableDisable(
        FCk_Handle_Targetable& InTargetable,
        const FCk_Delegate_Targetable_OnEnableDisable& InDelegate)
    -> FCk_Handle_Targetable
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnTargetableEnableDisable, InTargetable, InDelegate);
    return InTargetable;
}

// --------------------------------------------------------------------------------------------------------------------
