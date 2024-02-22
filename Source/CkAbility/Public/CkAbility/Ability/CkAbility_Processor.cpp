#include "CkAbility_Processor.h"

#include "CkAbility/CkAbility_Log.h"
#include "CkAbility/Ability/CkAbility_Utils.h"
#include "CkAbility/AbilityOwner/CkAbilityOwner_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_Ability_Teardown::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType& InHandle,
            const FFragment_Ability_Current& InCurrent) const
        -> void
    {
        // there is nothing to teardown if the Ability is already Inactive
        if (InCurrent.Get_Status() == ECk_Ability_Status::NotActive)
        { return; }

        auto LifetimeOwner = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InHandle);
        auto AbilityOwner = UCk_Utils_AbilityOwner_UE::CastChecked(LifetimeOwner);

        ability::Verbose
        (
            TEXT("FORCE DEACTIVATING Ability [Name: {} | Entity: {}] with AbilityOwner [{}] that is {}"),
            UCk_Utils_GameplayLabel_UE::Get_Label(InHandle), InHandle, AbilityOwner,
            ck::IsValid(LifetimeOwner) ? TEXT("VALID") : TEXT("PENDING DESTROY")
        );

        UCk_Utils_Ability_UE::DoDeactivate(AbilityOwner, InHandle);
    }
}

// --------------------------------------------------------------------------------------------------------------------
