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

    public:
        // TODO: Use FCk_DebuggableEntity when available [OBS-845]
        using EntityType = FCk_Handle;

    private:
        EntityType _Entity;

    public:
        CK_PROPERTY_GET(_Entity);

        CK_DEFINE_CONSTRUCTORS(FFragment_LifetimeOwner, _Entity);
    };
}

// --------------------------------------------------------------------------------------------------------------------
