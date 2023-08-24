#pragma once

#include "CkMacros/CkMacros.h"
#include "Component/CkActorComponent.h"

#include <CoreMinimal.h>

#include "CkObjectReplicatorComponent.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_ReplicatedObject;

// --------------------------------------------------------------------------------------------------------------------


UCLASS()
class CKCORE_API UCk_ObjectReplicator_Component : public UCk_ActorComponent_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_ObjectReplicator_Component);

public:
    UCk_ObjectReplicator_Component();

public:
    auto Request_RegisterObjectForReplication(UCk_ReplicatedObject* InObject) -> void;
    auto Request_UnregisterObjectForReplication(UCk_ReplicatedObject* InObject) -> void;

public:
    UPROPERTY()
    TSet<UCk_ReplicatedObject*> _ReplicatedObjects;
};

// --------------------------------------------------------------------------------------------------------------------
