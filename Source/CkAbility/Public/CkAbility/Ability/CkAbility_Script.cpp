#include "CkAbility_Script.h"

#include "CkAbility/Ability/CkAbility_Utils.h"
#include "CkAbility/AbilityCue/CkAbilityCue_Utils.h"
#include "CkAbility/AbilityOwner/CkAbilityOwner_Utils.h"
#include "CkAbility/Settings/CkAbility_Settings.h"

#include "CkNet/CkNet_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Ability_Script_PDA::
    OnActivateAbility(
        const FCk_Ability_ActivationPayload& InActivationPayload)
    -> void
{
    const auto AbilityOwnerEntity = Get_AbilityOwnerHandle();

    switch(const auto ExecutionPolicy = Get_Data().Get_NetworkSettings().Get_ExecutionPolicy())
    {
        case ECk_Net_NetExecutionPolicy::PreferHost:
        {
            if (UCk_Utils_Net_UE::Get_IsEntityNetMode_Host(AbilityOwnerEntity))
            {
                DoOnActivateAbility(InActivationPayload);
            }
            break;
        }
        case ECk_Net_NetExecutionPolicy::LocalAndHost:
        {
            if (UCk_Utils_Net_UE::Get_HasAuthority(AbilityOwnerEntity) ||
                UCk_Utils_Net_UE::Get_IsEntityNetMode_Host(AbilityOwnerEntity))
            {
                DoOnActivateAbility(InActivationPayload);
            }
            break;
        }
        case ECk_Net_NetExecutionPolicy::All:
        {
            DoOnActivateAbility(InActivationPayload);
            break;
        }
        default:
        {
            CK_INVALID_ENUM(ExecutionPolicy);
            break;
        }
    }
}

auto
    UCk_Ability_Script_PDA::
    OnDeactivateAbility()
    -> void
{
    DoOnDeactivateAbility();
}

auto
    UCk_Ability_Script_PDA::
    OnGiveAbility()
    -> void
{
    DoOnGiveAbility();
}

auto
    UCk_Ability_Script_PDA::
    OnRevokeAbility()
    -> void
{
    DoOnRevokeAbility();
}

auto
    UCk_Ability_Script_PDA::
    DoRequest_ActivateAbility(
        FCk_Ability_ActivationPayload InActivationPayload)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(Get_AbilityHandle()),
        TEXT("AbilityHandle is INVALID. It's possible that this was not set correctly by the Processor that Gives the Ability.{}"),
        ck::Context(this))
    { return; }

    UCk_Utils_AbilityOwner_UE::Request_TryActivateAbility(_AbilityOwnerHandle,
        FCk_Request_AbilityOwner_ActivateAbility{Get_AbilityHandle(), InActivationPayload});
}

auto
    UCk_Ability_Script_PDA::
    DoRequest_DeactivateAbility()
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(Get_AbilityHandle()),
        TEXT("AbilityHandle is INVALID. It's possible that this was not set correctly by the Processor that Gives the Ability.{}"),
        ck::Context(this))
    { return; }

    UCk_Utils_AbilityOwner_UE::Request_DeactivateAbility(_AbilityOwnerHandle,
        FCk_Request_AbilityOwner_DeactivateAbility{Get_AbilityHandle()});
}

auto
    UCk_Ability_Script_PDA::
    DoRequest_ApplyCost()
    -> void
{
    CK_ENSURE_IF_NOT(UCk_Utils_AbilityOwner_UE::Has(Get_AbilityHandle()),
        TEXT("Ability Entity [{}] with AbilityScript [{}] is NOT an AbilityOwner itself. Did you forget to add a 'Cost' Ability to the aforementioned?"),
        Get_AbilityHandle(), this)
    { return; }

    const auto& DefaultApplyCostTag = UCk_Utils_Ability_Settings_UE::Get_Default_ApplyCostTag();

    auto AbilityAsAbilityOwner = UCk_Utils_AbilityOwner_UE::Conv_HandleToAbilityOwner(_AbilityHandle);
    UCk_Utils_AbilityOwner_UE::Request_SendAbilityEvent
    (
        AbilityAsAbilityOwner,
        FCk_Request_AbilityOwner_SendEvent
        {
            FCk_AbilityOwner_Event{DefaultApplyCostTag}
                .Set_ContextEntity(Get_AbilityHandle())
        }
    );
}

auto
    UCk_Ability_Script_PDA::
    DoRequest_ApplyCost_OnOwner()
    -> void
{
    const auto& DefaultApplyCostTag = UCk_Utils_Ability_Settings_UE::Get_Default_ApplyCostTag();

    UCk_Utils_AbilityOwner_UE::Request_SendAbilityEvent
    (
        _AbilityOwnerHandle,
        FCk_Request_AbilityOwner_SendEvent
        {
            FCk_AbilityOwner_Event{DefaultApplyCostTag}
                .Set_ContextEntity(Get_AbilityHandle())
        }
    );
}

auto
    UCk_Ability_Script_PDA::
    DoRequest_ApplyCooldown()
    -> void
{
    CK_ENSURE_IF_NOT(UCk_Utils_AbilityOwner_UE::Has(Get_AbilityHandle()),
        TEXT("Ability Entity [{}] with AbilityScript [{}] is NOT an AbilityOwner itself. Did you forget to add a 'Cooldown' Ability to the aforementioned?"),
        Get_AbilityHandle(), this)
    { return; }

    const auto& DefaultApplyCooldownTag = UCk_Utils_Ability_Settings_UE::Get_Default_ApplyCooldownTag();

    auto AbilityAsAbilityOwner = UCk_Utils_AbilityOwner_UE::Conv_HandleToAbilityOwner(_AbilityHandle);
    UCk_Utils_AbilityOwner_UE::Request_SendAbilityEvent
    (
        AbilityAsAbilityOwner,
        FCk_Request_AbilityOwner_SendEvent
        {
            FCk_AbilityOwner_Event{DefaultApplyCooldownTag}
                .Set_ContextEntity(Get_AbilityHandle())
        }
    );
}

auto
    UCk_Ability_Script_PDA::
    DoRequest_ApplyCooldown_OnOwner()
    -> void
{
    const auto& DefaultApplyCooldownTag = UCk_Utils_Ability_Settings_UE::Get_Default_ApplyCooldownTag();

    UCk_Utils_AbilityOwner_UE::Request_SendAbilityEvent
    (
        _AbilityOwnerHandle,
        FCk_Request_AbilityOwner_SendEvent
        {
            FCk_AbilityOwner_Event{DefaultApplyCooldownTag}
                .Set_ContextEntity(Get_AbilityHandle())
        }
    );
}

auto
    UCk_Ability_Script_PDA::
    DoRequest_TrackTask(
        UObject* InTask)
    -> void
{
    if (ck::Is_NOT_Valid(InTask))
    { return; }

    _Tasks.Emplace(InTask);
}

auto
    UCk_Ability_Script_PDA::
    DoRequest_SpawnAbilityCue(
        const FCk_AbilityCue_Params&  InReplicatedParams,
        FGameplayTag InAbilityCueName) const
    -> void
{
    UCk_Utils_AbilityCue_UE::Request_Spawn_AbilityCue
    (
        Get_AbilityHandle(),
        FCk_Request_AbilityCue_Spawn{InAbilityCueName, const_cast<UCk_Ability_Script_PDA*>(this)}
            .Set_ReplicatedParams(InReplicatedParams)
    );
}

auto
    UCk_Ability_Script_PDA::
    DoGet_Status() const
    -> ECk_Ability_Status
{
    CK_ENSURE_IF_NOT(ck::IsValid(Get_AbilityHandle()),
        TEXT("AbilityHandle is INVALID. It's possible that this was not set correctly by the Processor that Gives the Ability.{}"),
        ck::Context(this))
    { return {}; }

    return UCk_Utils_Ability_UE::Get_Status(Get_AbilityHandle());
}

auto
    UCk_Ability_Script_PDA::
    DoGet_AbilityEntity() const
    -> FCk_Handle_Ability
{
    CK_ENSURE_IF_NOT(ck::IsValid(Get_AbilityHandle()),
        TEXT("AbilityHandle is INVALID. It's possible that this was not set correctly by the Processor that Gives the Ability.{}"),
        ck::Context(this))
    { return {}; }

    return Get_AbilityHandle();
}

auto
    UCk_Ability_Script_PDA::
    DoGet_AbilityOwnerEntity() const
    -> FCk_Handle_AbilityOwner
{
    CK_ENSURE_IF_NOT(ck::IsValid(Get_AbilityOwnerHandle()),
        TEXT("AbilityOwnerHandle is INVALID. It's possible that this was not set correctly by the Processor that Gives the Ability.{}"),
        ck::Context(this))
    { return {}; }

    return Get_AbilityOwnerHandle();
}

// --------------------------------------------------------------------------------------------------------------------
