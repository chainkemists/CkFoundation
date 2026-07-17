#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkJolt/CkJolt_ContactEvent.h"

#include <Async/Future.h>

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------

struct FCk_Handle;
class UWorld;

// ReSharper disable once CppInconsistentNaming
namespace JPH
{
    class PhysicsSystem;
    class TempAllocatorImpl;
    class JobSystem;
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // A registered consumer of a drained batch of contact events. Invoked on the game thread, in
    // registration order, inside FProcessor_JoltWorld_DrainEvents.
    using FCk_Jolt_ContactEventRouter = TFunction<void(const TArray<FCk_Jolt_ContactEvent>&)>;

    // --------------------------------------------------------------------------------------------------------------------

    // Per-active-body interpolation state captured off the simulated Jolt world (UE-space). Prev holds
    // last frame's pose, Curr this frame's; DirtyThisFrame gates the game-thread apply pass so a
    // body untouched this frame is not re-applied.
    struct FCk_Jolt_StepPoseEntry
    {
        uint64  UserData = 0;
        FVector PrevLocation = FVector::ZeroVector;
        FQuat   PrevRotation = FQuat::Identity;
        FVector CurrLocation = FVector::ZeroVector;
        FQuat   CurrRotation = FQuat::Identity;
        bool    DirtyThisFrame = false;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Runtime-internal Jolt-world step engine, owned by UCk_Jolt_Subsystem and published to the ECS
    // registry as a TSharedPtr<ck::FJoltWorld> context. The three FGroup_Transform step processors read
    // that context and drive it. Non-owning: every Jolt pointer below is owned by the subsystem, which
    // nulls them (Shutdown) before destroying the pointed-at objects.
    //
    // THREADING CONTRACT: the step while-loop (DoPhysicsUpdate + DoCapturePoses_AnyThread) may run on the
    // task graph in async mode; it touches ONLY Jolt objects + _PoseBuffer. The game thread never touches
    // those while _AsyncFuture is pending (FProcessor_JoltWorld_WaitForAsync consumes it first).
    // DoApplyPoseBuffer_GameThread + the contact routers + the accumulator math are game-thread only.
    struct CKJOLT_API FJoltWorld
    {
    public:
        CK_GENERATED_BODY(FJoltWorld);

    public:
        // Populated once by the subsystem in Initialize. Pointers are non-owning.
        struct FInitParams
        {
            TWeakPtr<JPH::PhysicsSystem>                            PhysicsSystem;
            JPH::TempAllocatorImpl*                                TempAllocator = nullptr;
            JPH::JobSystem*                                        JobSystem = nullptr;
            TWeakObjectPtr<UWorld>                                 World;
            int32                                                  CollisionSteps = 1;
            bool                                                   AsyncMode = false;
            TFunction<void(TArray<FCk_Jolt_ContactEvent>&)>        DrainQueueFn;
        };

    public:
        explicit FJoltWorld(FInitParams InParams);

        // Waits any in-flight async step, then nulls the non-owning Jolt pointers so a processor that
        // reads a still-live context after teardown resolves them as null and returns silently. Called by
        // the subsystem's Deinitialize BEFORE it destroys the pointed-at objects.
        auto Shutdown() -> void;

    public:
        // ---- Contact routing (game-thread only) ----
        auto RegisterContactRouter(FName InName, FCk_Jolt_ContactEventRouter InRouter) -> void;
        auto UnregisterContactRouter(FName InName) -> void;
        auto DrainEventsAndRoute() -> void;

        // ---- Broadphase (game-thread only; safe once the async future is consumed upstream) ----
        auto Request_OptimizeBroadPhaseBeforeNextUpdate() -> void;
        auto DoOptimizeBroadPhase() -> void;

        // ---- Step primitives ----
        // Advances the simulation one fixed sub-step. Jolt-only; task-graph safe.
        auto DoPhysicsUpdate(float InFixedDt) -> void;
        // Snapshots every active rigid body's pose into _PoseBuffer. NO registry/UObject access — may run
        // on the task graph.
        auto DoCapturePoses_AnyThread() -> void;
        // Writes buffered dirty poses onto their entity's FFragment_JoltBody_StepPose (adding the dirty tag)
        // and reaps dead-entity entries. Game-thread only.
        auto DoApplyPoseBuffer_GameThread(const FCk_Handle& InTransientEntity) -> void;

        // ---- Async future (game-thread only) ----
        auto Set_PendingAsyncStep(TFuture<void>&& InFuture) -> void;
        auto WaitForAsyncStep() -> void;

    private:
        TWeakPtr<JPH::PhysicsSystem>                     _PhysicsSystem;
        JPH::TempAllocatorImpl*                          _TempAllocator = nullptr;
        JPH::JobSystem*                                  _JobSystem = nullptr;
        TWeakObjectPtr<UWorld>                           _World;
        int32                                            _CollisionSteps = 1;
        bool                                             _AsyncMode = false;

        TFunction<void(TArray<FCk_Jolt_ContactEvent>&)>  _DrainQueueFn;

        // ---- Step state ----
        float _Accumulator = 0.0f;
        float _Alpha = 0.0f;
        int32 _NumStepsLastFrame = 0;
        float _PendingSimTime = 0.0f;                    // sim time the current step batch advances; quartet uses it later
        bool  _OptimizeBroadPhaseRequested = false;
        TFuture<void> _AsyncFuture;

        // ---- Pose buffer, keyed by BodyID index+sequence (stable while a body is alive) ----
        TMap<uint32, FCk_Jolt_StepPoseEntry> _PoseBuffer;

        // ---- Contact routers, invoked in registration order ----
        TArray<TPair<FName, FCk_Jolt_ContactEventRouter>> _ContactRouters;

    public:
        CK_PROPERTY_GET(_World);
        CK_PROPERTY_GET(_AsyncMode);
        CK_PROPERTY(_Accumulator);
        CK_PROPERTY(_Alpha);
        CK_PROPERTY(_NumStepsLastFrame);
        CK_PROPERTY(_PendingSimTime);
        CK_PROPERTY_GET(_OptimizeBroadPhaseRequested);
        CK_PROPERTY_GET(_AsyncFuture);
    };
}

// --------------------------------------------------------------------------------------------------------------------
