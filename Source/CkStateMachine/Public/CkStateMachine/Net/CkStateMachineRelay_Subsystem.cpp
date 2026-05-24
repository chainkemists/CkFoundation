#include "CkStateMachineRelay_Subsystem.h"

#include "CkStateMachine/Net/CkStateMachineRelay_Actor.h"

#include "CkCore/GameplayTag/CkGameplayTag_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_StateMachineRelay_Subsystem_UE::
    Get_GroupTag() const
    -> FGameplayTag
{
    return UCk_Utils_GameplayTag_UE::ResolveGameplayTag(TEXT("ActorRelay.StateMachine"));
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_StateMachineRelay_Subsystem_UE::
    Get_ActorClass() const
    -> TSubclassOf<ACk_ActorRelay_UE>
{
    return ACk_StateMachineRelay_UE::StaticClass();
}

// --------------------------------------------------------------------------------------------------------------------
