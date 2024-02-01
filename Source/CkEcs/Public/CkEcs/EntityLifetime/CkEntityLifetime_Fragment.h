#pragma once

#include "CkEcs/Handle/CkHandle.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_PendingDestroyEntity);
    CK_DEFINE_ECS_TAG(FTag_TriggerDestroyEntity);
    CK_DEFINE_ECS_TAG(FTag_EntityJustCreated);

#define CK_IGNORE_PENDING_KILL \
    ck::TExclude<ck::FTag_PendingDestroyEntity>, ck::TExclude<ck::FTag_TriggerDestroyEntity>

#define CK_IF_PENDING_KILL \
    ck::FTag_PendingDestroyEntity

    // --------------------------------------------------------------------------------------------------------------------

    struct FFragment_LifetimeDependents
    {
        CK_GENERATED_BODY(FFragment_LifetimeDependents);

        friend class UCk_Utils_EntityLifetime_UE;

    public:
        // TODO: Use FCk_DebuggableEntity when available [OBS-845]
        using EntityType = FCk_Handle;

    private:
        TArray<EntityType> _Entities;

    public:
        CK_PROPERTY_GET(_Entities);
    };

    // --------------------------------------------------------------------------------------------------------------------

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
