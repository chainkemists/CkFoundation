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

auto
    UCk_Utils_Ability_Settings_UE::
    Get_CueTypes()
    -> const TArray<TSubclassOf<UCk_Ability_Script_PDA>>&
{
    return UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Ability_ProjectSettings_UE>()->Get_CueTypes();
}

auto
    UCk_Utils_Ability_Settings_UE::
    Get_AbilityNotActivatedDebug()
    -> const ECk_EnableDisable
{
    return UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Ability_UserSettings_UE>()->Get_AbilityNotActivatedDebug();
}

auto
    UCk_Utils_Ability_Settings_UE::
    Get_NumberOfCueReplicators()
    -> int32
{
    return UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Ability_ProjectSettings_UE>()->Get_NumberOfCueReplicators();
}

// --------------------------------------------------------------------------------------------------------------------
