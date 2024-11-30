#include "CkAbilityExt_Fragment_Data.h"

#include "CkAbility/Ability/CkAbility_Fragment.h"
#include "CkAbility/AbilityOwner/CkAbilityOwner_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_GrantAbilityOwnerTagsWhenGiven_Ability_Trait_UE::
    DoOnOwningAbilityGiven_Implementation(
        FCk_Handle_Ability InAbility,
        FCk_Handle_AbilityOwner InAbilityOwner) -> void
{
    auto& AbilityOwnerComp = InAbilityOwner.Get<ck::FFragment_AbilityOwner_Current>();
    AbilityOwnerComp.AppendTags(InAbilityOwner, Get_TagsToGrant());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_DisplayInfo_Ability_Trait_UE::
    DoOnOwningAbilityCreated_Implementation(
        FCk_Handle_Ability InAbility)
    -> void
{
    InAbility.Add<ck::FFragment_Ability_DisplayInfo>(_DisplayName);
}

// --------------------------------------------------------------------------------------------------------------------

