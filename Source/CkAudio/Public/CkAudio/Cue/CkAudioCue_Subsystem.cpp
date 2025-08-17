// CkAudioCueSubsystem.cpp
#include "CkAudioCue_Subsystem.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_AudioCueSubsystem_UE::
    Get_CueBaseClass() const -> TSubclassOf<UCk_CueBase_EntityScript>
{
    return UCk_AudioCue_EntityScript::StaticClass();
}

// --------------------------------------------------------------------------------------------------------------------