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
    // An absent Jolt world is legal (non-Jolt worlds): the whole tick silent-returns and the NeedsSetup
    // entities retry once a world exists.
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
        // WaitForAsync edge: without it the scheduler's lexical tie-break races CharacterVirtual creation
        // against the async step.
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
        // WaitForAsync edge: the Teleport handler mutates the CharacterVirtual and must never race the async step.
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
        FJoltWorld* _JoltWorld = nullptr;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Runs after HandleRequests (intents settled) and PlanStep (step order fixed); FProcessor_JoltWorld_Step
    // lists this in its RunAfter so the intents land in the SAME step they were pushed for.
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

    class CKJOLT_API FProcessor_JoltCharacter_EndPlay : public ck_exp::TProcessor<
            FProcessor_JoltCharacter_EndPlay,
            FCk_Handle_JoltCharacter,
            ck::TReadOnly<FFragment_JoltCharacter_Params>,
            ck::TReadWrite<FFragment_JoltCharacter_Current>,
            CK_IF_END_PLAY>
    {
    public:
        using Group = FGroup_EndPlay;
        // Non-runtime worlds never have a Jolt subsystem.
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
