#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Component/CkActorComponent.h"

#include <CoreMinimal.h>

#include "CkObjectReplicatorComponent.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_ReplicatedObject_UE;

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKCORE_API UCk_ObjectReplicator_ActorComponent_UE : public UCk_ActorComponent_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_ObjectReplicator_ActorComponent_UE);

public:
    UCk_ObjectReplicator_ActorComponent_UE();

public:
    UFUNCTION(BlueprintCallable)
    void Request_RegisterObjectForReplication(UCk_ReplicatedObject_UE* InObject);

    UFUNCTION(BlueprintCallable)
    void Request_UnregisterObjectForReplication(UCk_ReplicatedObject_UE* InObject);

    auto GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const -> void override;

public:
    UFUNCTION()
    void OnRep_ReplicatedObject();

    UPROPERTY(ReplicatedUsing = OnRep_ReplicatedObject)
    TArray<UCk_ReplicatedObject_UE*> _ReplicatedObjects;
};

// --------------------------------------------------------------------------------------------------------------------
