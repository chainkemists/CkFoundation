#include "CkProcessorScript.h"

#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Ecs_ProcessorScript_Base::
    Tick(
        TimeType InTime)
    -> void
{
    if (ck::Is_NOT_Valid(_Registry))
    { _Registry = GetWorld()->GetSubsystem<UCk_EcsWorld_Subsystem_UE>()->Get_Registry(); }
}

// --------------------------------------------------------------------------------------------------------------------
