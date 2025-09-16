#pragma once

#include "CkEcs/Tag/CkTag.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment_Data.h"
#include "CkEcs/Handle/CkHandle.h"

#include "CkEcs/Signal/CkSignal_Macros.h"

// --------------------------------------------------------------------------------------------------------------------

/*
 * Entity Destruction Pipeline
 * ===========================
 *
 * When Destroy(Entity) is called, the entity goes through a multi-frame destruction process
 * to ensure all systems have proper opportunity to clean up before the entity is invalidated.
 *
 * Overview:
 * ---------
 * Frame N:     DestroyEntity_Initiate → DestroyEntity_Initiate_Confirm (at end of frame)
 * Frame N+1:   DestroyEntity_Initiate_Confirm → DestroyEntity_Await (at start of frame)
 * Frame N+2:   DestroyEntity_Await → DestroyEntity_Finalize (sequential at start of frame)
 *
 *
 * Detailed Flow:
 * ==============
 *
 * Frame N                    Frame N+1                    Frame N+2
 * ┌──────────────────┐      ┌──────────────────────-┐     ┌────────────────────┐
 * │ DestroyEntity_   │      │ DestroyEntity_        │     │ Sequential at      │
 * │ Initiate         │      │ Initiate_Confirm      │     │ start of frame:    │
 * │                  │      │        │              │     │                    │
 * │ Entity VALID     │      │        ▼              │     │ Await →            │
 * │                  │      │ Teardown processors   │     │ Finalize →         │
 * │ Regular          │      │ run here (between     │ ───►│ Entity Destroyed   │
 * │ processors       │      │ Initiate_Confirm      │     │                    │
 * │ MAY run          │      │ and Await)            │     │ Entity INVALID     │
 * │ (timing          │      │        │              │     │ Memory reclaimed   │
 * │ dependent)       │      │        ▼              │     │                    │
 * │                  │      │ DestroyEntity_        │     │                    │
 * │        │         │      │ Await                 │     │                    │
 * │        ▼         │      │                       │     │                    │
 * │ At END of frame: │      │ Entity handle VALID   │     │                    │
 * │ Initiate →       │      │ but component data    │     │                    │
 * │ Initiate_Confirm │ ────►│ may be torn down      │     │                    │
 * │                  │      │                       │     │                    │
 * │                  │      │ Processors can access │     │                    │
 * │                  │      │ with Await entities   │     │                    │
 * │                  │      │ but must handle       │     │                    │
 * │                  │      │ missing/invalid data  │     │                    │
 * └──────────────────┘      └──────────────────────-┘     └────────────────────┘
 *
 *    Destroy(Entity)         Initiate_Confirm only        Sequential processing:
 *    called anytime          seen by processors            JustBeforeDestruction
 *    during frame            injected BEFORE the           → Await → Finalize
 *                           Confirm→Await conversion       → Entity Destroyed
 *
 *
 * Processor Execution Order (within each frame):
 * ==============================================
 *
 * JustBeforeDestruction Injection Point:
 * ├── FProcessor_OwningActor_Destroy
 * ├── FProcessor_EntityLifetime_DestructionPhase_Finalize
 * └── FProcessor_EntityLifetime_DestructionPhase_Await
 *
 * ... (other regular processors and gameplay pump) ...
 *
 * End of Frame Processors:
 * ├── FProcessor_EntityLifetime_EntityJustCreated
 * ├── FProcessor_EntityLifetime_DestroyEntity
 * └── FProcessor_EntityLifetime_DestructionPhase_InitiateConfirm
 *
 *
 * Key Guarantees & Design Rationale:
 * ==================================
 *
 * 1. INITIATE Phase (Frame N):
 *    - Entity remains valid
 *    - Destroy() can be called anytime during frame
 *    - Not all regular processors guaranteed to run with Initiate state (timing dependent)
 *    - At END of frame: Initiate → Initiate_Confirm transition occurs
 *
 * 2. INITIATE_CONFIRM Phase (Frame N+1):
 *    - "Helper phase" for cascading destruction edge case
 *    - Problem: When teardown processors run, they may trigger destruction of OTHER entities
 *    - Without Initiate_Confirm, those newly-destroyed entities would skip to Await immediately
 *    - Solution: Initiate_Confirm ensures cascading destructions still get proper processor treatment
 *    - Only seen by processors injected BEFORE Initiate_Confirm → Await conversion
 *    - Most processors won't see this phase
 *
 * 3. AWAIT Phase (Frame N+1 → N+2):
 *    - The ONLY guaranteed full-frame destruction phase
 *    - Teardown processors run between Initiate_Confirm → Await transition
 *    - Entity handle remains VALID but component data may be torn down
 *    - Processors can work with Await entities but must handle missing/invalid data gracefully
 *    - Entity data has no guarantees after teardown processors have run
 *
 * 4. FINALIZE Phase (Frame N+2):
 *    - Sequential processing: Await → Finalize → Entity Destroyed (all at start of frame)
 *    - Entity becomes INVALID and memory is reclaimed
 *    - No processors run between these sequential operations
 *
 */

namespace ck
{
    // Defined in CkHandle.h to avoid circular dependency since it's needed for debugging purposes

    //CK_DEFINE_ECS_TAG(FTag_DestroyEntity_Finalize);
    //CK_DEFINE_ECS_TAG(FTag_DestroyEntity_Initiate);
    //CK_DEFINE_ECS_TAG(FTag_DestroyEntity_Initiate_Confirm);
    //CK_DEFINE_ECS_TAG(FTag_DestroyEntity_Await);
    //CK_DEFINE_ECS_TAG(FTag_EntityJustCreated);

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

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKECS_API, OnEntityBeginDestroy, FCk_Delegate_OnBeginDestroy_MC, FCk_Handle);
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKECS_API, OnEntityTeardown, FCk_Delegate_OnTeardown_MC, FCk_Handle);
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKECS_API, OnEntityDestroyed, FCk_Delegate_OnDestroy_MC, FCk_Entity);
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
