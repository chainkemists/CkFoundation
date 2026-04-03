#include "CkVfxCue_Subsystem.h"

#include "CkCore/GameplayTag/CkGameplayTag_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_VfxCueExecutor_Subsystem_UE::
    Get_GroupTag() const
    -> FGameplayTag
{
    return UCk_Utils_GameplayTag_UE::ResolveGameplayTag(TEXT("ActorRelay.Cue.Vfx"));
}

auto
    UCk_VfxCueExecutor_Subsystem_UE::
    Get_CueSubsystemClass() const
    -> TSubclassOf<UCk_CueSubsystem_Base_UE>
{
    return UCk_VfxCueSubsystem_UE::StaticClass();
}

auto
    UCk_VfxCueExecutor_Subsystem_UE::
    Get_DedicatedServerPolicy() const
    -> ECk_Cue_DedicatedServerPolicy
{
    return ECk_Cue_DedicatedServerPolicy::CosmeticOnly;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_VfxCueSubsystem_UE::
    Get_CueBaseClass() const
    -> TSubclassOf<UCk_CueBase_EntityScript>
{
    return UCk_VfxCueBase_EntityScript::StaticClass();
}

// --------------------------------------------------------------------------------------------------------------------
