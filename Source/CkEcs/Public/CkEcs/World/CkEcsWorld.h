#pragma once

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Registry/CkRegistry.h"

#include "CkCore/Macros/CkMacros.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKECS_API FEcsWorld
    {
        CK_GENERATED_BODY(FEcsWorld);

    public:
        using RegistryType = FCk_Registry;
        using HandleType = FCk_Handle;

    private:
        RegistryType _Registry;

    public:
        CK_PROPERTY_GET(_Registry);
        CK_PROPERTY_GET_NON_CONST(_Registry);

        CK_DEFINE_CONSTRUCTORS(FEcsWorld, _Registry);
    };
}

// --------------------------------------------------------------------------------------------------------------------
