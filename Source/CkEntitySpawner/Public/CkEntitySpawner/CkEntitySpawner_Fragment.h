#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <Containers/UnrealString.h>
#include <GameplayTagContainer.h>
#include <UObject/ObjectPtr.h>

// --------------------------------------------------------------------------------------------------------------------

class UCk_EntityScript_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct CKENTITYSPAWNER_API FFragment_EntitySpawner_PendingSpawn
    {
    public:
        CK_GENERATED_BODY(FFragment_EntitySpawner_PendingSpawn);

    public:
        friend class FProcessor_EntitySpawner_Spawn;

    private:
        TObjectPtr<UCk_EntityScript_UE> _EntityScript;
        FGameplayTag _ReplicatedChannelGroup;

        // The spawner actor never sees the entity it queued — it is spawned ticks later under a relay channel — so
        // the rendezvous identity has to ride the queue entry to reach the only place that can stamp it. Empty for
        // a runtime-spawned spawner, which has no identity stable enough to key.
        FString _SaveKeyIdentity;

    public:
        CK_PROPERTY_GET(_EntityScript);
        CK_PROPERTY_GET(_ReplicatedChannelGroup);
        CK_PROPERTY_GET(_SaveKeyIdentity);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_EntitySpawner_PendingSpawn, _EntityScript, _ReplicatedChannelGroup, _SaveKeyIdentity);
    };
}

// --------------------------------------------------------------------------------------------------------------------
