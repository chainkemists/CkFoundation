#pragma once
#include "CkGeometryCollectionComponent.h"

#include "CkDestructibleActor.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(BlueprintType)
class ACk_Destructible : public AActor
{
    GENERATED_BODY()

public:
    ACk_Destructible(const FObjectInitializer& InObjectInitializer = FObjectInitializer::Get());

private:
    UPROPERTY(Category=Destructible, VisibleAnywhere, BlueprintReadOnly,
        meta=(AllowPrivateAccess = "true"))
    TObjectPtr<UCk_GeometryCollectionComponent> _GeometryCollection;
};
