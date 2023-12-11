#include "CkAbility_Fragment_Data.h"

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

// --------------------------------------------------------------------------------------------------------------------

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
