#include "CkSceneNode_Fragment.h"

#include "Components/SceneComponent.h"

// --------------------------------------------------------------------------------------------------------------------

ck::FFragment_SceneNode_UnrealAnchor::
    FFragment_SceneNode_UnrealAnchor(
        const USceneComponent* InComponent,
        FName InSocket)
    : _Component(InComponent)
    , _Socket(InSocket)
{
}

// --------------------------------------------------------------------------------------------------------------------
