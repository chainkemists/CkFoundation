#pragma once
#include "CkEcs/Handle/CkHandle.h"

#include "CkTransientEntity_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintType)
class CKECS_API UCk_Utils_TransientEntity_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    static auto
    Get_World(
        const FCk_Handle& InHandle) -> UWorld*;
};

// --------------------------------------------------------------------------------------------------------------------
