#pragma once

#include "GameplayTagContainer.h"
#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Fragment_Params.h"
#include "CkEcs/Handle/CkHandle.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkIntent_Fragment_Params.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKINTENT_API FCk_Request_Intent_NewIntent
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Intent_NewIntent);

public:
    using InitializerFuncType = TFunction<void(UActorComponent*)>;

public:
    FCk_Request_Intent_NewIntent() = default;
    explicit FCk_Request_Intent_NewIntent(FGameplayTag InIntent);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FGameplayTag _Intent;

public:
    CK_PROPERTY_GET(_Intent);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKINTENT_API UCk_Intent_ReplicatedObject_UE : public UCk_Ecs_ReplicatedObject_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY_FRAGMENT_REP(UCk_Intent_ReplicatedObject_UE);

public:
    // TODO: introduce a Params struct for this
    static auto Create(AActor* InOwningActor, FCk_Handle InAssociatedEntity) -> UCk_Intent_ReplicatedObject_UE*;

protected:
    UFUNCTION()
    virtual void OnRep_IntentReady(bool InReady);

    virtual auto GetLifetimeReplicatedProps(TArray<FLifetimeProperty>&) const -> void override;

public:
    UPROPERTY(ReplicatedUsing = OnRep_IntentReady)
    bool _Ready = false;

public:
    UFUNCTION(Client, Reliable)
    void AddIntent(FGameplayTag InIntent);
};

// --------------------------------------------------------------------------------------------------------------------

