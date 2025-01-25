#include "CkAbility_ProcessorInjector.h"

#include "CkAbility/Ability/CkAbility_Processor.h"
#include "CkAbility/AbilityCue/CkAbilityCue_Processor.h"
#include "CkAbility/AbilityOwner/CkAbilityOwner_Processor.h"
#include "CkAbility/Settings/CkAbility_Settings.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Ability_ProcessorInjector_Teardown::
    DoInjectProcessors(
        EcsWorldType& InWorld)
    -> void
{
    InWorld.Add<ck::FProcessor_Ability_Teardown>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_AbilityOwner_Teardown>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Ability_ProcessorInjector::
    DoInjectProcessors(
        EcsWorldType& InWorld)
        -> void
{
    InWorld.Add<ck::FProcessor_AbilityOwner_ResolvePendingOperationTags>(InWorld.Get_Registry());

#if CK_BUILD_TEST
    if (UCk_Utils_Ability_Settings_UE::Get_LogResolvePendingOperationTags() == ECk_EnableDisable::Enable)
    {
        InWorld.Add<ck::FProcessor_AbilityOwner_ResolvePendingOperationTags_DEBUG>(InWorld.Get_Registry());
    }
#endif

    InWorld.Add<ck::FProcessor_Ability_AddReplicated>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_AbilityOwner_EnsureAllAppended>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_AbilityOwner_Setup>(InWorld.Get_Registry());

    InWorld.Add<ck::FProcessor_AbilityOwner_DeferClientRequestUntilReady>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_AbilityOwner_HandleRequests>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_Ability_HandleRequests>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_AbilityOwner_HandleEvents>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_AbilityOwner_TagsUpdated>(InWorld.Get_Registry());

    InWorld.Add<ck::FProcessor_AbilityCue_Spawn>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------
