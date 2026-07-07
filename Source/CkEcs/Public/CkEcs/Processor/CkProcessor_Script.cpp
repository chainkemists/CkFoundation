#include "CkProcessor_Script.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Processor_Script_Base_UE::
    Set_Handle(
        const FCk_Handle& InHandle)
    -> void
{
    _Handle = InHandle;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Processor_Script_Base_UE::
    Set_IterationTarget(
        UCk_Processor_Script_Base_UE* InTarget)
    -> void
{
    _IterationTarget = InTarget;
}
