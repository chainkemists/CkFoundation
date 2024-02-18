#include "CkAbilityProcessorInjector.h"

#include "CkAbility/AbilityCue/CkAbilityCue_Processor.h"
#include "CkAbility/AbilityOwner/CkAbilityOwner_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Ability_ProcessorInjector::
    DoInjectProcessors(
        EcsWorldType& InWorld)
        -> void
{
    InWorld.Add<ck::FProcessor_AbilityOwner_Setup>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_AbilityOwner_HandleRequests>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_AbilityOwner_HandleEvents>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_AbilityOwner_Teardown>(InWorld.Get_Registry());

    InWorld.Add<ck::FProcessor_AbilityCue_Spawn>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------
