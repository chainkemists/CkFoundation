#pragma once

#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"

#include "CkFlowGraph_ProcessorInjector.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, NotBlueprintType)
class CKFLOWGRAPH_API UCk_FlowGraph_ProcessorInjector_Requests_UE : public UCk_EcsWorld_ProcessorInjector_Base_UE
{
    GENERATED_BODY()

protected:
    auto
    DoInjectProcessors(
        EcsWorldType& InWorld) -> void override;
};

// --------------------------------------------------------------------------------------------------------------------
