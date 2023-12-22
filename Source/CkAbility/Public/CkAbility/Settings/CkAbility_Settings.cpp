#include "CkAbility_Settings.h"

#include "CkCore/Object/CkObject_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Ability_ProjectSettings_UE::
    Get_AbilityRecyclingPolicy()
    -> ECk_Ability_RecyclingPolicy
{
    return UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Ability_ProjectSettings_UE>()->Get_AbilityRecyclingPolicy();
}

auto
    UCk_Utils_Ability_ProjectSettings_UE::
    Get_Default_ApplyCooldownTag()
    -> FGameplayTag
{
    const auto& Tag = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Ability_ProjectSettings_UE>()->Get_ApplyCooldownTag();;

    CK_ENSURE_IF_NOT(ck::IsValid(Tag),
        TEXT("Default ApplyCooldownTag NOT populated in Project Settings. Cooldowns may not work correctly."))
    { return {}; }

    return Tag;
}

auto
    UCk_Utils_Ability_ProjectSettings_UE::
    Get_Default_ApplyCostTag()
    -> FGameplayTag
{
    const auto& Tag = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Ability_ProjectSettings_UE>()->Get_ApplyCostTag();

    CK_ENSURE_IF_NOT(ck::IsValid(Tag),
        TEXT("Default ApplyCostTag NOT populated in Project Settings. Costs may not work correctly."))
    { return {}; }

    return Tag;
}

auto
    UCk_Utils_Ability_ProjectSettings_UE::
    Get_Default_ConditionsNotMetTag()
    -> FGameplayTag
{
    const auto& Tag = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Ability_ProjectSettings_UE>()->Get_ConditionsNotMetTag();

    CK_ENSURE_IF_NOT(ck::IsValid(Tag),
        TEXT("Default ConditionsNotMetTag NOT populated in Project Settings. ConditionsNotMet Abilities may not work correctly."))
    { return {}; }

    return Tag;
}

// --------------------------------------------------------------------------------------------------------------------
