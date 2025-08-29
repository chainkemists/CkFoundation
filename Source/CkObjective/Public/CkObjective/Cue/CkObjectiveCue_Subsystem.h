#pragma once

#include "CkCue/CkCueSubsystem_Base.h"
#include "CkObjectiveCue_EntityScript.h"

#include "CkObjectiveCue_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(DisplayName = "CkSubsystem_ObjectiveCueExecutor", NotBlueprintable, BlueprintType)
class CKOBJECTIVE_API UCk_ObjectiveCueExecutor_Subsystem_UE : public UCk_CueExecutor_Subsystem_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_ObjectiveCueExecutor_Subsystem_UE);

protected:
    auto Get_CueSubsystemClass() const -> TSubclassOf<UCk_CueSubsystem_Base_UE> override;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(DisplayName = "CkSubsystem_ObjectiveCue", NotBlueprintable, NotBlueprintType)
class CKOBJECTIVE_API UCk_ObjectiveCueSubsystem_UE : public UCk_CueSubsystem_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_ObjectiveCueSubsystem_UE);

protected:
    auto Get_CueBaseClass() const -> TSubclassOf<UCk_CueBase_EntityScript> override;
};

// --------------------------------------------------------------------------------------------------------------------