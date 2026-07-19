#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------

class AActor;
class UCk_Utils_JoltStaticActor_UE;
class UCk_JoltStaticWorld_Subsystem_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_JoltStaticActor_EndPlay;

    // --------------------------------------------------------------------------------------------------------------------

    // Attribution state for a baked-static-world source actor. _BodyIds are the raw JPH::BodyIDs (index+seq)
    // this actor contributed; the subsystem stamps each body's Jolt user-data with this entity's id so hits
    // resolve back here. _SourceActor observes the contributing actor (may go null when it dies);
    // _SourceActorName caches its FName so attribution survives the actor's death. An empty _BodyIds array is
    // the idempotence guard for the bidirectional removal helper.
    struct CKJOLT_API FFragment_JoltStaticActor_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_JoltStaticActor_Current);

    public:
        friend class FProcessor_JoltStaticActor_EndPlay;
        friend class ::UCk_Utils_JoltStaticActor_UE;
        friend class ::UCk_JoltStaticWorld_Subsystem_UE;

    private:
        TArray<uint32>               _BodyIds;
        TWeakObjectPtr<const AActor> _SourceActor;
        FName                        _SourceActorName;

    public:
        CK_PROPERTY_GET(_BodyIds);
        CK_PROPERTY_GET(_SourceActor);
        CK_PROPERTY_GET(_SourceActorName);
    };
}

// --------------------------------------------------------------------------------------------------------------------
