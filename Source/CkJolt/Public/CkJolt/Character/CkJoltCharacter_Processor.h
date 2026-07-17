#pragma once

#include "CkJolt/Character/CkJoltCharacter_Fragment.h"
#include "CkJolt/World/CkJoltWorld.h"
#include "CkJolt/World/CkJoltWorld_Processor.h"

#include "CkEcs/Entity/CkEntity.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"
#include "CkEcsExt/Transform/CkTransform_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

// ReSharper disable once CppInconsistentNaming
namespace JPH
{
    class PhysicsSystem;
}

namespace ck::jolt
{
    class FCk_Jolt_CollisionLayerTable;
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Builds the CharacterVirtual for each entity flagged NeedsSetup: capsule shape, profile-derived layer,
    // Z-up settings, then registers it with the FJoltWorld and points it at the shared contact listener. The
    // FJoltWorld / PhysicsSystem / layer-table contexts are resolved per-tick — an absent Jolt world is legal
    // (non-Jolt worlds), so the whole tick silent-returns and the NeedsSetup entities retry once a world exists.
    class CKJOLT_API FProcessor_JoltCharacter_Setup : public ck_exp::TProcessor<
            FProcessor_JoltCharacter_Setup,
            FCk_Handle_JoltCharacter,
            ck::TReadOnly<FFragment_JoltCharacter_Params>,
            ck::TReadWrite<FFragment_JoltCharacter_Current>,
            FTag_JoltCharacter_NeedsSetup,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Transform;
        // WaitForAsync edge: creating a CharacterVirtual mutates Jolt state on the game thread and must never
        // race an in-flight async step (the scheduler's lexical tie-break would otherwise order this first).
        using RunAfter = TDepList<FProcessor_Transform_HandleRequests, FProcessor_JoltWorld_WaitForAsync>;
        using MarkedDirtyBy = FTag_JoltCharacter_NeedsSetup;

    public:
        using TProcessor::TProcessor;

    public:
        auto DoTick(TimeType InDeltaT) -> void;

        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_JoltCharacter_Params& InParams,
            FFragment_JoltCharacter_Current& InCurrent) -> void;

    private:
        TWeakPtr<JPH::PhysicsSystem>            _PhysicsSystem;
        ck::jolt::FCk_Jolt_CollisionLayerTable* _LayerTable = nullptr;
        FJoltWorld*                             _JoltWorld = nullptr;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Drains the JoltCharacter request queue (CkTimer ritual): Move / Jump write the pending intent onto
    // Current (drained into the FJoltWorld entry by PreStep); Teleport snaps the CharacterVirtual, the ECS
    // transform, and the step pose (mirrors the JoltBody Teleport handler).
    class CKJOLT_API FProcessor_JoltCharacter_HandleRequests : public ck_exp::TProcessor<
            FProcessor_JoltCharacter_HandleRequests,
            FCk_Handle_JoltCharacter,
            ck::TReadWrite<FFragment_JoltCharacter_Current>,
            ck::TReadWrite<FFragment_JoltCharacter_Requests>,
            TExclude<FTag_JoltCharacter_NeedsSetup>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Transform;
        // WaitForAsync edge: the Teleport handler mutates the CharacterVirtual (SetPosition/SetRotation) and
        // must never race an in-flight async step — without the explicit edge the lexical tie-break runs first.
        using RunAfter = TDepList<FProcessor_JoltCharacter_Setup, FProcessor_JoltWorld_WaitForAsync>;
        using MarkedDirtyBy = FFragment_JoltCharacter_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto DoTick(TimeType InDeltaT) -> void;

        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_JoltCharacter_Current& InCurrent,
            FFragment_JoltCharacter_Requests& InRequestsComp) const -> void;

    private:
        auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_JoltCharacter_Current& InCurrent,
            const FCk_Request_JoltCharacter_Move& InRequest) const -> void;

        auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_JoltCharacter_Current& InCurrent,
            const FCk_Request_JoltCharacter_Jump& InRequest) const -> void;

        auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_JoltCharacter_Current& InCurrent,
            const FCk_Request_JoltCharacter_Teleport& InRequest) const -> void;

    private:
        // Teleport must also snap the character's FJoltWorld out-pose entry (see the handler) — resolved per tick.
        FJoltWorld* _JoltWorld = nullptr;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Drains each set-up character's pending intents + push policy into its FJoltWorld entry in-fields on the
    // game thread, BEFORE the step is kicked. Runs after HandleRequests (intents settled) and PlanStep (so the
    // step order is fixed); FProcessor_JoltWorld_Step lists it in RunAfter so intents land in the same step.
    class CKJOLT_API FProcessor_JoltCharacter_PreStep : public ck_exp::TProcessor<
            FProcessor_JoltCharacter_PreStep,
            FCk_Handle_JoltCharacter,
            ck::TReadOnly<FFragment_JoltCharacter_Params>,
            ck::TReadWrite<FFragment_JoltCharacter_Current>,
            TExclude<FTag_JoltCharacter_NeedsSetup>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Transform;
        using RunAfter = TDepList<
            FProcessor_JoltCharacter_HandleRequests,
            FProcessor_JoltWorld_PlanStep,
            FProcessor_JoltWorld_WaitForAsync>;

    public:
        using TProcessor::TProcessor;

    public:
        auto DoTick(TimeType InDeltaT) -> void;

        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_JoltCharacter_Params& InParams,
            FFragment_JoltCharacter_Current& InCurrent) const -> void;

    private:
        FJoltWorld* _JoltWorld = nullptr;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Frees the character when the entity dies: releases the Jolt physics-ownership claim, unregisters from the
    // FJoltWorld registry, then drops the owning Ref (which destroys the CharacterVirtual). A CharacterVirtual
    // is NOT in the body interface, so there is no body to Remove/Destroy.
    class CKJOLT_API FProcessor_JoltCharacter_EndPlay : public ck_exp::TProcessor<
            FProcessor_JoltCharacter_EndPlay,
            FCk_Handle_JoltCharacter,
            ck::TReadOnly<FFragment_JoltCharacter_Params>,
            ck::TReadWrite<FFragment_JoltCharacter_Current>,
            CK_IF_END_PLAY>
    {
    public:
        using Group = FGroup_EndPlay;
        // Mirrors FProcessor_JoltBody_EndPlay: non-runtime worlds never have a Jolt subsystem.
        static constexpr auto WorldTypeRequirement = ECk_ProcessorWorldTypeRequirement::RuntimeOnly;

    public:
        using TProcessor::TProcessor;

    public:
        auto DoTick(TimeType InDeltaT) -> void;

        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_JoltCharacter_Params& InParams,
            FFragment_JoltCharacter_Current& InCurrent) const -> void;

    private:
        FJoltWorld* _JoltWorld = nullptr;
    };
}

// --------------------------------------------------------------------------------------------------------------------
