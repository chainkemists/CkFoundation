// CkAudioCueSubsystem.h
#pragma once

#include "CkCue/CkCueSubsystem_Base.h"
#include "CkAudio/Cue/CkAudioCue_EntityScript.h"

#include "CkAudioCue_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(DisplayName = "CkSubsystem_AudioCue")
class CKAUDIO_API UCk_AudioCueSubsystem_UE : public UCk_CueSubsystem_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_AudioCueSubsystem_UE);

protected:
    auto Get_CueBaseClass() const -> TSubclassOf<UCk_CueBase_EntityScript> override;
};

// --------------------------------------------------------------------------------------------------------------------