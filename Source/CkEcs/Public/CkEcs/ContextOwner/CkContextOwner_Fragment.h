#pragma once

#include "CkEcs/Tag/CkTag.h"
#include "CkEcs/Handle/CkHandle.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct FFragment_ContextOwner
    {
        CK_GENERATED_BODY(FFragment_ContextOwner);

    public:
        using EntityType = FCk_Handle;

    private:
        EntityType _Entity;

    public:
        CK_PROPERTY_GET(_Entity);

        CK_DEFINE_CONSTRUCTORS(FFragment_ContextOwner, _Entity);
    };
}

// --------------------------------------------------------------------------------------------------------------------