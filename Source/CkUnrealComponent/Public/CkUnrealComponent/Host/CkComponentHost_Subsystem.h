#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"

#include "CkComponentHost_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKUNREALCOMPONENT_API UCk_ComponentHost_Subsystem_UE : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    static auto
    Get(UWorld* InWorld) -> UCk_ComponentHost_Subsystem_UE*;
};

// --------------------------------------------------------------------------------------------------------------------
