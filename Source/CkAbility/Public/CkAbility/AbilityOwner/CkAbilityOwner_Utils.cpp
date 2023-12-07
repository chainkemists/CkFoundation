#include "CkAbilityOwner_Utils.h"

#include "CkAbility/Ability/CkAbility_Utils.h"
#include "CkAbility/AbilityOwner/CkAbilityOwner_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_AbilityOwner_UE::
    Add(
        FCk_Handle                                  InHandle,
        const FCk_Fragment_AbilityOwner_ParamsData& InParams)
    -> void
{
    InHandle.Add<ck::FFragment_AbilityOwner_Params>(InParams);
    InHandle.Add<ck::FFragment_AbilityOwner_Current>();
    InHandle.Add<ck::FTag_AbilityOwner_Setup>();
}

auto
    UCk_Utils_AbilityOwner_UE::
    Has(
        FCk_Handle InHandle)
    -> bool
{
    return InHandle.Has_All<ck::FFragment_AbilityOwner_Current, ck::FFragment_AbilityOwner_Params>();
}

auto
    UCk_Utils_AbilityOwner_UE::
    Ensure(
        FCk_Handle InHandle)
    -> bool
{
    CK_ENSURE_IF_NOT(Has(InHandle), TEXT("Handle [{}] does NOT have an Ability Owner"), InHandle)
    { return false; }

    return true;
}

auto
    UCk_Utils_AbilityOwner_UE::
    Get_ActiveTags(
        FCk_Handle InAbilityOwnerHandle)
    -> FGameplayTagContainer
{
    if (NOT Ensure(InAbilityOwnerHandle))
    { return {}; }

    return InAbilityOwnerHandle.Get<ck::FFragment_AbilityOwner_Current>().Get_ActiveTags();
}

auto
    UCk_Utils_AbilityOwner_UE::
    Request_GiveAbility(
        FCk_Handle                                  InAbilityOwnerHandle,
        const FCk_Request_AbilityOwner_GiveAbility& InRequest)
    -> void
{
    if (NOT Ensure(InAbilityOwnerHandle))
    { return; }

    InAbilityOwnerHandle.AddOrGet<ck::FFragment_AbilityOwner_Requests>()._Requests.Emplace(InRequest);
}

auto
    UCk_Utils_AbilityOwner_UE::
    Request_RevokeAbility(
        FCk_Handle                                    InAbilityOwnerHandle,
        const FCk_Request_AbilityOwner_RevokeAbility& InRequest)
    -> void
{
    if (NOT Ensure(InAbilityOwnerHandle))
    { return; }

    InAbilityOwnerHandle.AddOrGet<ck::FFragment_AbilityOwner_Requests>()._Requests.Emplace(InRequest);
}

auto
    UCk_Utils_AbilityOwner_UE::
    Request_TryActivateAbility(
        FCk_Handle                                      InAbilityOwnerHandle,
        const FCk_Request_AbilityOwner_ActivateAbility& InRequest)
    -> void
{
    if (NOT Ensure(InAbilityOwnerHandle))
    { return; }

    InAbilityOwnerHandle.AddOrGet<ck::FFragment_AbilityOwner_Requests>()._Requests.Emplace(InRequest);
}

auto
    UCk_Utils_AbilityOwner_UE::
    Request_EndAbility(
        FCk_Handle                                 InAbilityOwnerHandle,
        const FCk_Request_AbilityOwner_EndAbility& InRequest)
    -> void
{
    if (NOT Ensure(InAbilityOwnerHandle))
    { return; }

    InAbilityOwnerHandle.AddOrGet<ck::FFragment_AbilityOwner_Requests>()._Requests.Emplace(InRequest);
}

auto
    UCk_Utils_AbilityOwner_UE::
    Request_SendEvent(
        FCk_Handle                    InAbilityOwnerHandle,
        const FCk_Request_AbilityOwner_SendEvent& InRequest)
    -> void
{
    if (NOT Ensure(InAbilityOwnerHandle))
    { return; }

    auto& Events = InAbilityOwnerHandle.AddOrGet<ck::FFragment_AbilityOwner_Events>();
    Events._Events.Emplace(InRequest.Get_Event());
}

auto
    UCk_Utils_AbilityOwner_UE::
    BindTo_OnEvents(
        FCk_Handle                              InAbilityOwnerHandle,
        ECk_Signal_BindingPolicy                InBehavior,
        const FCk_Delegate_AbilityOwner_Events& InDelegate)
    -> void
{
    if (NOT Ensure(InAbilityOwnerHandle))
    { return; }

    ck::UUtils_Signal_AbilityOwner_Events::Bind(InAbilityOwnerHandle, InDelegate, InBehavior);
}

auto
    UCk_Utils_AbilityOwner_UE::
    UnbindFrom_OnEvents(
        FCk_Handle                              InAbilityOwnerHandle,
        const FCk_Delegate_AbilityOwner_Events& InDelegate)
    -> void
{
    if (NOT Ensure(InAbilityOwnerHandle))
    { return; }

    ck::UUtils_Signal_AbilityOwner_Events::Unbind(InAbilityOwnerHandle, InDelegate);
}

// --------------------------------------------------------------------------------------------------------------------
