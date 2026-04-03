#include "CkObjectiveCue_Subsystem.h"

#include "CkCore/GameplayTag/CkGameplayTag_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ObjectiveCueExecutor_Subsystem_UE::
    Get_GroupTag() const
    -> FGameplayTag
{
    return UCk_Utils_GameplayTag_UE::ResolveGameplayTag(TEXT("ActorRelay.Cue.Objective"));
}

auto
    UCk_ObjectiveCueExecutor_Subsystem_UE::
    Get_CueSubsystemClass() const
    -> TSubclassOf<UCk_CueSubsystem_Base_UE>
{
    return UCk_ObjectiveCueSubsystem_UE::StaticClass();
}

auto
    UCk_ObjectiveCueExecutor_Subsystem_UE::
    Get_DedicatedServerPolicy() const
    -> ECk_Cue_DedicatedServerPolicy
{
    return ECk_Cue_DedicatedServerPolicy::GameplayRelevant;
}

// --------------------------------------------------------------------------------------------------------------------
auto
    UCk_ObjectiveCueSubsystem_UE::
    Get_CueBaseClass() const
    -> TSubclassOf<UCk_CueBase_EntityScript>
{
    return UCk_ObjectiveCue_EntityScript::StaticClass();
}

// --------------------------------------------------------------------------------------------------------------------