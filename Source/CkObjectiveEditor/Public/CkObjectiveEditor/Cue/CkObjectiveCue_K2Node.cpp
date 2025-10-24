#include "CkObjectiveCue_K2Node.h"

#include "CkObjective/Cue/CkObjectiveCue_Subsystem.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_K2Node_ObjectiveCue::
    Get_CueExecutorSubsystemClass() const
    -> TSubclassOf<UCk_CueExecutor_Subsystem_Base_UE>
{
    return UCk_ObjectiveCueExecutor_Subsystem_UE::StaticClass();
}

auto
    UCk_K2Node_ObjectiveCue::
    Get_CueTagCategory() const
    -> FString
{
    return TEXT("Cue");
}

auto
    UCk_K2Node_ObjectiveCue::
    DoGet_Menu_NodeTitle() const
    -> FText
{
    return CK_UTILS_IO_GET_LOCTEXT(TEXT("UCk_K2Node_VfxCue"), TEXT("[Ck] Execute Objective Cue 🎯"));
}

auto
    UCk_K2Node_ObjectiveCue::
    DoGet_DisplayNodeTitle() const
    -> FText
{
    return CK_UTILS_IO_GET_LOCTEXT(TEXT("UCk_K2Node_VfxCue"), TEXT("[Ck] Execute Objective Cue 🎯"));
}

// --------------------------------------------------------------------------------------------------------------------
