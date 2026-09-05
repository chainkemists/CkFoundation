#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkJolt/CkJolt_ActivationEvent.h"
#include "CkJolt/CkJolt_ContactEvent.h"
#include "CkJolt/Character/CkJoltCharacter_Fragment_Data.h"

#include <Async/Future.h>

#include <CoreMinimal.h>

#include <atomic>

#include <Jolt/Jolt.h>
#include <Jolt/Core/Reference.h>
#include <Jolt/Physics/Character/CharacterBase.h>
#include <Jolt/Physics/Constraints/TwoBodyConstraint.h>

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

namespace ck::jolt
{
    class FCk_Jolt_CollisionLayerTable;
}

namespace ck
{
    class CkJoltCharacterContactListener;
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::jolt
{
    // How many fixed steps a load's convergence phase is granted per frame, and how many must have run before
    // physics counts as converged. Two is the smallest number that can settle a restored contact: the first step
    // produces the contacts and the second is what proves the queue that routed them came back empty.
    inline constexpr int32 kConvergePhysicsSteps = 2;

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

    // ----------------------------------------------------------------------------------------------------------------

    /*
     * What a debugger needs to DRAW an in-flight drag: where the spring is attached on the body (world space,
     * cached by Apply_DragRequests off the body's live transform, so it tracks rotation without the getter ever
     * reading Jolt) and where it is being pulled to. The body is named by its debug-draw KEY, the same one the
     * capture draws it under.
     *
     * _BodyKey has no "no body" value — 0 is a VALID key — so a default-constructed state names body 0. Only a
     * state handed back by Get_DragState is meaningful, and that one is produced exclusively while a drag is live.
     */
    struct CKJOLT_API FCk_Jolt_DebugDragState
    {
    public:
        CK_GENERATED_BODY(FCk_Jolt_DebugDragState);

    private:
        uint64 _BodyKey = 0;
        FVector _GrabPointWorld = FVector::ZeroVector;
        FVector _AnchorPointWorld = FVector::ZeroVector;

    public:
        CK_PROPERTY(_BodyKey);
        CK_PROPERTY(_GrabPointWorld);
        CK_PROPERTY(_AnchorPointWorld);
    };
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
            // Needed only by the debug-drag facility, which registers its own non-colliding anchor layer lazily on
            // the first drag. Absent (headless fixtures, a world built for the step math alone) = no drag.
            ck::jolt::FCk_Jolt_CollisionLayerTable*                LayerTable = nullptr;
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
        // Advances the simulation one fixed sub-step. Jolt-only; task-graph safe. Returns the
        // EPhysicsUpdateError bit mask as a plain integer so the frame pump can trace it without
        // exposing a Jolt enum through this header.
        auto DoPhysicsUpdate(float InFixedDt) -> uint32;
        // Snapshots every active rigid body's pose into _PoseBuffer. NO registry/UObject access.
        auto DoCapturePoses_AnyThread() -> void;
        auto DoApplyPoseBuffer_GameThread(const FCk_Handle& InTransientEntity) -> void;

        // O(1) step census except Get_NumBodies_AnyThread, which takes Jolt's body-list mutex once.
        // Call only from the stable pre-step window or the step thread itself.
        auto Get_NumBodies_AnyThread() const -> int32;
        auto Get_NumActiveRigidBodies_AnyThread() const -> int32;
        auto Get_NumActiveSoftBodies_AnyThread() const -> int32;
        auto Get_NumRegisteredCharacters_AnyThread() const -> int32;

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

        // ---- Static-scene change token (game-thread only) ----
        // Bumped by every funnel that adds or removes a Static-motion or baked-static-world body. Consumers
        // (the debug-draw capture) treat an unchanged value as "the static scene did not move" and skip work.
        auto Request_NoteStaticSceneChanged() -> void;

        // ---- Debug pause / single step (game-thread only) ----
        /*
         * A pause of the JOLT world, independent of UWorld::IsPaused() — the engine's pause is set by the
         * PlayerController and stops the whole game, while this one freezes physics alone so a debugger can
         * inspect it. Both are honoured by the same two step guards.
         */
        auto Request_SetDebugPaused(bool InIsDebugPaused) -> void;

        /*
         * Permits exactly ONE step while debug-paused, after which the world re-pauses itself. Ignored (Verbose)
         * while the world is not debug-paused: stepping once has no meaning when it is already stepping.
         */
        auto Request_StepOnce() -> void;

        /*
         * The gate, CONSUMED once per frame by FProcessor_JoltWorld_PlanStep and nothing else. Answers true while
         * the world must plan zero steps, and eats the step-once one-shot in the process — which is why it lives
         * in PlanStep: a flag the Step processor interpreted instead would bypass the accumulator model entirely.
         * Get_StepOnceGrantedThisFrame is how the Step processor reads the same decision without re-consuming it.
         */
        auto TryConsume_DebugPauseGate() -> bool;

        // ---- Load-convergence step grant (game-thread only) ----
        /*
         * Runs N fixed steps on the next frame that steps, independently of that frame's delta AND of the debug
         * pause. It exists for a snapshot load: a load holds global time dilation at its floor, so the frame delta
         * plans zero steps and a restored world would be handed back with its bodies never settled and its contact
         * queue never routed.
         *
         * Distinct from the debug step-once gate. That one PERMITS a step the pause is refusing, and only while
         * paused; this one COMMANDS N whatever the world's own pacing says. Repeat grants before the next
         * consumption do NOT queue — the pending grant is the MAX, so a driver granting every frame cannot bank a
         * burst against a frame the engine happened to block.
         */
        auto Request_GrantFixedSteps(int32 InNumSteps) -> void;

        /*
         * The grant, CONSUMED once per frame by FProcessor_JoltWorld_PlanStep and nothing else — the same rule the
         * debug gate follows, for the same reason: the plan is what the Step processor executes, so a flag
         * interpreted downstream would bypass the accumulator model. Returns 0 when nothing is granted.
         */
        auto TryConsume_GrantedFixedSteps() -> int32;

        // ---- Step duration (written on the step thread, read anywhere) ----
        auto Set_LastStepDurationMs(float InDurationMs) -> void;
        auto Get_LastStepDurationMs() const -> float;

#if !UE_BUILD_SHIPPING
        /*
         * ---- Debug drag (P6-D47) — DEV-ONLY and SIM-MUTATING, unlike everything else the debug facility does ----
         *
         * A spring drag of one DYNAMIC body, the way a physics sandbox lets you grab and throw. The three requests
         * QUEUE and are applied by FProcessor_JoltDebugDrag_Apply on the game thread, before the step: a mutation
         * issued from a Slate click must not land in the middle of a solve. One drag at a time — a Begin while
         * another is live ends that one first.
         *
         * It mutates the simulation, so it belongs on the authority only: a drag taken on a client moves a body the
         * server will correct on the next replication, which reads as a bug rather than a tool. The whole facility
         * — state, requests, processor and subsystem forwarders — is compiled out of Shipping for the same reason.
         */
        auto Request_BeginDrag(uint64 InBodyKey, const FVector& InWorldGrabPoint) -> void;
        auto Request_UpdateDrag(const FVector& InWorldTargetPoint) -> void;
        auto Request_EndDrag() -> void;

        /*
         * Drains the queued drag requests AND re-reads the live drag. Game thread, BEFORE the step, and nowhere
         * else. It runs unconditionally rather than early-outing on an empty queue: a dragged body can be
         * destroyed with no request behind it, and this is the only place that would notice.
         */
        auto Apply_DragRequests() -> void;

        auto Get_IsDragging() const -> bool;

        /// Cached values only — the getter never reads a JPH body, so a Slate consumer may call it at any time.
        auto Get_DragState() const -> TOptional<ck::jolt::FCk_Jolt_DebugDragState>;
#endif

        /*
         * Debug-draw KEYS of the bodies this world created for its own internal use — today exactly the drag anchor,
         * which is a raw JPH kinematic body with NO entity behind it. Every consumer surface (the capture's four
         * technique steps, TryPick_Body, the outliner) must skip them: an anchor a user can see, click or select is
         * a bug, not a cosmetic issue. Empty whenever nothing is dragging.
         */
        auto Get_DebugInternalBodyKeys() const -> const TSet<uint64>&;

        /*
         * ---- Contact-pair counter (P6-D48) ----
         * Bumped from Jolt's contact callbacks, which run on WORKER threads during the solve, and RESET at the top
         * of every DoPhysicsUpdate — so a game-thread read after the frame's steps reports the LAST sub-step's pair
         * count, not an unbounded accumulation. Relaxed: a lone diagnostic scalar that orders nothing.
         */
        auto Note_ContactPair() -> void;
        auto Get_ContactPairsLastStep() const -> int32;

#if !UE_BUILD_SHIPPING
        /*
         * Sensor bodies that currently have one or more active Jolt contacts, named in the debug-draw keyspace.
         * Maintained while the game thread drains Added/Removed events only; Persisted is deliberately excluded so
         * repeated manifolds cannot inflate an active-contact count. No JPH objects escape through this API.
         */
        auto Get_SensorContactBodyKeys() const -> const TSet<uint64>&;
#endif

        // ---- Body-removed change token (game-thread only) ----
        // Bumped by the JoltBody EndPlay funnel for EVERY body it destroys, whatever its motion type. The
        // debug-draw capture's sweep for destroyed SLEEPING bodies is O(sleeping) and can only be skipped
        // safely while this value has not moved — a sleeping body is invisible to both of its body passes.
        auto Request_NoteBodyRemoved() -> void;

        // Jolt operations that need scratch memory (HeightFieldShape::SetHeights) borrow the world's
        // allocator rather than standing up their own. Null once the world has shut down.
        auto Get_TempAllocator() const -> JPH::TempAllocatorImpl*;

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
        uint64 _StaticSceneRevision = 0;
        uint64 _BodyRemovedRevision = 0;
        TFuture<void> _AsyncFuture;

        // ---- Debug pause state (game thread only) ----
        bool _IsDebugPaused = false;
        bool _StepOnceRequested = false;
        bool _StepOnceGrantedThisFrame = false;

        // ---- Load-convergence grant state (game thread only) ----
        int32 _GrantedFixedSteps = 0;         // pending, waiting for the next frame that plans
        int32 _GrantedStepsThisFrame = 0;     // consumed by PlanStep this frame; READ by Step
        // Monotonic, never reset: a convergence predicate diffs it against a baseline sampled when the phase
        // began, so it needs no notion of which load is running.
        int32 _GrantedStepsExecutedTotal = 0;

        // ---- Contact-drain observability (game thread only) ----
        // How many routed drains have run, and how many events the most recent one carried. A settled world drains
        // nothing; a world that has never drained has not established anything either way.
        uint64 _NumContactDrains = 0;
        int32  _LastDrainedContactEventCount = 0;

        // Written by the step loop, which runs on a TASK GRAPH thread in async mode, and read by the game thread
        // whenever a consumer asks. Relaxed: it is a lone diagnostic scalar that orders nothing.
        std::atomic<float> _LastStepDurationMs{0.0f};

#if !UE_BUILD_SHIPPING
        // ---- Debug drag (game thread only; the anchor body and constraint are the solve's after that) ----
        struct FDragRequest
        {
            enum class EType : uint8
            {
                Begin,
                Update,
                End
            };

            EType _Type = EType::End;
            uint64 _BodyKey = 0;
            FVector _Point = FVector::ZeroVector;
        };

        TArray<FDragRequest> _DragRequests;

        JPH::Ref<JPH::TwoBodyConstraint> _DragConstraint;

        // BodyID index+sequence numbers. A BodyID of 0 is VALID, so _IsDragging is the presence test — never a
        // sentinel id.
        uint32 _DragAnchorBodyId = 0;
        uint32 _DraggedBodyId = 0;

        // Body-LOCAL, so the grab point tracks the body as it rotates; Jolt stores the same thing internally, but
        // it is not readable back off a DistanceConstraint. Its world projection is refreshed once per
        // Apply_DragRequests so Get_DragState answers from a cache instead of the live body.
        FVector _DragGrabPointLocal = FVector::ZeroVector;
        FVector _DragGrabPointWorld = FVector::ZeroVector;
        FVector _DragAnchorPointWorld = FVector::ZeroVector;

        bool _IsDragging = false;

        // Registered LAZILY on the first drag, never at world init: a world that is never dragged must not spend a
        // layer out of the table's fixed 1024 (P5-D61/S2).
        uint16 _DragAnchorLayer = 0;
        bool _DragAnchorLayerRegistered = false;
#endif

        ck::jolt::FCk_Jolt_CollisionLayerTable* _LayerTable = nullptr;

        TSet<uint64> _DebugInternalBodyKeys;

        // Written by Jolt's contact callbacks on worker threads, reset by the step thread, read by the game thread.
        std::atomic<int32> _ContactPairsThisStep{0};

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

#if !UE_BUILD_SHIPPING
        // Game-thread contact lifetime accounting for the sensor-contact debug overlay. The count is per SENSOR
        // BodyID index+sequence because one sensor may overlap several bodies; the public set mirrors exactly the
        // entries whose count is positive.
        TMap<uint32, int32> _SensorBodyContactCounts;
        TSet<uint64> _SensorContactBodyKeys;
#endif

    private:
#if !UE_BUILD_SHIPPING
        auto Apply_SensorContactEvent(const FCk_Jolt_ContactEvent& InEvent) -> void;
#endif
        auto Find_CharacterEntry(uint64 InUserData) -> FCk_Jolt_CharacterEntry*;

#if !UE_BUILD_SHIPPING
        auto DoBegin_Drag(uint64 InBodyKey, const FVector& InWorldGrabPoint) -> void;
        auto DoUpdate_Drag(const FVector& InWorldTargetPoint) -> void;

        /// Idempotent, and the ONLY teardown path: Shutdown and the destructor both funnel through it so an
        /// orphaned anchor body or constraint is impossible.
        auto DoEnd_Drag() -> void;

        /// Ends a live drag whose body no longer exists, so the constraint can never outlive one of its ends.
        auto DoEnd_DragIfBodyIsGone() -> void;

        /// Re-projects the body-local grab point into world space. Called once per Apply_DragRequests, in the same
        /// game-thread window everything else here mutates Jolt from.
        auto DoRefresh_DragGrabPoint() -> void;
#endif

    public:
        CK_PROPERTY_GET(_World);
        CK_PROPERTY_GET(_AsyncMode);
        CK_PROPERTY(_Accumulator);
        CK_PROPERTY(_Alpha);
        CK_PROPERTY(_NumStepsLastFrame);
        CK_PROPERTY(_PendingSimTime);
        CK_PROPERTY_GET(_OptimizeBroadPhaseRequested);
        CK_PROPERTY_GET(_StaticSceneRevision);
        CK_PROPERTY_GET(_BodyRemovedRevision);
        CK_PROPERTY_GET(_AsyncFuture);
        CK_PROPERTY_GET(_Debug_NumPersistedContactEventsTotal);
        CK_PROPERTY_GET(_IsDebugPaused);
        CK_PROPERTY_GET(_StepOnceGrantedThisFrame);
        CK_PROPERTY(_GrantedStepsThisFrame);
        CK_PROPERTY_GET(_GrantedStepsExecutedTotal);
        CK_PROPERTY_GET(_NumContactDrains);
        CK_PROPERTY_GET(_LastDrainedContactEventCount);
    };
}

// --------------------------------------------------------------------------------------------------------------------
