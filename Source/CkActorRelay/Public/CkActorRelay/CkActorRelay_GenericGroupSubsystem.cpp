#include "CkActorRelay_GenericGroupSubsystem.h"

#include "CkActorRelay_GenericActor.h"

#include "CkActorRelay/Settings/CkActorRelay_Settings.h"

#include "CkCore/GameplayTag/CkGameplayTag_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ActorRelay_GenericGroup_Subsystem_UE::
    Get_GroupTag() const
    -> FGameplayTag
{
    return UCk_Utils_GameplayTag_UE::ResolveGameplayTag(TEXT("ActorRelay.Generic"));
}

auto
    UCk_ActorRelay_GenericGroup_Subsystem_UE::
    Get_ActorClass() const
    -> TSubclassOf<ACk_ActorRelay_UE>
{
    return ACk_ActorRelay_Generic_UE::StaticClass();
}

auto
    UCk_ActorRelay_GenericGroup_Subsystem_UE::
    Get_OwnershipPolicy() const
    -> ECk_ActorRelay_OwnershipPolicy
{
    return ECk_ActorRelay_OwnershipPolicy::ServerOwned;
}

auto
    UCk_ActorRelay_GenericGroup_Subsystem_UE::
    Get_ChannelCount() const
    -> int32
{
    return UCk_Utils_ActorRelay_Settings_UE::Get_GenericChannelCount();
}

auto
    UCk_ActorRelay_GenericGroup_Subsystem_UE::
    Get_SelectionAlgorithm() const
    -> ECk_ActorRelay_SelectionAlgorithm
{
    return ECk_ActorRelay_SelectionAlgorithm::RoundRobin;
}

// --------------------------------------------------------------------------------------------------------------------
