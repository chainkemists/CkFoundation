#include "CkAudioCue_K2Node.h"

#include "CkAudio/Cue/CkAudioCue_Subsystem.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_K2Node_AudioCue::
    Get_CueExecutorSubsystemClass() const
    -> TSubclassOf<UCk_CueExecutor_Subsystem_Base_UE>
{
    return UCk_AudioCueExecutor_Subsystem_UE::StaticClass();
}

auto
    UCk_K2Node_AudioCue::
    Get_CueTagCategory() const
    -> FString
{
    return TEXT("Cue");
}

auto
    UCk_K2Node_AudioCue::
    DoGet_Menu_NodeTitle() const
    -> FText
{
    return CK_UTILS_IO_GET_LOCTEXT(TEXT("UCk_K2Node_AudioCue"), TEXT("[Ck] Execute Audio Cue ♪"));
}

auto
    UCk_K2Node_AudioCue::
    DoGet_DisplayNodeTitle() const
    -> FText
{
    return CK_UTILS_IO_GET_LOCTEXT(TEXT("UCk_K2Node_AudioCue"), TEXT("[Ck] Execute Audio Cue ♪"));
}

// --------------------------------------------------------------------------------------------------------------------
