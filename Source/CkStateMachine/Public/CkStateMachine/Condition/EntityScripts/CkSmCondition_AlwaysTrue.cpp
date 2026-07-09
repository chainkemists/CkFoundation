#include "CkSmCondition_AlwaysTrue.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_SmCondition_AlwaysTrue::
    EnterCondition(
        FCk_Handle_SmCondition InHandle,
        ECk_Sm_NetContext InNetContext)
    -> void
{
    // Super must run first so EventDriven::EnterCondition's resting-state Fail write happens
    // BEFORE we override with Pass. Reversing the order would leave the condition stuck at Fail.
    // Living in the condition lifecycle (not BeginPlay) keeps the Pass write behind the base's
    // transition-authority gate.
    Super::EnterCondition(InHandle, InNetContext);
    MarkSatisfied();
}

// --------------------------------------------------------------------------------------------------------------------
