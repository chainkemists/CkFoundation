#pragma once

#include "CkActor/ActorModifier/CkActorModifier_Fragment_Data.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Fragment_Params.h"

#include "CkSignal/Public/CkSignal/CkSignal_Macros.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_ActorModifier_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct CKACTOR_API FFragment_ActorModifier_SpawnActorRequests
    {
    public:
        CK_GENERATED_BODY(FFragment_ActorModifier_SpawnActorRequests);

    public:
        friend class FProcessor_ActorModifier_SpawnActor_HandleRequests;
        friend class UCk_Utils_ActorModifier_UE;

    public:
        using RequestType = FCk_Request_ActorModifier_SpawnActor;

    private:
        RequestType _Request;

    private:
        CK_PROPERTY_GET_NON_CONST(_Request);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKACTOR_API FFragment_ActorModifier_AddActorComponentRequests
    {
    public:
        CK_GENERATED_BODY(FFragment_ActorModifier_AddActorComponentRequests);

    public:
        friend class FProcessor_ActorModifier_AddActorComponent_HandleRequests;
        friend class UCk_Utils_ActorModifier_UE;

    public:
        using RequestType = FCk_Request_ActorModifier_AddActorComponent;

    private:
        RequestType _Request;

    private:
        CK_PROPERTY_GET_NON_CONST(_Request);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKACTOR_API FFragment_ActorModifier_RemoveActorComponentRequests
    {
    public:
        CK_GENERATED_BODY(FFragment_ActorModifier_RemoveActorComponentRequests);

    public:
        friend class FProcessor_ActorModifier_RemoveActorComponent_HandleRequests;
        friend class UCk_Utils_ActorModifier_UE;

    public:
        using RequestType = FCk_Request_ActorModifier_RemoveActorComponent;

    private:
        RequestType _Request;

    private:
        CK_PROPERTY_GET_NON_CONST(_Request);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKACTOR_API,
        OnActorSpawned,
        FCk_Delegate_ActorModifier_OnActorSpawned_MC,
        TWeakObjectPtr<AActor>,
        FInstancedStruct);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKACTOR_API,
        OnActorComponentAdded,
        FCk_Delegate_ActorModifier_OnActorComponentAdded_MC,
        TWeakObjectPtr<AActor>,
        TWeakObjectPtr<UActorComponent>,
        FInstancedStruct);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKACTOR_API,
        OnActorComponentRemoved,
        FCk_Delegate_ActorModifier_OnActorComponentRemoved_MC,
        TWeakObjectPtr<AActor>,
        TSubclassOf<UActorComponent>,
        FInstancedStruct);
}

// --------------------------------------------------------------------------------------------------------------------
