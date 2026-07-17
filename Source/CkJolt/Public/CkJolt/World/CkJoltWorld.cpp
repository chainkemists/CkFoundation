#include "CkJoltWorld.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "CkEcs/Entity/CkEntity.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Registry/CkRegistry.h"

#include "CkJolt/Body/CkJoltBody_Fragment.h"
#include "CkJolt/CkJolt_Utils.h"

#include <Jolt/Jolt.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Body/BodyType.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::jolt
{
    auto
        ComputeStepPlan(
            float InAccumulator,
            float InDeltaTSeconds,
            int32 InFixedTimestepHz,
            int32 InMaxStepsPerFrame)
        -> FCk_Jolt_StepPlan
    {
        const auto FixedHz = FMath::Max(1, InFixedTimestepHz);
        const auto FixedDt = 1.0f / static_cast<float>(FixedHz);

        auto Plan = FCk_Jolt_StepPlan{};
        auto Accumulator = InAccumulator + InDeltaTSeconds;

        // NO ensure in this clamp path — a spiral-of-death under load is expected; excess time is dropped.
        const auto MaxAccum = static_cast<float>(InMaxStepsPerFrame) * FixedDt;
        if (Accumulator > MaxAccum)
        {
            Plan.DroppedTime = Accumulator - MaxAccum;
            Accumulator = MaxAccum;
        }

        Plan.NumSteps = FMath::FloorToInt(Accumulator / FixedDt);
        Accumulator -= static_cast<float>(Plan.NumSteps) * FixedDt;
        Plan.NewAccumulator = Accumulator;
        Plan.Alpha = Accumulator / FixedDt;
        Plan.PendingSimTime = static_cast<float>(Plan.NumSteps) * FixedDt;

        return Plan;
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    FJoltWorld::
        FJoltWorld(
            FInitParams InParams)
        : _PhysicsSystem(InParams.PhysicsSystem)
        , _TempAllocator(InParams.TempAllocator)
        , _JobSystem(InParams.JobSystem)
        , _World(InParams.World)
        , _CollisionSteps(InParams.CollisionSteps)
        , _AsyncMode(InParams.AsyncMode)
        , _DrainQueueFn(MoveTemp(InParams.DrainQueueFn))
        , _DrainActivationQueueFn(MoveTemp(InParams.DrainActivationQueueFn))
    {
    }

    auto
        FJoltWorld::
        Shutdown()
        -> void
    {
        if (_AsyncFuture.IsValid())
        {
            _AsyncFuture.Wait();
            _AsyncFuture = {};
        }

        _PhysicsSystem = nullptr;
        _TempAllocator = nullptr;
        _JobSystem = nullptr;
        _DrainQueueFn = {};
        _DrainActivationQueueFn = {};
        _World = nullptr;
        _ContactRouters.Empty();
        _PoseBuffer.Empty();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FJoltWorld::
        RegisterContactRouter(
            FName InName,
            FCk_Jolt_ContactEventRouter InRouter)
        -> void
    {
        const auto ExistingIndex = _ContactRouters.IndexOfByPredicate([&](const auto& InPair)
        {
            return InPair.Key == InName;
        });

        CK_ENSURE_IF_NOT(ExistingIndex == INDEX_NONE,
            TEXT("A contact router named [{}] is already registered on the Jolt world."), InName)
        { return; }

        _ContactRouters.Emplace(InName, MoveTemp(InRouter));
    }

    auto
        FJoltWorld::
        UnregisterContactRouter(
            FName InName)
        -> void
    {
        _ContactRouters.RemoveAll([&](const auto& InPair)
        {
            return InPair.Key == InName;
        });
    }

    auto
        FJoltWorld::
        DrainEventsAndRoute()
        -> void
    {
        if (NOT _DrainQueueFn)
        { return; }

        auto Events = TArray<FCk_Jolt_ContactEvent>{};
        _DrainQueueFn(Events);

        if (Events.IsEmpty())
        { return; }

        for (const auto& Router : _ContactRouters)
        { Router.Value(Events); }
    }

    auto
        FJoltWorld::
        DrainActivationEvents(
            TArray<FCk_Jolt_ActivationEvent>& OutEvents)
        -> void
    {
        if (NOT _DrainActivationQueueFn)
        { return; }

        _DrainActivationQueueFn(OutEvents);
    }

    auto
        FJoltWorld::
        Remove_PoseBufferEntry(
            uint32 InBodyIndexAndSeq)
        -> void
    {
        _PoseBuffer.Remove(InBodyIndexAndSeq);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FJoltWorld::
        Request_OptimizeBroadPhaseBeforeNextUpdate()
        -> void
    {
        _OptimizeBroadPhaseRequested = true;
    }

    auto
        FJoltWorld::
        DoOptimizeBroadPhase()
        -> void
    {
        const auto PhysicsSystem = _PhysicsSystem.Pin();
        if (NOT PhysicsSystem.IsValid())
        { return; }

        PhysicsSystem->OptimizeBroadPhase();
        _OptimizeBroadPhaseRequested = false;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FJoltWorld::
        DoPhysicsUpdate(
            float InFixedDt)
        -> void
    {
        const auto PhysicsSystem = _PhysicsSystem.Pin();
        if (NOT PhysicsSystem.IsValid())
        { return; }

        PhysicsSystem->Update(InFixedDt, _CollisionSteps, _TempAllocator, _JobSystem);
    }

    auto
        FJoltWorld::
        DoCapturePoses_AnyThread()
        -> void
    {
        const auto PhysicsSystem = _PhysicsSystem.Pin();
        if (NOT PhysicsSystem.IsValid())
        { return; }

        auto ActiveBodies = JPH::BodyIDVector{};
        PhysicsSystem->GetActiveBodies(JPH::EBodyType::RigidBody, ActiveBodies);

        // NoLock is safe here: PhysicsSystem::Update has returned on this thread, so no worker is mutating
        // bodies, and the game thread does not touch Jolt while an async step is pending (see class contract).
        const auto& BodyInterface = PhysicsSystem->GetBodyInterfaceNoLock();

        for (const auto BodyId : ActiveBodies)
        {
            auto Position = JPH::RVec3{};
            auto Rotation = JPH::Quat{};
            BodyInterface.GetPositionAndRotation(BodyId, Position, Rotation);

            const auto UserData = BodyInterface.GetUserData(BodyId);
            const auto Location = ck::jolt::Conv(Position);
            const auto RotationUe = ck::jolt::Conv(Rotation);
            const auto Key = BodyId.GetIndexAndSequenceNumber();

            if (auto* Existing = _PoseBuffer.Find(Key))
            {
                Existing->PrevLocation = Existing->CurrLocation;
                Existing->PrevRotation = Existing->CurrRotation;
                Existing->CurrLocation = Location;
                Existing->CurrRotation = RotationUe;
                Existing->UserData = UserData;
                Existing->DirtyThisFrame = true;
            }
            else
            {
                auto Entry = FCk_Jolt_StepPoseEntry{};
                Entry.UserData = UserData;
                Entry.PrevLocation = Location;
                Entry.PrevRotation = RotationUe;
                Entry.CurrLocation = Location;
                Entry.CurrRotation = RotationUe;
                Entry.DirtyThisFrame = true;
                _PoseBuffer.Add(Key, Entry);
            }
        }
    }

    auto
        FJoltWorld::
        DoApplyPoseBuffer_GameThread(
            const FCk_Handle& InTransientEntity)
        -> void
    {
        const auto RegView = InTransientEntity.Get_RegistryView();

        auto DeadKeys = TArray<uint32>{};

        for (auto& Pair : _PoseBuffer)
        {
            auto& Entry = Pair.Value;

            if (NOT Entry.DirtyThisFrame)
            { continue; }

            // Body UserData is a raw (versioned) entity id. A snapshot load wipes/restores the registry, so
            // a buffered pose can resolve to an id that is dead in the fresh registry — do a non-ensuring
            // liveness check first (mirrors ResolveBodyEntity in CkSpatialQuery_Subsystem).
            const auto Entity = FCk_Entity{FCk_Entity::IdType{static_cast<FCk_Entity::IdType>(Entry.UserData)}};
            if (NOT RegView.IsValid(Entity))
            {
                DeadKeys.Add(Pair.Key);
                continue;
            }

            auto Handle = InTransientEntity.Get_ValidHandle(Entity.Get_ID());
            if (ck::Is_NOT_Valid(Handle) || NOT Handle.Has<ck::FFragment_JoltBody_StepPose>())
            {
                Entry.DirtyThisFrame = false;
                continue;
            }

            // An entity may own MORE Jolt bodies than its JoltBody (e.g. a Probe) — all share the entity id
            // as UserData, and _PoseBuffer is keyed by body id. Only the JoltBody's own body may write the
            // entity's StepPose; another body's entry (Probe) must not clobber the simulated pose.
            if (NOT Handle.Has<ck::FFragment_JoltBody_Current>() ||
                Handle.Get<ck::FFragment_JoltBody_Current>().Get_BodyId().GetIndexAndSequenceNumber() != Pair.Key)
            {
                Entry.DirtyThisFrame = false;
                continue;
            }

            auto& Pose = Handle.Get<ck::FFragment_JoltBody_StepPose>();
            Pose.Set_PrevLocation(Entry.PrevLocation)
                .Set_PrevRotation(Entry.PrevRotation)
                .Set_CurrLocation(Entry.CurrLocation)
                .Set_CurrRotation(Entry.CurrRotation);

            if (NOT Handle.Has<ck::FTag_JoltBody_TransformDirty>())
            { Handle.Add<ck::FTag_JoltBody_TransformDirty>(); }

            Entry.DirtyThisFrame = false;
        }

        for (const auto Key : DeadKeys)
        { _PoseBuffer.Remove(Key); }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FJoltWorld::
        Set_PendingAsyncStep(
            TFuture<void>&& InFuture)
        -> void
    {
        _AsyncFuture = MoveTemp(InFuture);
    }

    auto
        FJoltWorld::
        WaitForAsyncStep()
        -> void
    {
        // Callers gate on Get_AsyncFuture().IsValid() and own the SCOPE_CYCLE_COUNTER around this.
        _AsyncFuture.Wait();
        _AsyncFuture = {};
    }
}

// --------------------------------------------------------------------------------------------------------------------
