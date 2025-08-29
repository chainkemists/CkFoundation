// CkAudioCueSubsystem.cpp
#include "CkAudioCue_Subsystem.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_AudioCueReplicator_Subsystem_UE::
    Get_CueSubsystem() const
    -> UCk_CueSubsystem_Base_UE*
{
    return GEngine->GetEngineSubsystem<UCk_AudioCueSubsystem_UE>();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_AudioCueSubsystem_UE::
    Get_CueBaseClass() const -> TSubclassOf<UCk_CueBase_EntityScript>
{
    return UCk_AudioCue_EntityScript::StaticClass();
}

// --------------------------------------------------------------------------------------------------------------------