#pragma once

#include "CkEcs/Handle/CkHandle.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct FTag_PendingDestroyEntity { };
    struct FTag_TriggerDestroyEntity { };
    struct FTag_EntityJustCreated    { };

    struct FFragment_LifetimeOwner
    {
        CK_GENERATED_BODY(FFragment_LifetimeOwner);

    private:
        FCk_Handle _Entity;

    public:
        CK_PROPERTY_GET(_Entity);

        CK_DEFINE_CONSTRUCTORS(FFragment_LifetimeOwner, _Entity);
    };
}

// --------------------------------------------------------------------------------------------------------------------
