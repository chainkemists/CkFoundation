#include "CkObjectiveCue_Subsystem.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ObjectiveCueExecutor_Subsystem_UE::
    Get_CueSubsystemClass() const
    -> TSubclassOf<UCk_CueSubsystem_Base_UE>
{
    return UCk_ObjectiveCueSubsystem_UE::StaticClass();
}

// --------------------------------------------------------------------------------------------------------------------
auto
    UCk_ObjectiveCueSubsystem_UE::
    Get_CueBaseClass() const
    -> TSubclassOf<UCk_CueBase_EntityScript>
{
    return UCk_ObjectiveCue_EntityScript::StaticClass();
}

// --------------------------------------------------------------------------------------------------------------------