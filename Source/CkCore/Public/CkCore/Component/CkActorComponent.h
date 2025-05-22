#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <Components/ActorComponent.h>

#include "CkActorComponent.generated.h"

// ----------------------------------------------------------------------------------------------------------------

UINTERFACE()
class CKCORE_API UCk_CustomActorComponentVisualizer_Interface : public UInterface { GENERATED_BODY() };
class CKCORE_API ICk_CustomActorComponentVisualizer_Interface
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintNativeEvent,
        Category = "Ck|ActorComponent",
        DisplayName = "[Ck] Draw Component Visualization")
    void
    DrawVisualization(
        const AActor* InComponentOwner,
        const UActorComponent* InComponent,
        const FVector& InViewOrigin,
        const FCk_Handle_PrimitiveDrawInterface& InDrawInterface) const;
};

// ----------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCORE_API FCk_ActorComponent_DoConstruct_Params
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_ActorComponent_DoConstruct_Params);

public:
    FCk_ActorComponent_DoConstruct_Params() = default;
    FCk_ActorComponent_DoConstruct_Params(
        AActor* InActor,
        const FTransform& InTransform);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    TWeakObjectPtr<AActor> _Actor;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FTransform _Transform;

public:
    CK_PROPERTY_GET(_Actor);
    CK_PROPERTY_GET(_Transform);
};

// ----------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, BlueprintType, Blueprintable)
class CKCORE_API UCk_ActorComponent_UE : public UActorComponent
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_ActorComponent_UE);

public:
    auto BeginPlay() -> void override;

protected:
    UFUNCTION(BlueprintImplementableEvent,
              Category = "Ck|Components",
              meta = (DisplayName = "ConstructionScript"))
    void
    Do_Construct(
        const FCk_ActorComponent_DoConstruct_Params& InParams);

protected:
    virtual auto
    Do_Construct_Implementation(
        const FCk_ActorComponent_DoConstruct_Params& InParams) -> void;
};

// ----------------------------------------------------------------------------------------------------------------

