#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkJolt/CkJolt_ActivationEvent.h"
#include "CkJolt/CkJolt_ContactEvent.h"
#include "CkJolt/Character/CkJoltCharacter_Fragment_Data.h"

#include <Async/Future.h>

#include <CoreMinimal.h>

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Character/CharacterBase.h>

// --------------------------------------------------------------------------------------------------------------------

struct FCk_Handle;
class UWorld;

// ReSharper disable once CppInconsistentNaming
namespace JPH
{
    class PhysicsSystem;
    class TempAllocatorImpl;
    class JobSystem;
    class CharacterVirtual;
    class CharacterContactListener;
}

namespace ck
{
    class CkJoltCharacterContactListener;
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::jolt
{
    // One frame's fixed-timestep plan, derived purely from the accumulator + frame delta. Extracted from
    // FProcessor_JoltWorld_PlanStep so the math is pinnable by tests without a physics world.
    struct CKJOLT_API FCk_Jolt_StepPlan
    {
        float NewAccumulator = 0.0f;
        int32 NumSteps = 0;
        float Alpha = 0.0f;
        float PendingSimTime = 0.0f;
        float DroppedTime = 0.0f;
    };

    CKJOLT_API auto ComputeStepPlan(
        float InAccumulator,
        float InDeltaTSeconds,
        int32 InFixedTimestepHz,
        int32 InMaxStepsPerFrame) -> FCk_Jolt_StepPlan;
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

    // Per-registered-CharacterVirtual state. Character is a NON-owning raw pointer (the entity's
    // FFragment_JoltCharacter_Current holds the owning JPH::Ref). Registration is game-thread only.
    //
    // in-fields are written by the game thread (FProcessor_JoltCharacter_PreStep) BEFORE the step is kicked;
    // out-fields are written by the step loop (DoStepCharacters_AnyThread — may run on the task graph) and
    // read by the game thread (DoApplyCharacterPoses_GameThread) AFTER the async step is waited. HasJump is an
    // in-field ARMED by the game thread and CONSUMED (cleared) by the step loop — both accesses are serialized
    // by the WaitForAsync gate, so it is never touched concurrently.
    struct FCk_Jolt_CharacterEntry
    {
        JPH::CharacterVirtual* Character = nullptr;
        uint64                 UserData = 0;
        uint16                 ObjectLayer = 0;

        // in-fields (written game-thread pre-kick)
        FVector                        MoveVelocity = FVector::ZeroVector;
        float                          JumpVelocity = 0.0f;
        bool                           HasJump = false;
        ECk_JoltCharacter_PushPolicy   PushPolicy = ECk_JoltCharacter_PushPolicy::PushAndBePushed;

        // out-fields (written in the step loop)
        FVector                            OutLocation = FVector::ZeroVector;
        FQuat                              OutRotation = FQuat::Identity;
        JPH::CharacterBase::EGroundState   OutGroundState = JPH::CharacterBase::EGroundState::InAir;
        FVector                            OutGroundNormal = FVector::ZeroVector;
        FVector                            OutGroundVelocity = FVector::ZeroVector;
        bool                               DirtyThisFrame = false;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Runtime-internal Jolt-world step engine, owned by UCk_Jolt_Subsystem and published to the ECS
    // registry as a TSharedPtr<ck::FJoltWorld> context. The three FGroup_Transform step processors read
    // that context and drive it. Non-owning: every Jolt pointer below is owned by the subsystem, which
    // nulls them (Shutdown) before destroying the pointed-at objects.
    //
    // THREADING CONTRACT: the step while-loop (DoStepCharacters_AnyThread + DoPhysicsUpdate +
    // DoCapturePoses_AnyThread) may run on the task graph in async mode; it touches ONLY Jolt objects +
    // _PoseBuffer + the character registry's Character pointers and out-fields (plus the HasJump in-field it
    // consumes). The game thread never touches those while _AsyncFuture is pending
    // (FProcessor_JoltWorld_WaitForAsync consumes it first) — it writes character in-fields only before the
    // kick and reads out-fields only after the wait. DoApplyPoseBuffer_GameThread,
    // DoApplyCharacterPoses_GameThread, character registration, the contact routers, and the accumulator math
    // are game-thread only.
    struct CKJOLT_API FJoltWorld
    {
    public:
        CK_GENERATED_BODY(FJoltWorld);

        // Out-of-line so the TUniquePtr<CkJoltCharacterContactListener> member can hold an incomplete type in
        // this header (destroyed in the .cpp where the listener is complete).
        ~FJoltWorld();

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
            TFunction<void(TArray<FCk_Jolt_ActivationEvent>&)>     DrainActivationQueueFn;
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

        // ---- Activation events (game-thread only) ----
        // Drains the body activation/deactivation queue produced by the last step. The JoltBody
        // sleep-state mirror processor resolves entities and toggles FTag_JoltBody_Sleeping itself,
        // so — unlike contacts — these are handed back rather than routed.
        auto DrainActivationEvents(TArray<FCk_Jolt_ActivationEvent>& OutEvents) -> void;

        // Reaps a pose-buffer entry when its body is torn down (JoltBody EndPlay), keyed by the
        // body's index+sequence number (BodyID::GetIndexAndSequenceNumber).
        auto Remove_PoseBufferEntry(uint32 InBodyIndexAndSeq) -> void;

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

        // ---- Character registry (game-thread only for register/unregister/intent/apply) ----
        // Registers a CharacterVirtual (non-owning pointer) so the step loop drives it and the apply pass
        // pushes its pose onto the entity. Keyed by UserData (the versioned entity id).
        auto Register_Character(const FCk_Jolt_CharacterEntry& InEntry) -> void;
        auto Unregister_Character(uint64 InUserData) -> void;

        // Copies this frame's game-thread intent into the character's entry in-fields, pre-kick. MoveVelocity
        // and PushPolicy are continuous; a jump is one-shot (armed here, consumed by the step loop).
        auto Push_CharacterIntent(
            uint64 InUserData,
            const FVector& InMoveVelocity,
            ECk_JoltCharacter_PushPolicy InPushPolicy,
            bool InArmJump,
            float InJumpVelocity) -> void;

        // Teleport-equivalent of the body's pose-buffer reap: aligns the character entry's out-pose to the
        // teleport target and clears its dirty flag so a stale post-step out-pose cannot revert the teleport.
        // (Unlike a body, the whole registry entry is NOT removed — it holds the live Character pointer.)
        auto Snap_CharacterOutPose(uint64 InUserData, const FVector& InLocation, const FQuat& InRotation) -> void;

        // Resolves a registered character's authored PushPolicy by its CharacterVirtual pointer, for the
        // shared contact listener (called on the step thread; the registry is stable during the step).
        auto Get_CharacterPushPolicy(const JPH::CharacterVirtual* InCharacter) const -> ECk_JoltCharacter_PushPolicy;

        // The shared listener every CharacterVirtual is pointed at via SetListener during setup.
        auto Get_CharacterContactListener() const -> JPH::CharacterContactListener*;

        // Writes each dirty character entry's out-pose onto its entity's shared FFragment_JoltBody_StepPose
        // (adding FTag_JoltBody_TransformDirty), mirrors ground state/normal/velocity onto Current, and
        // broadcasts OnJoltCharacterGroundStateChanged on the ground-state edge. Game-thread only.
        auto DoApplyCharacterPoses_GameThread(const FCk_Handle& InTransientEntity) -> void;

        // Advances every registered character one fixed sub-step (ExtendedUpdate) and captures its out-fields.
        // NO registry/UObject access — may run on the task graph inside the step loop.
        auto DoStepCharacters_AnyThread(float InFixedDt) -> void;

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

        TFunction<void(TArray<FCk_Jolt_ContactEvent>&)>     _DrainQueueFn;
        TFunction<void(TArray<FCk_Jolt_ActivationEvent>&)>  _DrainActivationQueueFn;

        // ---- Step state ----
        float _Accumulator = 0.0f;
        float _Alpha = 0.0f;
        int32 _NumStepsLastFrame = 0;
        float _PendingSimTime = 0.0f;                    // sim time the current step batch advances; quartet uses it later
        bool  _OptimizeBroadPhaseRequested = false;
        TFuture<void> _AsyncFuture;

        // ---- Pose buffer, keyed by BodyID index+sequence (stable while a body is alive) ----
        TMap<uint32, FCk_Jolt_StepPoseEntry> _PoseBuffer;

        // ---- Character registry (non-owning Character pointers) + the shared contact listener they use ----
        TArray<FCk_Jolt_CharacterEntry>              _CharacterRegistry;
        TUniquePtr<CkJoltCharacterContactListener>   _CharacterContactListener;

        // ---- Contact routers, invoked in registration order ----
        TArray<TPair<FName, FCk_Jolt_ContactEventRouter>> _ContactRouters;

    private:
        auto Find_CharacterEntry(uint64 InUserData) -> FCk_Jolt_CharacterEntry*;

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
