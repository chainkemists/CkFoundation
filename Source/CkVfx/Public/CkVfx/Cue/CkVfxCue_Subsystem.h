#pragma once

#include "CkCue/CkCueSubsystem_Base.h"
#include "CkVfxCue_EntityScript.h"

#include "CkVfxCue_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(DisplayName = "CkSubsystem_VfxCueExecutor")
class CKVFX_API UCk_VfxCueExecutor_Subsystem_UE : public UCk_CueExecutor_Subsystem_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_VfxCueExecutor_Subsystem_UE);

protected:
    auto
    Get_CueSubsystemClass() const -> TSubclassOf<UCk_CueSubsystem_Base_UE> override;

    auto 
    Get_DedicatedServerPolicy() const -> ECk_Cue_DedicatedServerPolicy override;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(DisplayName = "CkSubsystem_VfxCue")
class CKVFX_API UCk_VfxCueSubsystem_UE : public UCk_CueSubsystem_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_VfxCueSubsystem_UE);

protected:
    auto
    Get_CueBaseClass() const -> TSubclassOf<UCk_CueBase_EntityScript> override;
};

// --------------------------------------------------------------------------------------------------------------------
