#include "CkAbility_Settings.h"

#include "CkCore/Object/CkObject_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Ability_Settings_UE::
    Get_AbilityRecyclingPolicy()
    -> ECk_Ability_RecyclingPolicy
{
    return UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Ability_ProjectSettings_UE>()->Get_AbilityRecyclingPolicy();
}

// --------------------------------------------------------------------------------------------------------------------
