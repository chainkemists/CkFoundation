#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"

#include "CkCue_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_Cue_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_Cue_Execute);
    CK_DEFINE_ECS_TAG(FTag_Cue_ExecuteLocal);

    // --------------------------------------------------------------------------------------------------------------------

    using FFragment_Cue_ExecuteRequest = FCk_Request_Cue_Execute;

    // --------------------------------------------------------------------------------------------------------------------

    struct CKCUE_API FFragment_Cue_ExecuteRequestLocal
    {
    public:
        CK_GENERATED_BODY(FFragment_Cue_ExecuteRequestLocal);

    private:
        FGameplayTag _CueName;
        FInstancedStruct _SpawnParams;

    public:
        CK_PROPERTY_GET(_CueName);
        CK_PROPERTY_GET(_SpawnParams);

        CK_DEFINE_CONSTRUCTORS(FFragment_Cue_ExecuteRequestLocal, _CueName, _SpawnParams);
    };
}

// --------------------------------------------------------------------------------------------------------------------
