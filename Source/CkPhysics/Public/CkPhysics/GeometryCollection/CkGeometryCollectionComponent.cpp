#include "CkGeometryCollectionComponent.h"

UCk_GeometryCollectionComponent::
    UCk_GeometryCollectionComponent(const FObjectInitializer& InObjectInitializer)
        : Super(InObjectInitializer)
{
    bEnableReplication = true;
    bEnableAbandonAfterLevel = true;
    ReplicationAbandonAfterLevel = 1;
    bNotifyBreaks = true;
}

auto
	UCk_GeometryCollectionComponent::
	OnCreatePhysicsState()
	-> void
{
	// This is done since some settings of geometry collections are mimicked in the component but we want to read the value from the collection directly
	// We use this function since it is called before RegisterAndInitializePhysicsProxy, which is not overridable
	// This would normally be done in UActorFactoryGeometryCollection::PostSpawnActor
	// TODO: make a way to hide all variables that are defined in both places to reduce cconfusion
	
	SetRestCollection(RestCollection);
	if (RestCollection)
	{
		EnableClustering = RestCollection->EnableClustering;
		ClusterGroupIndex = RestCollection->ClusterGroupIndex;
		MaxClusterLevel = RestCollection->MaxClusterLevel;
	}
	SetPhysMaterialOverride(GEngine->DefaultDestructiblePhysMaterial);
	
	Super::OnCreatePhysicsState();
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
