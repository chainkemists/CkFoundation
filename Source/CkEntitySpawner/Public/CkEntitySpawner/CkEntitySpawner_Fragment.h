#pragma once

#include "CkCore/Macros/CkMacros.h"

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

    public:
        CK_PROPERTY_GET(_EntityScript);
        CK_PROPERTY_GET(_ReplicatedChannelGroup);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_EntitySpawner_PendingSpawn, _EntityScript, _ReplicatedChannelGroup);
    };
}

// --------------------------------------------------------------------------------------------------------------------
