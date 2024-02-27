#pragma once

#include "CkEcs/Handle/CkHandle.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_DestroyEntity_Finalize);
    CK_DEFINE_ECS_TAG(FTag_DestroyEntity_Initiate);
    CK_DEFINE_ECS_TAG(FTag_DestroyEntity_Await);
    CK_DEFINE_ECS_TAG(FTag_EntityJustCreated);

    // 'Initialize' phase NOT part of Pending Kill as all regular Processors should still be able to complete their work
    // before the end of the frame
#define CK_IGNORE_PENDING_KILL \
    ck::TExclude<ck::FTag_DestroyEntity_Await>, ck::TExclude<ck::FTag_DestroyEntity_Finalize>

#define CK_IF_PENDING_KILL \
    ck::FTag_DestroyEntity_Await

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
