#pragma once

#include "CkChaos_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKCHAOS_API UCk_Utils_Chaos_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Chaos|GeometryCollection",
        DisplayName="[Ck][GeometryCollection] Get Closest Particle Index To Location")
    static int32
    Get_ClosestParticleIndexToLocation(
        const UGeometryCollectionComponent* InGeometryCollection,
        const FVector& InLocation);
};

// --------------------------------------------------------------------------------------------------------------------