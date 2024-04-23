#pragma once

#include "CkEcs/Net/CkNet_Fragment_Data.h"
#include "CkEcs/Tag/CkTag.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_HasAuthority);
    CK_DEFINE_ECS_TAG(FTag_NetMode_IsHost);

    // --------------------------------------------------------------------------------------------------------------------

    struct CKECS_API FFragment_Net_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_Net_Params);

    private:
        FCk_Net_ConnectionSettings _ConnectionSettings;

    public:
        CK_PROPERTY_GET(_ConnectionSettings);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_Net_Params, _ConnectionSettings);
    };
}

// --------------------------------------------------------------------------------------------------------------------
