#include "CkIsmSubsystem.h"

#include "Components/InstancedStaticMeshComponent.h"

// --------------------------------------------------------------------------------------------------------------------

ACk_IsmRenderer_Actor_UE::
    ACk_IsmRenderer_Actor_UE()
{
    _InstancedStaticMeshComponent = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("InstancedStaticMeshComponent"));

    // Set it as the root component if this is the only component, otherwise attach it to the root
    if (!RootComponent)
    {
        SetRootComponent(_InstancedStaticMeshComponent);
    }
    else
    {
        _InstancedStaticMeshComponent->SetupAttachment(RootComponent);
    }
}

// --------------------------------------------------------------------------------------------------------------------
