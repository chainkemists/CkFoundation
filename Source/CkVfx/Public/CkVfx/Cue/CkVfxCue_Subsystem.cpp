#include "CkVfxCue_Subsystem.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_VfxCueExecutor_Subsystem_UE::
    Get_CueSubsystemClass() const
    -> TSubclassOf<UCk_CueSubsystem_Base_UE>
{
    return UCk_VfxCueSubsystem_UE::StaticClass();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_VfxCueSubsystem_UE::
    Get_CueBaseClass() const
    -> TSubclassOf<UCk_CueBase_EntityScript>
{
    return UCk_VfxCue_EntityScript::StaticClass();
}

// --------------------------------------------------------------------------------------------------------------------
