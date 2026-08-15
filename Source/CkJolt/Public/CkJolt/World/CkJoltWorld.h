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
    // One frame's fixed-timestep plan — pure math over accumulator + frame delta, so tests can pin it.
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

    // Per-active-body interpolation state captured off the simulated Jolt world (UE-space).
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
    // FFragment_JoltCharacter_Current holds the owning JPH::Ref). Registration is game-thread only; the
    // in/out split below is serialized by the WaitForAsync gate, so nothing here is touched concurrently.
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

    // Runtime-internal Jolt-world step engine, owned by UCk_Jolt_Subsystem and published to the ECS registry
    // as a TSharedPtr<ck::FJoltWorld> context that the FGroup_Transform step processors drive. Every Jolt
    // pointer below is NON-owning (the subsystem nulls them in Shutdown before destroying them). The
    // _AnyThread/_GameThread suffixes are the threading contract; rationale: CkJolt/CLAUDE.md § Threading model.
    struct CKJOLT_API FJoltWorld
    {
    public:
        CK_GENERATED_BODY(FJoltWorld);

        // Out-of-line so the TUniquePtr<CkJoltCharacterContactListener> member can hold an incomplete type here.
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
            TFunction<void(bool)>                                  SetPersistedContactsWantedFn;
        };

    public:
        explicit FJoltWorld(FInitParams InParams);

        // Waits any in-flight async step, then nulls the Jolt pointers so a processor reading a still-live
        // context after teardown resolves them as null and returns silently.
        auto Shutdown() -> void;

    public:
        // ---- Contact routing (game-thread only) ----
        auto RegisterContactRouter(FName InName, FCk_Jolt_ContactEventRouter InRouter) -> void;
        auto UnregisterContactRouter(FName InName) -> void;
        auto DrainEventsAndRoute() -> void;

        // ---- Persisted-contact interest (game-thread only) ----
        // A named vote for "something wants Persisted contact events". Every vote is re-asked once
        // per DrainEventsAndRoute and the OR is published to the contact listener, so a consumer that
        // stops wanting them (or unregisters) opts out on its own — there is no refcount to leak, and
        // a leak here would silently keep the per-manifold allocation cost alive forever.
        auto Register_PersistedContactInterestProvider(FName InName, TFunction<bool()> InProvider) -> void;
        auto Unregister_PersistedContactInterestProvider(FName InName) -> void;

        // ---- Activation events (game-thread only) ----
        // Handed back rather than routed: FProcessor_JoltBody_SleepStateMirror resolves the entities and
        // toggles FTag_JoltBody_Sleeping itself.
        auto DrainActivationEvents(TArray<FCk_Jolt_ActivationEvent>& OutEvents) -> void;

        auto Remove_PoseBufferEntry(uint32 InBodyIndexAndSeq) -> void;

        // ---- Broadphase (game-thread only; safe once the async future is consumed upstream) ----
        auto Request_OptimizeBroadPhaseBeforeNextUpdate() -> void;
        auto DoOptimizeBroadPhase() -> void;

        // ---- Step primitives ----
        // Advances the simulation one fixed sub-step. Jolt-only; task-graph safe.
        auto DoPhysicsUpdate(float InFixedDt) -> void;
        // Snapshots every active rigid body's pose into _PoseBuffer. NO registry/UObject access.
        auto DoCapturePoses_AnyThread() -> void;
        auto DoApplyPoseBuffer_GameThread(const FCk_Handle& InTransientEntity) -> void;

        // ---- Character registry (game-thread only for register/unregister/intent/apply) ----
        // Non-owning pointer, keyed by UserData (the versioned entity id).
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

        // Teleport-equivalent of the body's pose-buffer reap, except the entry is NOT removed (it holds the
        // live Character pointer): the dirty flag is cleared so a stale out-pose cannot revert the teleport.
        auto Snap_CharacterOutPose(uint64 InUserData, const FVector& InLocation, const FQuat& InRotation) -> void;

        // For the shared contact listener: called on the step thread, where the registry is stable.
        auto Get_CharacterPushPolicy(const JPH::CharacterVirtual* InCharacter) const -> ECk_JoltCharacter_PushPolicy;

        // The shared listener every CharacterVirtual is pointed at via SetListener during setup.
        auto Get_CharacterContactListener() const -> JPH::CharacterContactListener*;

        auto DoApplyCharacterPoses_GameThread(const FCk_Handle& InTransientEntity) -> void;

        // Advances every registered character one fixed sub-step (ExtendedUpdate). NO registry/UObject access.
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
        TFunction<void(bool)>                               _SetPersistedContactsWantedFn;

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

        // ---- Persisted-contact interest votes, OR-ed once per drain ----
        TArray<TPair<FName, TFunction<bool()>>> _PersistedContactInterestProviders;

        // Diagnostic only: how many Persisted events this world has consumed since it was created. Never
        // reset, so a reader can diff two samples over a window instead of racing a single drain that may
        // legitimately have seen no sub-step.
        uint64 _Debug_NumPersistedContactEventsTotal = 0;

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
        CK_PROPERTY_GET(_Debug_NumPersistedContactEventsTotal);
    };
}

// --------------------------------------------------------------------------------------------------------------------
