#include "CkSmCondition_AlwaysTrue.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_SmCondition_AlwaysTrue::
    BeginPlay()
    -> void
{
    // Super::BeginPlay must run first so EventDriven::EnterCondition's
    // resting-state Fail write happens BEFORE we override with Pass.
    // Reversing the order would leave the condition stuck at Fail.
    Super::BeginPlay();
    MarkSatisfied();
}

// --------------------------------------------------------------------------------------------------------------------
