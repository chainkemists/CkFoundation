#include "CkObjectiveCue_EntityScript.h"

#include "CkCore/Validation/CkIsValid.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ObjectiveCue_EntityScript::
    Construct(
        FCk_Handle& InHandle,
        const FInstancedStruct& InSpawnParams)
    -> ECk_EntityScript_ConstructionFlow
{
    return Super::Construct(InHandle, InSpawnParams);
}

// --------------------------------------------------------------------------------------------------------------------