#include "CkAbility_Fragment_Data.h"

#include "CkAbility/Ability/CkAbility_Utils.h"

#include "CkCore/Object/CkObject_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Ability_Script_PDA::
    OnActivateAbility()
    -> void
{
    const auto AbilityOwnerEntity = Get_AbilityOwnerHandle();

    switch(const auto ExecutionPolicy = Get_Data().Get_NetworkSettings().Get_ExecutionPolicy())
    {
        case ECk_Net_NetExecutionPolicy::PreferHost:
        {
            if (UCk_Utils_Net_UE::Get_IsEntityNetMode_Host(AbilityOwnerEntity))
            {
                DoOnActivateAbility();
            }
            break;
        }
        case ECk_Net_NetExecutionPolicy::LocalAndHost:
        {
            if (UCk_Utils_Net_UE::Get_HasAuthority(AbilityOwnerEntity) ||
                UCk_Utils_Net_UE::Get_IsEntityNetMode_Host(AbilityOwnerEntity))
            {
                DoOnActivateAbility();
            }
            break;
        }
        case ECk_Net_NetExecutionPolicy::All:
        {
            DoOnActivateAbility();
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
    OnEndAbility()
    -> void
{
    DoOnEndAbility();
}

auto
    UCk_Ability_Script_PDA::
    Get_AbilityEntity(const UCk_Ability_Script_PDA* InScript)
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
    Get_AbilityOwnerEntity(const UCk_Ability_Script_PDA* InScript)
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
    -> UCk_EntityBridge_ConstructionScript_PDA*
{
    return _EntityConstructionScript;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Ability_Script_PDA::
    DoOnEndAbility_Implementation()
    -> void
{
}

auto
    UCk_Ability_Script_PDA::
    DoOnActivateAbility_Implementation()
    -> void
{
}

// --------------------------------------------------------------------------------------------------------------------
