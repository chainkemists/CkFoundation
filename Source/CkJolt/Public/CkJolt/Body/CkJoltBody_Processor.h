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
    // An absent Jolt world is legal (non-Jolt worlds): the whole tick silent-returns and the NeedsSetup
    // entities retry once a world exists.
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
        // WaitForAsync edge: without it the scheduler's lexical tie-break races CreateBody against the async step.
        using RunAfter = TDepList<FProcessor_Transform_HandleRequests, FProcessor_JoltWorld_WaitForAsync>;
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
        TWeakPtr<JPH::PhysicsSystem>          _PhysicsSystem;
        ck::jolt::FCk_Jolt_CollisionLayerTable* _LayerTable = nullptr;
        FJoltWorld* _JoltWorld = nullptr;

        // Split by initial activation because Jolt's AddBodiesFinalize takes ONE EActivation per batch.
        TArray<FPendingBody> _PendingActivate;
        TArray<FPendingBody> _PendingDontActivate;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Params is ReadWrite (not ReadOnly as in Setup) because SetMotionType / SetCollisionProfile must keep the
    // params copy in step with the live body: Get_MotionType reads Params, and EndPlay reads Params to decide
    // whether to bump the static-scene revision. A stale copy would make both lie.
    class CKJOLT_API FProcessor_JoltBody_HandleRequests : public ck_exp::TProcessor<
            FProcessor_JoltBody_HandleRequests,
            FCk_Handle_JoltBody,
            ck::TReadWrite<FFragment_JoltBody_Params>,
            ck::TReadWrite<FFragment_JoltBody_Current>,
            ck::TReadWrite<FFragment_JoltBody_Requests>,
            TExclude<FTag_JoltBody_NeedsSetup>,
            TExclude<FTag_DestroyEntity_Initiate>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Transform;
        // WaitForAsync edge: the handlers mutate Jolt bodies and must never race an in-flight async step.
        using RunAfter = TDepList<FProcessor_JoltBody_Setup, FProcessor_JoltWorld_WaitForAsync>;
        using MarkedDirtyBy = FFragment_JoltBody_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto DoTick(TimeType InDeltaT) -> void;

        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_JoltBody_Params& InParams,
            FFragment_JoltBody_Current& InCurrent,
            FFragment_JoltBody_Requests& InRequestsComp) const -> void;

    private:
        auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_JoltBody_Current& InCurrent,
            const FCk_Request_JoltBody_SetSleepState& InRequest) const -> void;

        auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_JoltBody_Current& InCurrent,
            const FCk_Request_JoltBody_AddForce& InRequest) const -> void;

        auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_JoltBody_Current& InCurrent,
            const FCk_Request_JoltBody_AddForceAtLocation& InRequest) const -> void;

        auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_JoltBody_Current& InCurrent,
            const FCk_Request_JoltBody_AddTorque& InRequest) const -> void;

        auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_JoltBody_Current& InCurrent,
            const FCk_Request_JoltBody_AddImpulse& InRequest) const -> void;

        auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_JoltBody_Current& InCurrent,
            const FCk_Request_JoltBody_AddImpulseAtLocation& InRequest) const -> void;

        auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_JoltBody_Current& InCurrent,
            const FCk_Request_JoltBody_AddAngularImpulse& InRequest) const -> void;

        auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_JoltBody_Current& InCurrent,
            const FCk_Request_JoltBody_SetLinearVelocity& InRequest) const -> void;

        auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_JoltBody_Current& InCurrent,
            const FCk_Request_JoltBody_SetAngularVelocity& InRequest) const -> void;

        auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_JoltBody_Current& InCurrent,
            const FCk_Request_JoltBody_Teleport& InRequest) const -> void;

        // The two runtime mutators below also rewrite InParams, so they take it non-const.
        auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_JoltBody_Params& InParams,
            const FFragment_JoltBody_Current& InCurrent,
            const FCk_Request_JoltBody_SetMotionType& InRequest) const -> void;

        auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_JoltBody_Params& InParams,
            const FFragment_JoltBody_Current& InCurrent,
            const FCk_Request_JoltBody_SetCollisionProfile& InRequest) const -> void;

    private:
        TWeakPtr<JPH::PhysicsSystem> _PhysicsSystem;
        ck::jolt::FCk_Jolt_CollisionLayerTable* _LayerTable = nullptr;
        FJoltWorld* _JoltWorld = nullptr;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // HandleRequests excludes owners already tagged for destruction, so a destroyed body's still-queued
    // requests are never drained. This fires each pending request's completion delegate with
    // Failed_Cancelled so a caller awaiting completion terminates instead of hanging.
    class CKJOLT_API FProcessor_JoltBody_CancelPendingRequests : public ck_exp::TProcessor<
        FProcessor_JoltBody_CancelPendingRequests,
        FCk_Handle_JoltBody,
        ck::TReadOnly<FFragment_JoltBody_Requests>,
        CK_IF_END_PLAY>
    {
    public:
        using Group = FGroup_EndPlay;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_JoltBody_Requests& InRequestsComp)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Drains the Jolt activation-event queue — produced off worker threads, drained here on the game thread.
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

    // Deliberately NOT gated on FTag_Transform_Updated: Jolt's MoveKinematic sets a PERSISTENT velocity that
    // only a subsequent MoveKinematic zeroes, so pushing every stepping frame (target == current ⇒ velocity
    // zero) is the idiomatic usage and keeps a move landing on a zero-step frame from being dropped.
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

    // Kinematic-from-ECS bodies are excluded — they are driven by the ECS transform, not by the simulation.
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

    class CKJOLT_API FProcessor_JoltBody_EndPlay : public ck_exp::TProcessor<
            FProcessor_JoltBody_EndPlay,
            FCk_Handle_JoltBody,
            ck::TReadOnly<FFragment_JoltBody_Params>,
            ck::TReadWrite<FFragment_JoltBody_Current>,
            CK_IF_END_PLAY>
    {
    public:
        using Group = FGroup_EndPlay;
        // Non-runtime worlds never have a Jolt subsystem, so running there would fire the teardown ensure for
        // bodies that were never created.
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
