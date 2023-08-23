#pragma once

#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Fragment_Params.h"
#include "CkEcs/Handle/CkHandle.h"

#include "CkMacros/CkMacros.h"

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
        FCk_Handle InHandle,
        const FCk_ReplicatedObjects& InReplicatedObjects) -> void;

    // TODO: see if InReplicatedObject can be `const` and then in Add(...) we const-cast because 'ideally'
    // TODO: the replicated objects should never be modified. The reason we ARE modifying them is for link-up with Server
    static auto
    Request_AddReplicatedObject(
        FCk_Handle InHandle,
        class UCk_ReplicatedObject* InReplicatedObject) -> void;

public:
    static auto
    Get_ReplicatedObjects(
        FCk_Handle InHandle) -> FCk_ReplicatedObjects;
};

// --------------------------------------------------------------------------------------------------------------------
