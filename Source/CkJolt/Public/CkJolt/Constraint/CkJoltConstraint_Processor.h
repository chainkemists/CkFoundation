#pragma once

#include "CkJolt/Body/CkJoltBody_Fragment.h"
#include "CkJolt/Body/CkJoltBody_Processor.h"
#include "CkJolt/Constraint/CkJoltConstraint_Fragment.h"
#include "CkJolt/World/CkJoltWorld.h"
#include "CkJolt/World/CkJoltWorld_Processor.h"

#include "CkEcs/Entity/CkEntity.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include <Jolt/Jolt.h>

// --------------------------------------------------------------------------------------------------------------------

// ReSharper disable once CppInconsistentNaming
namespace JPH
{
    class PhysicsSystem;
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKJOLT_API FProcessor_JoltConstraint_Setup : public ck_exp::TProcessor<
            FProcessor_JoltConstraint_Setup,
            FCk_Handle_JoltConstraint,
            ck::TReadOnly<FFragment_JoltConstraint_Params>,
            ck::TReadWrite<FFragment_JoltConstraint_Current>,
            FTag_JoltConstraint_NeedsSetup,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Transform;
        // WaitForAsync edge: CreateConstraint/AddConstraint must never race an in-flight async step.
        using RunAfter = TDepList<FProcessor_JoltBody_Setup, FProcessor_JoltWorld_WaitForAsync>;
        using MarkedDirtyBy = FTag_JoltConstraint_NeedsSetup;

    public:
        using TProcessor::TProcessor;

    public:
        auto DoTick(TimeType InDeltaT) -> void;

        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_JoltConstraint_Params& InParams,
            FFragment_JoltConstraint_Current& InCurrent) -> void;

    private:
        TWeakPtr<JPH::PhysicsSystem> _PhysicsSystem;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKJOLT_API FProcessor_JoltConstraint_HandleRequests : public ck_exp::TProcessor<
            FProcessor_JoltConstraint_HandleRequests,
            FCk_Handle_JoltConstraint,
            ck::TReadOnly<FFragment_JoltConstraint_Params>,
            ck::TReadWrite<FFragment_JoltConstraint_Current>,
            ck::TReadWrite<FFragment_JoltConstraint_Requests>,
            TExclude<FTag_JoltConstraint_NeedsSetup>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Transform;
        // WaitForAsync edge: the handlers mutate JPH constraints and must never race an in-flight step.
        using RunAfter = TDepList<FProcessor_JoltConstraint_Setup, FProcessor_JoltWorld_WaitForAsync>;
        using MarkedDirtyBy = FFragment_JoltConstraint_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto DoTick(TimeType InDeltaT) -> void;

        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_JoltConstraint_Params& InParams,
            FFragment_JoltConstraint_Current& InCurrent,
            FFragment_JoltConstraint_Requests& InRequestsComp) const -> void;

    private:
        auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_JoltConstraint_Params& InParams,
            const FFragment_JoltConstraint_Current& InCurrent,
            const FCk_Request_JoltConstraint_SetEnabled& InRequest) const -> void;

        auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_JoltConstraint_Params& InParams,
            const FFragment_JoltConstraint_Current& InCurrent,
            const FCk_Request_JoltConstraint_Distance_SetRange& InRequest) const -> void;

        auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_JoltConstraint_Params& InParams,
            const FFragment_JoltConstraint_Current& InCurrent,
            const FCk_Request_JoltConstraint_Hinge_SetMotor& InRequest) const -> void;

    private:
        TWeakPtr<JPH::PhysicsSystem> _PhysicsSystem;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKJOLT_API FProcessor_JoltConstraint_LivenessReap : public ck_exp::TProcessor<
            FProcessor_JoltConstraint_LivenessReap,
            FCk_Handle_JoltConstraint,
            ck::TReadWrite<FFragment_JoltConstraint_Current>,
            TExclude<FTag_JoltConstraint_NeedsSetup>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_EndPlay;
        // Constraints must be gone from Jolt BEFORE their bodies are freed.
        using RunBefore = TDepList<FProcessor_JoltBody_EndPlay>;

    public:
        using TProcessor::TProcessor;

    public:
        auto DoTick(TimeType InDeltaT) -> void;

        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_JoltConstraint_Current& InCurrent) const -> void;

    private:
        TWeakPtr<JPH::PhysicsSystem> _PhysicsSystem;
        FJoltWorld* _JoltWorld = nullptr;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Runs before FProcessor_JoltBody_EndPlay so a cascade-destroyed constraint frees before its bodies do.
    class CKJOLT_API FProcessor_JoltConstraint_EndPlay : public ck_exp::TProcessor<
            FProcessor_JoltConstraint_EndPlay,
            FCk_Handle_JoltConstraint,
            ck::TReadWrite<FFragment_JoltConstraint_Current>,
            CK_IF_END_PLAY>
    {
    public:
        using Group = FGroup_EndPlay;
        using RunAfter = TDepList<FProcessor_JoltConstraint_LivenessReap>;
        using RunBefore = TDepList<FProcessor_JoltBody_EndPlay>;
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
            FFragment_JoltConstraint_Current& InCurrent) const -> void;

    private:
        TWeakPtr<JPH::PhysicsSystem> _PhysicsSystem;
        FJoltWorld* _JoltWorld = nullptr;
    };
}

// --------------------------------------------------------------------------------------------------------------------
