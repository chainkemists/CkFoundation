#pragma once

#include <Engine/Classes/Components/ActorComponent.h>
#include <CoreMinimal.h>

#include "CkMacros/CkMacros.h"

#include "CkActorComponent.generated.h"

// ----------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCORE_API FCk_ActorComponent_DoConstruct_Params
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_ActorComponent_DoConstruct_Params);

public:
    FCk_ActorComponent_DoConstruct_Params() = default;
    FCk_ActorComponent_DoConstruct_Params(TObjectPtr<AActor> InActor, const FTransform& InTransform);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    TWeakObjectPtr<AActor>   _Actor = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FTransform               _Transform;

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
    void Do_Construct (const FCk_ActorComponent_DoConstruct_Params& InParams);

protected:
    virtual auto Do_Construct_Implementation(const FCk_ActorComponent_DoConstruct_Params& InParams) -> void;
};

// ----------------------------------------------------------------------------------------------------------------

