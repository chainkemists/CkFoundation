#include "CkAbility_Fragment_Data.h"

#include "CkAbility/Ability/CkAbility_Utils.h"
#include "CkAbility/AbilityCue/CkAbilityCue_Utils.h"
#include "CkAbility/AbilityOwner/CkAbilityOwner_Utils.h"
#include "CkAbility/Settings/CkAbility_Settings.h"

#include "CkCore/EditorOnly/CkEditorOnly_Utils.h"
#include "CkCore/Object/CkObject_Utils.h"

#include <UObject/ObjectSaveContext.h>

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
        FCk_Ability_ActivationPayload InActivationPayload) const
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(Get_AbilityHandle()),
        TEXT("AbilityHandle is INVALID. It's possible that this was not set correctly by the Processor that Gives the Ability.{}"),
        ck::Context(this))
    { return; }

    UCk_Utils_AbilityOwner_UE::Request_TryActivateAbility(Get_AbilityOwnerHandle(),
        FCk_Request_AbilityOwner_ActivateAbility{Get_AbilityHandle(), InActivationPayload});
}

auto
    UCk_Ability_Script_PDA::
    DoRequest_DeactivateAbility() const
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(Get_AbilityHandle()),
        TEXT("AbilityHandle is INVALID. It's possible that this was not set correctly by the Processor that Gives the Ability.{}"),
        ck::Context(this))
    { return; }

    UCk_Utils_AbilityOwner_UE::Request_DeactivateAbility(Get_AbilityOwnerHandle(),
        FCk_Request_AbilityOwner_DeactivateAbility{Get_AbilityHandle()});
}

auto
    UCk_Ability_Script_PDA::
    DoRequest_ApplyCost() const
    -> void
{
    DoRequest_ApplyCustomCost(ck::FAbility_Tags::Get_Cost());
}

auto
    UCk_Ability_Script_PDA::
    DoRequest_ApplyCustomCost(
        FGameplayTag InAbilityCostTag) const
    -> void
{
    CK_ENSURE_IF_NOT(UCk_Utils_AbilityOwner_UE::Has(Get_AbilityHandle()),
        TEXT("Ability Entity [{}] with AbilityScript [{}] is NOT an AbilityOwner itself. Did you forget to add a 'Cost' Ability to the aforementioned?"),
        Get_AbilityHandle(), this)
    { return; }

    const auto& DefaultApplyCostTag = UCk_Utils_Ability_Settings_UE::Get_Default_ApplyCostTag();

    UCk_Utils_AbilityOwner_UE::Request_SendAbilityEvent
    (
        Get_AbilityHandle(),
        FCk_Request_AbilityOwner_SendEvent
        {
            FCk_AbilityOwner_Event{InAbilityCostTag}
                .Set_ContextEntity(Get_AbilityHandle())
        }
    );
}

auto
    UCk_Ability_Script_PDA::
    DoRequest_ApplyCost_OnOwner() const
    -> void
{
    DoRequest_ApplyCustomCost_OnOwner(ck::FAbility_Tags::Get_Cost());
}

auto
    UCk_Ability_Script_PDA::
    DoRequest_ApplyCustomCost_OnOwner(
        FGameplayTag InAbilityCostTag) const
    -> void
{
    UCk_Utils_AbilityOwner_UE::Request_SendAbilityEvent
    (
        Get_AbilityOwnerHandle(),
        FCk_Request_AbilityOwner_SendEvent
        {
            FCk_AbilityOwner_Event{InAbilityCostTag}
                .Set_ContextEntity(Get_AbilityHandle())
        }
    );
}

auto
    UCk_Ability_Script_PDA::
    DoRequest_ApplyCooldown() const
    -> void
{
    DoRequest_ApplyCustomCooldown(ck::FAbility_Tags::Get_Cooldown());
}

auto
    UCk_Ability_Script_PDA::
    DoRequest_ApplyCustomCooldown(
        FGameplayTag InAbilityCooldownTag) const
    -> void
{
    CK_ENSURE_IF_NOT(UCk_Utils_AbilityOwner_UE::Has(Get_AbilityHandle()),
        TEXT("Ability Entity [{}] with AbilityScript [{}] is NOT an AbilityOwner itself. Did you forget to add a 'Cooldown' Ability to the aforementioned?"),
        Get_AbilityHandle(), this)
    { return; }

    const auto& DefaultApplyCooldownTag = UCk_Utils_Ability_Settings_UE::Get_Default_ApplyCooldownTag();

    UCk_Utils_AbilityOwner_UE::Request_SendAbilityEvent
    (
        Get_AbilityHandle(),
        FCk_Request_AbilityOwner_SendEvent
        {
            FCk_AbilityOwner_Event{InAbilityCooldownTag}
                .Set_ContextEntity(Get_AbilityHandle())
        }
    );
}

auto
    UCk_Ability_Script_PDA::
    DoRequest_ApplyCooldown_OnOwner() const
    -> void
{
    DoRequest_ApplyCustomCooldown_OnOwner(ck::FAbility_Tags::Get_Cooldown());
}

auto
    UCk_Ability_Script_PDA::
    DoRequest_ApplyCustomCooldown_OnOwner(
        FGameplayTag InAbilityCooldownTag) const
    -> void
{
    UCk_Utils_AbilityOwner_UE::Request_SendAbilityEvent
    (
        Get_AbilityOwnerHandle(),
        FCk_Request_AbilityOwner_SendEvent
        {
            FCk_AbilityOwner_Event{InAbilityCooldownTag}
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
    -> FCk_Handle
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
    -> FCk_Handle
{
    CK_ENSURE_IF_NOT(ck::IsValid(Get_AbilityOwnerHandle()),
        TEXT("AbilityOwnerHandle is INVALID. It's possible that this was not set correctly by the Processor that Gives the Ability.{}"),
        ck::Context(this))
    { return {}; }

    return Get_AbilityOwnerHandle();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Fragment_Ability_ParamsData::
    Get_Data() const
    -> const FCk_Ability_Script_Data&
{
    const auto& AbilityScriptCDO = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Ability_Script_PDA>(_AbilityScriptClass);
    return AbilityScriptCDO->Get_Data();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Ability_ConstructionScript_PDA::
    DoConstruct_Implementation(
        const FCk_Handle& InHandle,
        const FInstancedStruct& InOptionalParams)
    -> void
{
    UCk_Utils_Ability_UE::DoAdd(InHandle, Get_AbilityParams());

    if (NOT _DefaultAbilityEntityConfigs.IsEmpty())
    {
        UCk_Utils_AbilityOwner_UE::Add(InHandle, FCk_Fragment_AbilityOwner_ParamsData{_DefaultAbilityEntityConfigs});
    }
}

auto
    UCk_Ability_EntityConfig_PDA::
    DoGet_EntityConstructionScript() const
    -> UCk_Entity_ConstructionScript_PDA*
{
    return _EntityConstructionScript;
}

// --------------------------------------------------------------------------------------------------------------------
