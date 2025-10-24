#pragma once

#include "CkCueEditor/Cue/CkCue_K2Node.h"

#include "CkObjectiveCue_K2Node.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(MinimalAPI)
class UCk_K2Node_ObjectiveCue : public UCk_K2Node_Cue_Base
{
    GENERATED_BODY()

protected:
    auto Get_CueExecutorSubsystemClass() const -> TSubclassOf<UCk_CueExecutor_Subsystem_Base_UE> override;
    auto Get_CueTagCategory() const -> FString override;
    auto DoGet_Menu_NodeTitle() const -> FText override;
    auto DoGet_DisplayNodeTitle() const -> FText override;
};

// --------------------------------------------------------------------------------------------------------------------
