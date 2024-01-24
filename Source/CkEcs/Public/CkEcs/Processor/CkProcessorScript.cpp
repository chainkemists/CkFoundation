#include "CkProcessorScript.h"

#include "CkEcs/Processor/CkProcessorScript_Subsystem.h"
#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"

#include <Engine/World.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Ecs_ProcessorScript_Base_UE::
    Tick(
        TimeType InTime)
    -> void
{
    if (ck::Is_NOT_Valid(_Registry))
    { _Registry = GetWorld()->GetSubsystem<UCk_EcsWorld_Subsystem_UE>()->Get_Registry(); }
}

auto
    UCk_Ecs_ProcessorScript_Base_UE::
    ProcessEntity_If_Implementation(
        FCk_Handle InEntity) const
    -> bool
{
    return true;
}

// --------------------------------------------------------------------------------------------------------------------
