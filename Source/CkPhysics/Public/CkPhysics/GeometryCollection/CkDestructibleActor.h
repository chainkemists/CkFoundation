#pragma once
#include "CkGeometryCollectionComponent.h"

#include "CkDestructibleActor.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_UniformKinematic;
class UCk_DestructibleAnchor_ActorComponent_UE;

// --------------------------------------------------------------------------------------------------------------------

UCLASS(BlueprintType)
class ACk_Destructible : public AFieldSystemActor
{
    GENERATED_BODY()

public:
    explicit
    ACk_Destructible(
        const FObjectInitializer& InObjectInitializer = FObjectInitializer::Get());

public:
    auto
    OnConstruction(
        const FTransform& Transform) -> void override;

private:
    auto
    DoRequest_DeleteAllFieldNodes() const -> void;

private:
    UPROPERTY(Category=Destructible, VisibleAnywhere, BlueprintReadOnly,
        meta=(AllowPrivateAccess = "true"))
    TObjectPtr<UCk_GeometryCollectionComponent> _GeometryCollection;

    UPROPERTY(Category=Destructible, VisibleAnywhere, BlueprintReadOnly,
        meta=(AllowPrivateAccess = "true"))
    TObjectPtr<UCk_UniformKinematic> _UniformKinematic;
};
