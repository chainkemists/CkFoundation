#pragma once

#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"

#include "CkMessaging_ProcessorInjector.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, NotBlueprintType)
class CKMESSAGING_API UCk_Messaging_ProcessorInjector_UE : public UCk_EcsWorld_ProcessorInjector_Base_UE
{
    GENERATED_BODY()

protected:
    auto
    DoInjectProcessors(
        EcsWorldType& InWorld) -> void override;
};

// --------------------------------------------------------------------------------------------------------------------
