#pragma once

#include "CkEcs/Tag/CkTag.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment_Data.h"
#include "CkEcs/Handle/CkHandle.h"

#include "CkEcs/Signal/CkSignal_Macros.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // The FTag_DestroyEntity_* and FTag_EntityJustCreated tags are defined in CkHandle.h to avoid a circular
    // dependency, since they are needed there for debugging purposes.

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

    // Provenance marker stamped on an entity created while its lifetime owner is still constructing: such a child
    // is re-created by the owner's replayed construction on load, so the save ADOPTS it by identity (owner + label)
    // rather than respawning a recipe. The tag is read LIVE at capture and recorded as entity-table metadata; it
    // must never round-trip through a save (no tag does).
    CK_DEFINE_ECS_TAG(FTag_ConstructSpawned);

    // Construction-window marker for a definition-built entity, which carries NO FFragment_EntityScript_Current and
    // so would be missed by the ConstructSpawned stamp above. Held for the SYNCHRONOUS span of ConstructionInfo
    // execution only. A live construction marker is not a persistent property and is never captured.
    CK_DEFINE_ECS_TAG(FTag_DefinitionBuild_InProgress);

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
