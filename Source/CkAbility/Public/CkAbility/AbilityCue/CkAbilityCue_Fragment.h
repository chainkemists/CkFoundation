#pragma once

#include "CkAbility/AbilityCue/CkAbilityCue_Fragment_Data.h"

#include "CkCore/Macros/CkMacros.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct FFragment_AbilityCue_SpawnRequest
    {
        CK_GENERATED_BODY(FFragment_AbilityCue_SpawnRequest);

    private:
        FGameplayTag _CueName;
        TObjectPtr<UObject> _WorldContextObject;
        FCk_AbilityCue_Params _ReplicatedParams;
        ECk_Replication _Replication;

    public:
        CK_PROPERTY_GET(_CueName);
        CK_PROPERTY_GET(_WorldContextObject);
        CK_PROPERTY_GET(_ReplicatedParams);
        CK_PROPERTY_GET(_Replication);

        CK_DEFINE_CONSTRUCTORS(FFragment_AbilityCue_SpawnRequest, _CueName, _WorldContextObject, _ReplicatedParams, _Replication);
    };
}

// --------------------------------------------------------------------------------------------------------------------
