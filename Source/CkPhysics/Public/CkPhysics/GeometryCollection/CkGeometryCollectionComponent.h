#pragma once

#include <GeometryCollection/GeometryCollectionComponent.h>

#include "CkGeometryCollectionComponent.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Blueprintable, BlueprintType, meta = (BlueprintSpawnableComponent), MinimalAPI)
class UCk_GeometryCollectionComponent : public UGeometryCollectionComponent
{
    GENERATED_BODY()

public:
    UCk_GeometryCollectionComponent(
        const FObjectInitializer& InObjectInitializer = FObjectInitializer::Get());
    
    virtual void OnCreatePhysicsState() override;
    
    // Required for Replicating Geometry Collections
    void Request_EnableAsyncPhysics();
};

// --------------------------------------------------------------------------------------------------------------------
