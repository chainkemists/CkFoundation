#pragma once

#include "GeometryCollectionComponent.h"

#include "CkGeometryCollectionComponent.generated.h"

UCLASS(meta = (BlueprintSpawnableComponent), MinimalAPI)
class UCk_GeometryCollectionComponent : public UGeometryCollectionComponent
{
    GENERATED_BODY()

public:
    UCk_GeometryCollectionComponent();

public:
    // Required for Replicating Geometry Collections
    void Request_EnableAsyncPhysics();
};
