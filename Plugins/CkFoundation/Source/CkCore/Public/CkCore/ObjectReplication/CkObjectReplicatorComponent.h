#pragma once

#include "CkMacros/CkMacros.h"
#include "Component/CkActorComponent.h"

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
    auto Request_RegisterObjectForReplication(UCk_ReplicatedObject_UE* InObject) -> void;
    auto Request_UnregisterObjectForReplication(UCk_ReplicatedObject_UE* InObject) -> void;

public:
    UPROPERTY()
    TSet<UCk_ReplicatedObject_UE*> _ReplicatedObjects;
};

// --------------------------------------------------------------------------------------------------------------------
