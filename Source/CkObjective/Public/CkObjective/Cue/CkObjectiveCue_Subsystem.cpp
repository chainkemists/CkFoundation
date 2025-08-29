#include "CkObjectiveCue_Subsystem.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ObjectiveCueSubsystem_UE::
    Get_CueBaseClass() const
    -> TSubclassOf<UCk_CueBase_EntityScript>
{
    return UCk_ObjectiveCue_EntityScript::StaticClass();
}

// --------------------------------------------------------------------------------------------------------------------