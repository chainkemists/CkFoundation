#pragma once

#include "CkEcs/Tag/CkTag.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment_Data.h"
#include "CkEcs/Handle/CkHandle.h"

#include "CkEcs/Signal/CkSignal_Macros.h"

// --------------------------------------------------------------------------------------------------------------------

/*
┌─────────────────────────────────────────────────────────────────────────┐
│ DOCUMENTATION IN CPP FILE                                               │
└─────────────────────────────────────────────────────────────────────────┘
*/

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Defined in CkHandle.h to avoid circular dependency since it's needed for debugging purposes

    //CK_DEFINE_ECS_TAG(FTag_DestroyEntity_Initiate);
    //CK_DEFINE_ECS_TAG(FTag_DestroyEntity_EndPlay);
    //CK_DEFINE_ECS_TAG(FTag_DestroyEntity_Teardown);
    //CK_DEFINE_ECS_TAG(FTag_DestroyEntity_Await);
    //CK_DEFINE_ECS_TAG(FTag_DestroyEntity_Finalize);
    //CK_DEFINE_ECS_TAG(FTag_EntityJustCreated);

    // 'Initialize' phase NOT part of Pending Kill as all regular Processors should still be able to complete their work
    // before the end of the frame
#define CK_IGNORE_PENDING_KILL                     \
    ck::TExclude<ck::FTag_DestroyEntity_EndPlay>,  \
    ck::TExclude<ck::FTag_DestroyEntity_Teardown>, \
    ck::TExclude<ck::FTag_DestroyEntity_Await>,    \
    ck::TExclude<ck::FTag_DestroyEntity_Finalize>

#define CK_IF_END_PLAY                             \
    ck::FTag_DestroyEntity_EndPlay,                \
    ck::TExclude<ck::FTag_DestroyEntity_Teardown>, \
    ck::TExclude<ck::FTag_DestroyEntity_Await>,    \
    ck::TExclude<ck::FTag_DestroyEntity_Finalize>

#define CK_IF_TEARING_DOWN                         \
    ck::FTag_DestroyEntity_Teardown, ck::TExclude<ck::FTag_DestroyEntity_Await>, ck::TExclude<ck::FTag_DestroyEntity_Finalize>

    // --------------------------------------------------------------------------------------------------------------------

    // Provenance marker (save/load rebuild+hydrate, spec §4.2). Stamped on an entity created while its lifetime
    // owner is a still-constructing EntityScript (owner has FFragment_EntityScript_Current but has NOT begun play):
    // such a child is re-created by the owner's replayed Construct/BeginPlay on load, so the save ADOPTS it by
    // identity (owner + label) rather than respawning a recipe (RuntimeSpawned). Stamped at create time in
    // Request_SetupEntityWithLifetimeOwner — the owner's live construction state is frozen here since it has long
    // begun play by capture time. TRANSIENT is deliberate: the v3 writer reads the LIVE tag at capture and records
    // provenance as entity-table metadata, so the tag itself must never round-trip through Model A's tag capture.
    CK_DEFINE_ECS_TAG_TRANSIENT(FTag_ConstructSpawned);

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

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKECS_API, OnEntityBeginDestroy, FCk_Delegate_OnBeginDestroy, FCk_Handle);
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
        ECk_EntityLifetime_DestructionPhase _DestructionPhase = ECk_EntityLifetime_DestructionPhase::BeginDestroy;

    public:
        CK_DEFINE_CONSTRUCTOR(IsDestructionPhase, _DestructionPhase)
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKECS_API Is_NOT_DestructionPhase
    {
    public:
        auto operator()(const FCk_Handle& InHandle) const -> bool;

    private:
        ECk_EntityLifetime_DestructionPhase _DestructionPhase = ECk_EntityLifetime_DestructionPhase::BeginDestroy;

    public:
        CK_DEFINE_CONSTRUCTOR(Is_NOT_DestructionPhase, _DestructionPhase)
    };
}

// --------------------------------------------------------------------------------------------------------------------
