#pragma once

#include "CkMacros/CkMacros.h"

#include "CkEnsure_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(DisplayName = "Ensure Subsystem")
class CKCORE_API UCk_Ensure_Subsystem_UE : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Ensure_Subsystem_UE);

public:
    virtual auto Deinitialize() -> void override;
    virtual auto Initialize(FSubsystemCollectionBase& InCollection) -> void override;

protected:
    UFUNCTION(BlueprintImplementableEvent)
    void OnInitialize();

    UFUNCTION(BlueprintImplementableEvent)
    void OnDeinitialize();
};

// --------------------------------------------------------------------------------------------------------------------
