#pragma once

#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"

#include "CkIsmRenderer_ProcessorInjector.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, NotBlueprintType)
class CKISMRENDERER_API UCk_IsmRenderer_ProcessorInjector_UE : public UCk_EcsWorld_ProcessorInjector_Base_UE
{
    GENERATED_BODY()

protected:
    auto
    DoInjectProcessors(
        EcsWorldType& InWorld) -> void override;
};

// --------------------------------------------------------------------------------------------------------------------
