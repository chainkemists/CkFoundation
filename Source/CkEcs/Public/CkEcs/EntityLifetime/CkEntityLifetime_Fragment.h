#pragma once

#include "CkEcs/Tag/CkTag.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment_Params.h"
#include "CkEcs/Handle/CkHandle.h"

#include "CkSignal/Public/CkSignal/CkSignal_Macros.h"

// --------------------------------------------------------------------------------------------------------------------

/*
 * Destruction Pipeline:
 * => Entity Destruction Request = DestroyEntity_Initiate (Entity is still valid)
 *
 * ---> Processors have a chance to reason about an Entity that has initiated destruction
 *
 * => End of Frame = DestroyEntity_Initiate_Confirm (Entity is still valid)
 *     - Initiate_Confirm is needed to guarantee that 'Initiate' will last 1 full frame
 *
 * ---> Teardown Processors can hook in here as this is their last chance to deal with an Entity that is about to be Invalidated
 *
 * => Start of Frame = DestroyEntity_Initiate CONVERTS TO DestroyEntity_Await (Entity is now Invalid)
 * => Start of NEXT Frame = DestroyEntity_Initiate CONVERT TO DestroyEntity_Finalize
 */

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_DestroyEntity_Finalize);
    CK_DEFINE_ECS_TAG(FTag_DestroyEntity_Initiate);
    CK_DEFINE_ECS_TAG(FTag_DestroyEntity_Initiate_Confirm);
    CK_DEFINE_ECS_TAG(FTag_DestroyEntity_Await);
    CK_DEFINE_ECS_TAG(FTag_EntityJustCreated);

    // 'Initialize' phase NOT part of Pending Kill as all regular Processors should still be able to complete their work
    // before the end of the frame
#define CK_IGNORE_PENDING_KILL \
    ck::TExclude<ck::FTag_DestroyEntity_Await>, ck::TExclude<ck::FTag_DestroyEntity_Finalize>

#define CK_IF_INITIATE_CONFIRM_KILL \
    ck::FTag_DestroyEntity_Initiate_Confirm

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

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKECS_API, EntityDestroyed, FCk_Delegate_Lifetime_OnDestroy_MC, FCk_Handle);
}

// --------------------------------------------------------------------------------------------------------------------
// Algos

namespace ck::algo
{
    struct CKECS_API IsDestructionPhase
    {
    public:
        auto operator()(const FCk_Handle& InHandle) const -> bool;

    private:
        ECk_EntityLifetime_DestructionPhase _DestructionPhase = ECk_EntityLifetime_DestructionPhase::Initiated;

    public:
        CK_DEFINE_CONSTRUCTOR(IsDestructionPhase, _DestructionPhase)
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKECS_API Is_NOT_DestructionPhase
    {
    public:
        auto operator()(const FCk_Handle& InHandle) const -> bool;

    private:
        ECk_EntityLifetime_DestructionPhase _DestructionPhase = ECk_EntityLifetime_DestructionPhase::Initiated;

    public:
        CK_DEFINE_CONSTRUCTOR(Is_NOT_DestructionPhase, _DestructionPhase)
    };
}

// --------------------------------------------------------------------------------------------------------------------
