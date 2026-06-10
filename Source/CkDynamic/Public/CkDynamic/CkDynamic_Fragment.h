#pragma once

#include <NativeGameplayTags.h>
#include <InstancedStruct.h>

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Tag/CkTag.h"
#include "CkEcs/Signal/CkSignal_Macros.h"

#include "CkDynamic_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_DynamicFragment_UE;

// --------------------------------------------------------------------------------------------------------------------

CKDYNAMIC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_EntityFragment_Root);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    using FFragment_DynamicFragment_Data = FCk_Fragment_DynamicFragment_Data;

    // --------------------------------------------------------------------------------------------------------------------
    // Replication

    // Host-side: the set of dynamic-fragment types on this entity that opted into replication (added with
    // ECk_Replication::Replicates). The server Replicate processor pushes exactly these into the driver,
    // so a co-located tag/non-replicated dynamic fragment is never pushed.
    struct CKDYNAMIC_API FFragment_DynamicFragment_ReplicatedTypes
    {
        CK_GENERATED_BODY(FFragment_DynamicFragment_ReplicatedTypes);

    private:
        TSet<const UScriptStruct*> _Types;

    public:
        CK_PROPERTY_GET(_Types);
        CK_PROPERTY_GET_NON_CONST(_Types);
    };

    // Host-side dirty gate for FProcessor_DynamicFragment_Replicate.
    CK_DEFINE_ECS_TAG(FTag_DynamicFragment_MayRequireReplication);

    // --------------------------------------------------------------------------------------------------------------------
    // Signal

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKDYNAMIC_API,
        DynamicFragment_OnRepNotify,
        FCk_DynamicFragment_OnRepNotify,
        FCk_Handle,
        FCk_DynamicFragment_RepNotifyInfo);

    // --------------------------------------------------------------------------------------------------------------------
}

// --------------------------------------------------------------------------------------------------------------------
