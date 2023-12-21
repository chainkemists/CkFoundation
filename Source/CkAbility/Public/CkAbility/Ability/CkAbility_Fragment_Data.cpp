#include "CkAbility_Fragment_Data.h"

#include "CkAbility/Ability/CkAbility_Utils.h"
#include "CkAbility/AbilityOwner/CkAbilityOwner_Utils.h"

#include "CkCore/Object/CkObject_Utils.h"

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
    Get_CanActivateAbility() const
    -> bool
{
    const auto& AbilityHandle = Get_AbilityHandle();
    if (UCk_Utils_Ability_UE::Get_Status(AbilityHandle) == ECk_Ability_Status::Active)
    { return false; }

    return DoGet_CanActivateAbility();
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
    Self_Request_ActivateAbility(
        const UCk_Ability_Script_PDA* InScript,
        const FCk_Ability_ActivationPayload& InActivationPayload)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InScript),
        TEXT("AbilityScript is [{}]. Was this Ability GC'ed and this function is being called in a latent node?"),
        InScript)
    { return; }

    CK_ENSURE_IF_NOT(ck::IsValid(InScript->_AbilityHandle),
        TEXT("AbilityHandle is INVALID. It's possible that this was not set correctly by the Processor that Gives the Ability.{}"),
        ck::Context(InScript))
    { return; }

    UCk_Utils_AbilityOwner_UE::Request_TryActivateAbility(InScript->Get_AbilityOwnerHandle(),
        FCk_Request_AbilityOwner_ActivateAbility{InScript->Get_AbilityHandle(), InActivationPayload});
}

auto
    UCk_Ability_Script_PDA::
    Self_Request_DeactivateAbility(const UCk_Ability_Script_PDA* InScript)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InScript),
        TEXT("AbilityScript is [{}]. Was this Ability GC'ed and this function is being called in a latent node?"),
        InScript)
    { return; }

    CK_ENSURE_IF_NOT(ck::IsValid(InScript->_AbilityHandle),
        TEXT("AbilityHandle is INVALID. It's possible that this was not set correctly by the Processor that Gives the Ability.{}"),
        ck::Context(InScript))
    { return; }

    UCk_Utils_AbilityOwner_UE::Request_DeactivateAbility(InScript->Get_AbilityOwnerHandle(),
        FCk_Request_AbilityOwner_DeactivateAbility{InScript->Get_AbilityHandle()});
}

auto
    UCk_Ability_Script_PDA::
    Self_Request_TrackTask(
        UCk_Ability_Script_PDA* InScript,
        UObject* InTask)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InScript),
        TEXT("AbilityScript is [{}]. Was this Ability GC'ed and this function is being called in a latent node?"),
        InScript)
    { return; }

    if (ck::Is_NOT_Valid(InTask))
    { return; }

    InScript->_Tasks.Emplace(InTask);
}

auto
    UCk_Ability_Script_PDA::
    Self_Get_Status(
        const UCk_Ability_Script_PDA* InScript)
    -> ECk_Ability_Status
{
    CK_ENSURE_IF_NOT(ck::IsValid(InScript),
        TEXT("AbilityScript is [{}]. Was this Ability GC'ed and this function is being called in a latent node?"),
        InScript)
    { return {}; }

    CK_ENSURE_IF_NOT(ck::IsValid(InScript->_AbilityHandle),
        TEXT("AbilityHandle is INVALID. It's possible that this was not set correctly by the Processor that Gives the Ability.{}"),
        ck::Context(InScript))
    { return {}; }

    const auto& AbilityHandle = InScript->Get_AbilityHandle();
    return UCk_Utils_Ability_UE::Get_Status(AbilityHandle);
}

auto
    UCk_Ability_Script_PDA::
    Self_Get_AbilityEntity(
        const UCk_Ability_Script_PDA* InScript)
    -> FCk_Handle
{
    CK_ENSURE_IF_NOT(ck::IsValid(InScript),
        TEXT("AbilityScript is [{}]. Was this Ability GC'ed and this function is being called in a latent node?"),
        InScript)
    { return {}; }

    CK_ENSURE_IF_NOT(ck::IsValid(InScript->_AbilityHandle),
        TEXT("AbilityHandle is INVALID. It's possible that this was not set correctly by the Processor that Gives the Ability.{}"),
        ck::Context(InScript))
    { return {}; }

    return InScript->_AbilityHandle;
}

auto
    UCk_Ability_Script_PDA::
    Self_Get_AbilityOwnerEntity(
        const UCk_Ability_Script_PDA* InScript)
    -> FCk_Handle
{
    CK_ENSURE_IF_NOT(ck::IsValid(InScript),
        TEXT("AbilityScript is [{}]. Was this Ability GC'ed and this function is being called in a latent node?"),
        InScript)
    { return {}; }

    CK_ENSURE_IF_NOT(ck::IsValid(InScript->_AbilityOwnerHandle),
        TEXT("AbilityOwnerHandle is INVALID. It's possible that this was not set correctly by the Processor that Gives the Ability.{}"),
        ck::Context(InScript))
    { return {}; }

    return InScript->_AbilityOwnerHandle;
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
        const FCk_Handle& InHandle)
    -> void
{
    UCk_Utils_Ability_UE::DoAdd(InHandle, Get_AbilityParams());
}

auto
    UCk_Ability_EntityConfig_PDA::
    DoGet_EntityConstructionScript() const
    -> UCk_Entity_ConstructionScript_PDA*
{
    return _EntityConstructionScript;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Ability_Script_PDA::
    DoOnDeactivateAbility_Implementation()
    -> void
{
}

auto
    UCk_Ability_Script_PDA::
    DoOnActivateAbility_Implementation(
        const FCk_Ability_ActivationPayload& InActivationPayload)
    -> void
{
}

auto
    UCk_Ability_Script_PDA::
    DoOnGiveAbility_Implementation()
    -> void
{
}

auto
    UCk_Ability_Script_PDA::
    DoOnRevokeAbility_Implementation()
    -> void
{
}

auto
    UCk_Ability_Script_PDA::
    DoGet_CanActivateAbility_Implementation() const
    -> bool
{
    return true;
}

// --------------------------------------------------------------------------------------------------------------------
