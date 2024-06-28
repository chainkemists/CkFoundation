#include "CkGeometryCollectionComponent.h"

UCk_GeometryCollectionComponent::
    UCk_GeometryCollectionComponent()
        : Super()
{
    bEnableReplication = true;
    bEnableAbandonAfterLevel = true;
    ReplicationAbandonAfterLevel = 1;
}

// --------------------------------------------------------------------------------------------------------------------
auto
    UCk_GeometryCollectionComponent::
    Request_EnableAsyncPhysics()
    -> void
{
    if (IsTemplate())
    { return; }

    if (IsDefaultSubobject())
    { return; }

    if (GetWorld()->IsNetMode(NM_Client))
    { return; }

    SetAsyncPhysicsTickEnabled(true);
}
