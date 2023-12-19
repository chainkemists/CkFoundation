#pragma once
#include "CkCore/Macros/CkMacros.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct FFragment_AbilityCue_SpawnRequest
    {
        CK_GENERATED_BODY(FFragment_AbilityCue_SpawnRequest);

    private:
        TObjectPtr<UObject> _WorldContextObject;

    public:
        CK_PROPERTY_GET(_WorldContextObject);

        CK_DEFINE_CONSTRUCTORS(FFragment_AbilityCue_SpawnRequest, _WorldContextObject);
    };
}

// --------------------------------------------------------------------------------------------------------------------
