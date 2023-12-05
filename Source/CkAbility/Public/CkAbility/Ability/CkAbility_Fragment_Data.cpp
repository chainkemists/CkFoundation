#include "CkAbility_Fragment_Data.h"

#include "CkAbility/Ability/CkAbility_Utils.h"

#include "CkCore/Object/CkObject_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Ability_Script_PDA::
    OnActivateAbility()
    -> void
{
    DoOnActivateAbility();
}

auto
    UCk_Ability_Script_PDA::
    OnEndAbility()
    -> void
{
    DoOnEndAbility();
}

FCk_Fragment_Ability_ParamsData::
    FCk_Fragment_Ability_ParamsData(
        const TSubclassOf<UCk_Ability_Script_PDA>& InAbilityScript)
    : _AbilityScriptClass(InAbilityScript)
{
    std::ignore = Get_Data();
}

auto
    FCk_Fragment_Ability_ParamsData::
    Get_Data() const
    -> const FCk_Ability_Script_Data&
{
    if (ck::Is_NOT_Valid(_Data))
    {
        const auto& AbilityScriptCDO = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Ability_Script_PDA>(_AbilityScriptClass);
        _Data = AbilityScriptCDO->Get_Data();
    }

    return *_Data;
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
