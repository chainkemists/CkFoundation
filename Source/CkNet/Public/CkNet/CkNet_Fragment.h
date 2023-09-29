#pragma once
#include "CkNet/CkNet_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct FTag_HasAuthority {};

    struct FTag_NetMode_IsHost{};
}

namespace ck
{
    struct CKNET_API FFragment_Net_Params
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
