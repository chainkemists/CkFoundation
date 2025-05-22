#include "CkActorComponent_Visualizer.h"

#include "CkCore/Component/CkActorComponent.h"
#include "CkCore/Scene/CkScene.h"
#include "CkCore/Validation/CkIsValid.h"

// ----------------------------------------------------------------------------------------------------------------

auto
    FCk_ActorComponent_Visualizer::
    DrawVisualization(
        const UActorComponent* InComponent,
        const FSceneView* InView,
        FPrimitiveDrawInterface* PDI)
    -> void
{
    if (ck::Is_NOT_Valid(InComponent))
    { return; }

    if (ck::Is_NOT_Valid(PDI, ck::IsValid_Policy_NullptrOnly{}))
    { return; }

    const auto PrimitiveDrawInterfaceHandle = FCk_Handle_PrimitiveDrawInterface{PDI};

    ICk_CustomActorComponentVisualizer_Interface::Execute_DrawVisualization(
        InComponent,
        InComponent->GetOwner(),
        InComponent,
        InView->ViewMatrices.GetViewOrigin(),
        PrimitiveDrawInterfaceHandle);
}

// ----------------------------------------------------------------------------------------------------------------
