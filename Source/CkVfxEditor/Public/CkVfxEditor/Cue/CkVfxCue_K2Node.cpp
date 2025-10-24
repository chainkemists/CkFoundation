#include "CkVfxCue_K2Node.h"

#include "CkVfx/Cue/CkVfxCue_Subsystem.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_K2Node_VfxCue::
    Get_CueExecutorSubsystemClass() const
    -> TSubclassOf<UCk_CueExecutor_Subsystem_Base_UE>
{
    return UCk_VfxCueExecutor_Subsystem_UE::StaticClass();
}

auto
    UCk_K2Node_VfxCue::
    Get_CueTagCategory() const
    -> FString
{
    return TEXT("Cue");
}

auto
    UCk_K2Node_VfxCue::
    DoGet_Menu_NodeTitle() const
    -> FText
{
    return CK_UTILS_IO_GET_LOCTEXT(TEXT("UCk_K2Node_VfxCue"), TEXT("[Ck] Execute VFX Cue ✨"));
}

auto
    UCk_K2Node_VfxCue::
    DoGet_DisplayNodeTitle() const
    -> FText
{
    return CK_UTILS_IO_GET_LOCTEXT(TEXT("UCk_K2Node_VfxCue"), TEXT("[Ck] Execute VFX Cue ✨"));
}

// --------------------------------------------------------------------------------------------------------------------
