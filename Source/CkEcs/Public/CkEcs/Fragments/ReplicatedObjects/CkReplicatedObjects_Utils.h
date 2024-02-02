#pragma once

#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Fragment_Params.h"
#include "CkEcs/Handle/CkHandle.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkReplicatedObjects_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKECS_API UCk_Utils_ReplicatedObjects_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_ReplicatedObjects_UE);

public:
    static auto
    Add(
        UPARAM(ref) FCk_Handle InHandle,
        const FCk_ReplicatedObjects& InReplicatedObjects) -> void;

    static auto
    Has(
        FCk_Handle InHandle) -> bool;

    static auto
    Ensure(
        FCk_Handle InHandle) -> bool;

public:
    // TODO: see if InReplicatedObject can be `const` and then in Add(...) we const-cast because 'ideally'
    // TODO: the replicated objects should never be modified. The reason we ARE modifying them is for link-up with Server
    static auto
    Request_AddReplicatedObject(
        FCk_Handle InHandle,
        class UCk_ReplicatedObject_UE* InReplicatedObject) -> void;

    static auto
    Get_NetRole(FCk_Handle InHandle) -> ENetRole;

    static auto
    OnFirstValidReplicatedObject(
        FCk_Handle InHandle,
        ECk_PendingKill_Policy InPendingKillPolicy,
        const std::function<void(const TWeakObjectPtr<UCk_ReplicatedObject_UE>& InRO)>& InFunc) -> void;

public:
    static auto
    Get_ReplicatedObjects(
        const FCk_Handle& InHandle) -> const FCk_ReplicatedObjects&;
};

// --------------------------------------------------------------------------------------------------------------------
