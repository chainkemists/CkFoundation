#pragma once

#include "CkJolt/Body/CkJoltBody_Fragment.h"
#include "CkJolt/World/CkJoltWorld.h"
#include "CkJolt/World/CkJoltWorld_Processor.h"

#include "CkEcs/Entity/CkEntity.h"
#include "CkEcs/Processor/CkParallelProcessor.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"
#include "CkEcsExt/Transform/CkTransform_Processor.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>
#include <Jolt/Physics/EActivation.h>

// --------------------------------------------------------------------------------------------------------------------

// ReSharper disable once CppInconsistentNaming
namespace JPH
{
    class PhysicsSystem;
    class BodyInterface;
}

namespace ck::jolt
{
    class FCk_Jolt_CollisionLayerTable;
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Builds the Jolt body for each entity flagged NeedsSetup: shape, layer, mass/COM/surface, then a
    // single batched AddBodies pass (split by initial activation). The FJoltWorld/PhysicsSystem contexts
    // are resolved per-tick — an absent Jolt world is legal (non-Jolt worlds), so the whole tick silent-
    // returns and the NeedsSetup entities retry once a world exists.
    class CKJOLT_API FProcessor_JoltBody_Setup : public ck_exp::TProcessor<
            FProcessor_JoltBody_Setup,
            FCk_Handle_JoltBody,
            ck::TReadOnly<FFragment_JoltBody_Params>,
            ck::TReadWrite<FFragment_JoltBody_Current>,
            FTag_JoltBody_NeedsSetup,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Transform;
        using RunAfter = TDepList<FProcessor_Transform_HandleRequests>;
        using MarkedDirtyBy = FTag_JoltBody_NeedsSetup;

    public:
        using TProcessor::TProcessor;

    public:
        auto DoTick(TimeType InDeltaT) -> void;

        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_JoltBody_Params& InParams,
            FFragment_JoltBody_Current& InCurrent) -> void;

    private:
        // One created-but-not-yet-added body, pending the batched AddBodies pass.
        struct FPendingBody
        {
            FCk_Entity  _Entity;
            JPH::BodyID _BodyId;
        };

        auto
        DoBatchAdd(
            JPH::BodyInterface& InBodyInterface,
            TArray<FPendingBody>& InPending,
            JPH::EActivation InActivation) -> void;

    private:
        // Per-tick context, resolved in DoTick before the view iteration.
        TWeakPtr<JPH::PhysicsSystem>          _PhysicsSystem;
        ck::jolt::FCk_Jolt_CollisionLayerTable* _LayerTable = nullptr;

        // Per-tick accumulators, filled by ForEachEntity and drained by DoBatchAdd. Split by initial
        // activation because Jolt's batch AddBodiesFinalize takes ONE EActivation for the whole batch.
        TArray<FPendingBody> _PendingActivate;
        TArray<FPendingBody> _PendingDontActivate;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Drains the JoltBody request queue (CkTimer ritual). Today only SetSleepState — activates/deactivates
    // the body. Phase 4 adds the rest.
    class CKJOLT_API FProcessor_JoltBody_HandleRequests : public ck_exp::TProcessor<
            FProcessor_JoltBody_HandleRequests,
            FCk_Handle_JoltBody,
            ck::TReadWrite<FFragment_JoltBody_Current>,
            ck::TReadWrite<FFragment_JoltBody_Requests>,
            TExclude<FTag_JoltBody_NeedsSetup>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Transform;
        using RunAfter = TDepList<FProcessor_JoltBody_Setup>;
        using MarkedDirtyBy = FFragment_JoltBody_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto DoTick(TimeType InDeltaT) -> void;

        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_JoltBody_Current& InCurrent,
            FFragment_JoltBody_Requests& InRequestsComp) const -> void;

    private:
        auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_JoltBody_Current& InCurrent,
            const FCk_Request_JoltBody_SetSleepState& InRequest) const -> void;

    private:
        TWeakPtr<JPH::PhysicsSystem> _PhysicsSystem;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Drains the Jolt activation-event queue (produced off worker threads, drained game-thread) and mirrors
    // each body's awake/asleep state onto FTag_JoltBody_Sleeping. Tag-level only; signals are Phase 4.
    class CKJOLT_API FProcessor_JoltBody_SleepStateMirror : public TProcessorBase<FProcessor_JoltBody_SleepStateMirror>
    {
    public:
        using Group = FGroup_Transform;
        using RunAfter = TDepList<FProcessor_JoltWorld_PlanStep>;

    private:
        using Super = TProcessorBase;
        friend class Super;

    public:
        explicit FProcessor_JoltBody_SleepStateMirror(const RegistryType& InRegistry);

    public:
        auto DoTick(TimeType InDeltaT) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Pushes EVERY added ECS-driven kinematic body's Transform onto its Jolt body via MoveKinematic each
    // stepping frame (velocity-based move over the pending sim time, so it sweeps collisions instead of
    // teleporting). Deliberately NOT gated on FTag_Transform_Updated: Jolt's MoveKinematic sets a PERSISTENT
    // velocity that only a subsequent MoveKinematic zeroes — pushing to the current transform every stepping
    // frame (target == current ⇒ velocity zero) is the Jolt-idiomatic usage, and also guarantees a move
    // landing on a zero-step frame is delivered on the next stepping frame rather than dropped with the tag.
    // Whole-tick early-out on a zero-step frame (no sim time to move across — accepted cost).
    class CKJOLT_API FProcessor_JoltBody_KinematicPush : public ck_exp::TProcessor<
            FProcessor_JoltBody_KinematicPush,
            FCk_Handle_JoltBody,
            ck::TReadOnly<FFragment_JoltBody_Current>,
            ck::TReadOnly<FFragment_Transform>,
            FTag_JoltBody_KinematicFromECS,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Transform;
        using RunAfter = TDepList<FProcessor_JoltBody_SleepStateMirror, FProcessor_JoltBody_HandleRequests>;

    public:
        using TProcessor::TProcessor;

    public:
        auto DoTick(TimeType InDeltaT) -> void;

        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_JoltBody_Current& InCurrent,
            const FFragment_Transform& InTransform) const -> void;

    private:
        TWeakPtr<JPH::PhysicsSystem> _PhysicsSystem;
        float _PendingSimTime = 0.0f;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Interpolates each simulated body's Prev/Curr step pose by the world step alpha and writes it back onto
    // the entity's Transform (parallel, direct-write). Kinematic-from-ECS bodies are excluded — they are
    // driven by the ECS transform, not by the simulation.
    class CKJOLT_API FProcessor_JoltBody_WritebackInterpolated : public TParallelProcessor<
            FProcessor_JoltBody_WritebackInterpolated,
            FCk_Handle_Transform,
            ck::TReadWrite<FFragment_Transform>,
            ck::TReadWrite<FFragment_Transform_Previous>,
            ck::TReadOnly<FFragment_JoltBody_StepPose>,
            FTag_JoltBody_TransformDirty,
            TExclude<FTag_JoltBody_KinematicFromECS>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Transform;
        using RunAfter = TDepList<FProcessor_JoltWorld_Step>;
        using MarkedDirtyBy = FTag_JoltBody_TransformDirty;

    public:
        using TParallelProcessor::TParallelProcessor;

    public:
        auto DoTick(TimeType InDeltaT) -> void;

        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Transform& InTransform,
            FFragment_Transform_Previous& InPrevTransform,
            const FFragment_JoltBody_StepPose& InStepPose) const -> void;

    private:
        float _Alpha = 0.0f;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Frees the Jolt body slot when the entity dies: RemoveBody only if it is still added, but ALWAYS
    // DestroyBody (mirrors the Probe leak-fix), then releases the Jolt physics-ownership claim and reaps the
    // pose-buffer entry.
    class CKJOLT_API FProcessor_JoltBody_EndPlay : public ck_exp::TProcessor<
            FProcessor_JoltBody_EndPlay,
            FCk_Handle_JoltBody,
            ck::TReadOnly<FFragment_JoltBody_Params>,
            ck::TReadWrite<FFragment_JoltBody_Current>,
            CK_IF_END_PLAY>
    {
    public:
        using Group = FGroup_EndPlay;
        // Mirrors FProcessor_Probe_EndPlay: non-runtime worlds never have a Jolt subsystem, so running there
        // would fire the teardown ensure for bodies that were never created.
        static constexpr auto WorldTypeRequirement = ECk_ProcessorWorldTypeRequirement::RuntimeOnly;

    public:
        using TProcessor::TProcessor;

    public:
        auto DoTick(TimeType InDeltaT) -> void;

        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_JoltBody_Params& InParams,
            FFragment_JoltBody_Current& InCurrent) const -> void;

    private:
        TWeakPtr<JPH::PhysicsSystem> _PhysicsSystem;
        FJoltWorld* _JoltWorld = nullptr;
    };
}

// --------------------------------------------------------------------------------------------------------------------
