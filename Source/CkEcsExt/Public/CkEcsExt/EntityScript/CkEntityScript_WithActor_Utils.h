#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/EntityScript/CkEntityScript_Fragment_Data.h"

#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkEntityScript_WithActor_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_EntityScript_WithActor_UE;

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "AActor"))
class CKECSEXT_API UCk_Utils_EntityScript_WithActor_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_EntityScript_WithActor_UE);

public:
    // The one-call replacement for the hand-rolled WithActor spawn boilerplate. Spawns only on
    // authority — on clients the Entity arrives through the EntityScript replication pipeline and
    // links up in Construct, so the returned handle is invalid there. The actor-side hook that
    // works on every world is UCk_Utils_OwningActor_UE::Promise_OnActorEcsReady.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|EntityScript",
              DisplayName = "[Ck][EntityScript] Request Spawn EntityScript On Actor",
              meta = (DefaultToSelf = "InActor", Keywords = "spawn, link, actor, entity, ecs, ready, bridge"))
    static FCk_Handle_PendingEntityScript
    Request_SpawnEntityScript_OnActor(
        AActor* InActor,
        TSubclassOf<UCk_EntityScript_WithActor_UE> InEntityScriptClass);
};

// --------------------------------------------------------------------------------------------------------------------
