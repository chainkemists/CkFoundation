#pragma once

#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"

#include "CkLocator_ProcessorInjector.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, NotBlueprintType)
class CKSPATIALQUERY_API UCk_Locator_ProcessorInjector_UE : public UCk_EcsWorld_ProcessorInjector_Base_UE
{
    GENERATED_BODY()

protected:
    auto
    DoInjectProcessors(
        EcsWorldType& InWorld) -> void override;
};

// --------------------------------------------------------------------------------------------------------------------
