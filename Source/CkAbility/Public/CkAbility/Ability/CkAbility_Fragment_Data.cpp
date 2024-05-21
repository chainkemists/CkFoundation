#include "CkAbility_Fragment_Data.h"

#include "CkAbility/Ability/CkAbility_Utils.h"
#include "CkAbility/AbilityCue/CkAbilityCue_Utils.h"
#include "CkAbility/Ability/CkAbility_Script.h"
#include "CkAbility/AbilityOwner/CkAbilityOwner_Utils.h"

#include "CkCore/Object/CkObject_Utils.h"

#include <UObject/ObjectSaveContext.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Fragment_Ability_ParamsData::
    Get_Data() const
    -> const FCk_Ability_Script_Data&
{
    if (ck::IsValid(_AbilityArchetype))
    { return _AbilityArchetype->Get_Data(); }

    const auto& AbilityScriptCDO = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Ability_Script_PDA>(_AbilityScriptClass);

    CK_ENSURE_IF_NOT(ck::IsValid(AbilityScriptCDO), TEXT("Could not retrieve valid CDO for Ability Script Class [{}]"), _AbilityScriptClass)
    {
        static FCk_Ability_Script_Data Invalid;
        return Invalid;
    }

    return AbilityScriptCDO->Get_Data();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Ability_ConstructionScript_PDA::
    DoConstruct_Implementation(
        FCk_Handle& InHandle,
        const FInstancedStruct& InOptionalParams) const
    -> void
{
    UCk_Utils_Ability_UE::DoAdd(InHandle, Get_AbilityParams());

    if (_DefaultAbilities_Instanced.IsEmpty())
    { return; }

    const auto AbilityOwnerParamsData = FCk_Fragment_AbilityOwner_ParamsData{}
                                        .Set_DefaultAbilities_Instanced(_DefaultAbilities_Instanced);

    UCk_Utils_AbilityOwner_UE::Add(InHandle, AbilityOwnerParamsData);
}

auto
    UCk_Ability_EntityConfig_PDA::
    DoGet_EntityConstructionScript() const
    -> UCk_Entity_ConstructionScript_PDA*
{
    return _EntityConstructionScript;
}

// --------------------------------------------------------------------------------------------------------------------
