// CkAudioCueSubsystem.cpp
#include "CkAudioCue_Subsystem.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_AudioCueExecutor_Subsystem_UE::
    Get_CueSubsystemClass() const
    -> TSubclassOf<UCk_CueSubsystem_Base_UE>
{
    return UCk_AudioCueSubsystem_UE::StaticClass();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_AudioCueSubsystem_UE::
    Get_CueBaseClass() const -> TSubclassOf<UCk_CueBase_EntityScript>
{
    return UCk_AudioCue_EntityScript::StaticClass();
}

// --------------------------------------------------------------------------------------------------------------------