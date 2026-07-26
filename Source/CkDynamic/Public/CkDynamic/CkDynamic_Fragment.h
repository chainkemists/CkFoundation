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

    // Host-side. The server Replicate processor pushes EXACTLY these types, never a co-located local-only one.
    struct CKDYNAMIC_API FFragment_DynamicFragment_ReplicatedTypes
    {
        CK_GENERATED_BODY(FFragment_DynamicFragment_ReplicatedTypes);

    private:
        TSet<const UScriptStruct*> _Types;

    public:
        CK_PROPERTY_GET(_Types);
        CK_PROPERTY_GET_NON_CONST(_Types);
    };

    CK_DEFINE_ECS_TAG(FTag_DynamicFragment_MayRequireReplication);

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKDYNAMIC_API,
        DynamicFragment_OnRepNotify,
        FCk_DynamicFragment_OnRepNotify,
        FCk_Handle,
        FCk_DynamicFragment_RepNotifyInfo);

    // --------------------------------------------------------------------------------------------------------------------
}

// --------------------------------------------------------------------------------------------------------------------
