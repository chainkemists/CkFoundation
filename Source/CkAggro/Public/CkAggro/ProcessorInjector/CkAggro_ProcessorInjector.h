#pragma once

#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"

#include "CkAggro_ProcessorInjector.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, NotBlueprintType)
class CKAGGRO_API UCk_Aggro_ProcessorInjector_UE : public UCk_EcsWorld_ProcessorInjector_Base_UE
{
    GENERATED_BODY()

protected:
    auto
    DoInjectProcessors(
        EcsWorldType& InWorld) -> void override;
};

// --------------------------------------------------------------------------------------------------------------------
