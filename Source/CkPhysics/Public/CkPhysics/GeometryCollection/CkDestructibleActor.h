#pragma once

#include "CkEcs/EntityConstructionScript/CkEntity_ConstructionScript.h"

#include "CkEntityBridge/CkEntityBridge_ConstructionScript.h"

#include "CkPhysics/GeometryCollection/CkGeometryCollectionComponent.h"

#include "CkDestructibleActor.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_UniformKinematic;
class UCk_DestructibleAnchor_ActorComponent_UE;

// --------------------------------------------------------------------------------------------------------------------

UCLASS(BlueprintType)
class ACk_Destructible : public AFieldSystemActor, public ICk_Entity_ConstructionScript_Interface
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

public:
    auto
    DoConstruct_Implementation(
        FCk_Handle& InHandle) -> void override;

private:
    auto
    DoRequest_DeleteAllFieldNodes() const -> void;

private:
    UPROPERTY(Category=Destructible, EditDefaultsOnly, BlueprintReadOnly,
        meta=(AllowPrivateAccess = "true"))
    TObjectPtr<UCk_GeometryCollectionComponent> _GeometryCollection;

    UPROPERTY(Category=Destructible, VisibleAnywhere, BlueprintReadOnly,
        meta=(AllowPrivateAccess = "true"))
    TObjectPtr<UCk_UniformKinematic> _UniformKinematic;

    UPROPERTY(Category=Ecs, VisibleAnywhere, BlueprintReadOnly,
        meta=(AllowPrivateAccess = "true"))
    TObjectPtr<UCk_EntityBridge_ActorComponent_UE> _EntityBridge;
};
