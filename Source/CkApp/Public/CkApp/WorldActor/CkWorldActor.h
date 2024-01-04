#pragma once

#include <GameFramework/Info.h>

#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"

#include "CkWorldActor.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, NotBlueprintType)
class CKAPP_API UCk_World_ProcessorInjector : public UCk_EcsWorld_ProcessorInjector_Base
{
    GENERATED_BODY()

protected:
    auto DoInjectProcessors(
        EcsWorldType& InWorld) -> void override;
};

// --------------------------------------------------------------------------------------------------------------------
