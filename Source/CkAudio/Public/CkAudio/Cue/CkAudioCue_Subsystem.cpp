#include "CkAudioCue_Subsystem.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_AudioCueExecutor_Subsystem_UE::
    Get_CueSubsystemClass() const
    -> TSubclassOf<UCk_CueSubsystem_Base_UE>
{
    return UCk_AudioCueSubsystem_UE::StaticClass();
}

auto
    UCk_AudioCueExecutor_Subsystem_UE::
    Get_DedicatedServerPolicy() const
    -> ECk_Cue_DedicatedServerPolicy
{
    return ECk_Cue_DedicatedServerPolicy::CosmeticOnly;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_AudioCueSubsystem_UE::
    Get_CueBaseClass() const
    -> TSubclassOf<UCk_CueBase_EntityScript>
{
    return UCk_AudioCue_EntityScript::StaticClass();
}

// --------------------------------------------------------------------------------------------------------------------