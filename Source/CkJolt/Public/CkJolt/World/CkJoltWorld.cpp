#include "CkJoltWorld.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Payload/CkPayload.h"

#include "CkEcs/Entity/CkEntity.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Registry/CkRegistry.h"

#include "CkJolt/Body/CkJoltBody_Fragment.h"
#include "CkJolt/Character/CkJoltCharacter_Fragment.h"
#include "CkJolt/Character/CkJoltCharacter_Utils.h"
#include "CkJolt/Character/CkJoltCharacterContactListener.h"
#include "CkJolt/CkJolt_Log.h"
#include "CkJolt/CkJolt_Utils.h"
#include "CkJolt/CollisionLayers/CkJoltCollisionLayerTable.h"
#include "CkJolt/Subsystem/CkJolt_DebugDrawTarget.h"

#include <Jolt/Jolt.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyFilter.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Body/BodyLockInterface.h>
#include <Jolt/Physics/Body/BodyType.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/ShapeFilter.h>
#include <Jolt/Physics/Constraints/DistanceConstraint.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_jolt_world
{
#if !UE_BUILD_SHIPPING
    // The drag spring, per P6-D47: soft limits at zero separation, so ANY distance between the grab point and the
    // anchor is over the limit and the spring is what closes it. 2 Hz / critical damping is a hand that follows the
    // cursor without whipping the body past it.
    constexpr auto DragSpringFrequencyHz = 2.0f;
    constexpr auto DragSpringDamping = 1.0f;

    // The anchor draws nothing and collides with nothing; it exists to be a constraint end-point, so its shape is
    // as close to a point as Jolt will accept.
    constexpr auto DragAnchorRadius = 1.0f;

    // A debug-draw key wider than a BodyID's index+sequence names a CHARACTER or an overlay, not a rigid body.
    constexpr auto DragBodyKeyMask = uint64{0xFFFFFFFF};

    /*
     * The anchor's object layer (RATIFIED, P5-D61/S2): a DEFAULT-CONSTRUCTED signature has an all-zero response
     * mask, so its pair interaction against every other layer is Ignore and Jolt's pair filter refuses every pair —
     * and a channel query, which reads the same mask, cannot see it either. The Dynamic domain is what keeps
     * ObjectVsBroadPhaseLayerFilter from culling it out of the dynamic tree, where the dragged body lives.
     */
    auto
        Get_DragAnchorSignature()
        -> FCk_Jolt_CollisionSignature
    {
        auto Signature = FCk_Jolt_CollisionSignature{};
        Signature.Set_Domain(ECk_Jolt_BodyDomain::Dynamic);
        return Signature;
    }
#endif
}

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
        , _SetPersistedContactsWantedFn(MoveTemp(InParams.SetPersistedContactsWantedFn))
        , _LayerTable(InParams.LayerTable)
    {
        _CharacterContactListener = MakeUnique<CkJoltCharacterContactListener>(this);
    }

    FJoltWorld::
        ~FJoltWorld()
    {
#if !UE_BUILD_SHIPPING
        // Belt to Shutdown's braces again: a world destroyed mid-drag would otherwise leave an anchor body and a
        // constraint in a PhysicsSystem that outlives it.
        DoEnd_Drag();
#endif

#if JPH_DEBUG_RENDERER
        // Belt to Shutdown's braces: a world torn down without Shutdown would leave its contact buffers keyed by
        // an address a later PhysicsSystem can be allocated at, and inherit a dead world's contacts once.
        if (const auto PhysicsSystem = _PhysicsSystem.Pin(); PhysicsSystem.IsValid())
        { ck::jolt::debug_draw::Forget_ContactRecord(PhysicsSystem.Get()); }
#endif
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

#if !UE_BUILD_SHIPPING
        // BEFORE the Jolt pointers are nulled below — the teardown needs the PhysicsSystem it created the anchor
        // and the constraint in.
        _DragRequests.Reset();
        DoEnd_Drag();
#endif

#if JPH_DEBUG_RENDERER
        if (const auto PhysicsSystem = _PhysicsSystem.Pin(); PhysicsSystem.IsValid())
        { ck::jolt::debug_draw::Forget_ContactRecord(PhysicsSystem.Get()); }
#endif

        _PhysicsSystem = nullptr;
        _TempAllocator = nullptr;
        _JobSystem = nullptr;
        _LayerTable = nullptr;
        _DrainQueueFn = {};
        _DrainActivationQueueFn = {};
        _SetPersistedContactsWantedFn = {};
        _World = nullptr;
        _ContactRouters.Empty();
        _PersistedContactInterestProviders.Empty();
#if !UE_BUILD_SHIPPING
        _SensorBodyContactCounts.Empty();
        _SensorContactBodyKeys.Empty();
#endif
        _PoseBuffer.Empty();
        _CharacterRegistry.Empty();
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
        Register_PersistedContactInterestProvider(
            FName InName,
            TFunction<bool()> InProvider)
        -> void
    {
        const auto ExistingIndex = _PersistedContactInterestProviders.IndexOfByPredicate([&](const auto& InPair)
        {
            return InPair.Key == InName;
        });

        CK_ENSURE_IF_NOT(ExistingIndex == INDEX_NONE,
            TEXT("A persisted-contact interest provider named [{}] is already registered on the Jolt world."), InName)
        { return; }

        _PersistedContactInterestProviders.Emplace(InName, MoveTemp(InProvider));
    }

    auto
        FJoltWorld::
        Unregister_PersistedContactInterestProvider(
            FName InName)
        -> void
    {
        _PersistedContactInterestProviders.RemoveAll([&](const auto& InPair)
        {
            return InPair.Key == InName;
        });
    }

    auto
        FJoltWorld::
        DrainEventsAndRoute()
        -> void
    {
        // Unconditional and BEFORE every early-out below: the flag would otherwise go stale exactly
        // when the queue is empty. This runs on the game thread, ordered before this frame's
        // PlanStep/Step (RunAfter chain), so the value is committed before the workers are kicked.
        if (_SetPersistedContactsWantedFn)
        {
            auto Wanted = false;
            for (const auto& Provider : _PersistedContactInterestProviders)
            {
                if (Provider.Value && Provider.Value())
                {
                    Wanted = true;
                    break;
                }
            }

            _SetPersistedContactsWantedFn(Wanted);
        }

        if (NOT _DrainQueueFn)
        { return; }

        auto Events = TArray<FCk_Jolt_ContactEvent>{};
        _DrainQueueFn(Events);

        // Recorded BEFORE the empty early-out below, because an empty drain is the observation that matters: a
        // world whose last drain carried nothing has no contacts still working their way out of the solve.
        ++_NumContactDrains;
        _LastDrainedContactEventCount = Events.Num();

        for (const auto& Event : Events)
        {
            if (Event.Type == FCk_Jolt_ContactEvent::EType::Persisted)
            { ++_Debug_NumPersistedContactEventsTotal; }

#if !UE_BUILD_SHIPPING
            Apply_SensorContactEvent(Event);
#endif
        }

        if (Events.IsEmpty())
        { return; }

        // Routers run user code that may Register/Unregister — iterate a copy so a mid-broadcast mutation cannot dangle.
        const auto RoutersCopy = _ContactRouters;
        for (const auto& Router : RoutersCopy)
        { Router.Value(Events); }
    }

#if !UE_BUILD_SHIPPING
    auto
        FJoltWorld::
        Apply_SensorContactEvent(
            const FCk_Jolt_ContactEvent& InEvent)
        -> void
    {
        if (InEvent.Type == FCk_Jolt_ContactEvent::EType::Persisted)
        { return; }

        const auto ApplyForSide = [this, &InEvent](uint32 InIndexAndSequence, bool InIsSensor) -> void
        {
            if (NOT InIsSensor)
            { return; }

            const auto BodyKey = ck::jolt::debug_draw::Make_BodyKey(InIndexAndSequence);

            if (InEvent.Type == FCk_Jolt_ContactEvent::EType::Added)
            {
                ++_SensorBodyContactCounts.FindOrAdd(InIndexAndSequence);
                _SensorContactBodyKeys.Add(BodyKey);
                return;
            }

            auto* Count = _SensorBodyContactCounts.Find(InIndexAndSequence);
            if (Count == nullptr)
            { return; }

            --*Count;

            if (*Count > 0)
            { return; }

            _SensorBodyContactCounts.Remove(InIndexAndSequence);
            _SensorContactBodyKeys.Remove(BodyKey);
        };

        ApplyForSide(InEvent.Body1IndexAndSeq, InEvent.IsSensor1);
        ApplyForSide(InEvent.Body2IndexAndSeq, InEvent.IsSensor2);
    }
#endif

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
        Request_NoteStaticSceneChanged()
        -> void
    {
        ++_StaticSceneRevision;
    }

    auto
        FJoltWorld::
        Request_NoteBodyRemoved()
        -> void
    {
        ++_BodyRemovedRevision;
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
        Request_SetDebugPaused(
            bool InIsDebugPaused)
        -> void
    {
        _IsDebugPaused = InIsDebugPaused;

        // A one-shot armed during a pause must not survive the resume and fire at the START of the next one —
        // the user asked for a step in a session that has ended.
        if (NOT InIsDebugPaused)
        { _StepOnceRequested = false; }
    }

    auto
        FJoltWorld::
        Request_StepOnce()
        -> void
    {
        if (NOT _IsDebugPaused)
        {
            ck::jolt::Verbose(TEXT("Ignoring Jolt Request_StepOnce: the world is not debug-paused"));
            return;
        }

        _StepOnceRequested = true;
    }

    auto
        FJoltWorld::
        TryConsume_DebugPauseGate()
        -> bool
    {
        _StepOnceGrantedThisFrame = false;

        if (NOT _IsDebugPaused)
        { return false; }

        if (NOT _StepOnceRequested)
        { return true; }

        _StepOnceRequested = false;
        _StepOnceGrantedThisFrame = true;

        return false;
    }

    auto
        FJoltWorld::
        Request_GrantFixedSteps(
            int32 InNumSteps)
        -> void
    {
        if (InNumSteps <= 0)
        {
            ck::jolt::Verbose(TEXT("Ignoring Jolt Request_GrantFixedSteps: [{}] is not a number of steps to run"),
                InNumSteps);
            return;
        }

        // MAX, not +=. A driver that grants every frame while the engine is blocking the world would otherwise
        // bank a burst and spend it all on the first frame that runs, which is the spiral the fixed-step pump's
        // own clamp exists to prevent.
        _GrantedFixedSteps = FMath::Max(_GrantedFixedSteps, InNumSteps);
    }

    auto
        FJoltWorld::
        TryConsume_GrantedFixedSteps()
        -> int32
    {
        _GrantedStepsThisFrame = _GrantedFixedSteps;
        _GrantedFixedSteps = 0;

        _GrantedStepsExecutedTotal += _GrantedStepsThisFrame;

        return _GrantedStepsThisFrame;
    }

    auto
        FJoltWorld::
        Set_LastStepDurationMs(
            float InDurationMs)
        -> void
    {
        _LastStepDurationMs.store(InDurationMs, std::memory_order_relaxed);
    }

    auto
        FJoltWorld::
        Get_LastStepDurationMs() const
        -> float
    {
        return _LastStepDurationMs.load(std::memory_order_relaxed);
    }

    // --------------------------------------------------------------------------------------------------------------------

#if !UE_BUILD_SHIPPING

    auto
        FJoltWorld::
        Request_BeginDrag(
            uint64 InBodyKey,
            const FVector& InWorldGrabPoint)
        -> void
    {
        // A key carrying any bit above the BodyID's 32 is a character or an overlay key. Truncating it to a
        // BodyID would grab an unrelated rigid body, which is worse than refusing.
        if ((InBodyKey & ~ck_jolt_world::DragBodyKeyMask) != 0)
        {
            ck::jolt::Verbose(TEXT("Ignoring Jolt Request_BeginDrag: body key [{}] is not a rigid-body key"),
                InBodyKey);
            return;
        }

        _DragRequests.Emplace(FDragRequest{FDragRequest::EType::Begin, InBodyKey, InWorldGrabPoint});
    }

    auto
        FJoltWorld::
        Request_UpdateDrag(
            const FVector& InWorldTargetPoint)
        -> void
    {
        _DragRequests.Emplace(FDragRequest{FDragRequest::EType::Update, 0, InWorldTargetPoint});
    }

    auto
        FJoltWorld::
        Request_EndDrag()
        -> void
    {
        _DragRequests.Emplace(FDragRequest{FDragRequest::EType::End, 0, FVector::ZeroVector});
    }

    auto
        FJoltWorld::
        Apply_DragRequests()
        -> void
    {
        // Runs whether or not anything was queued: the dragged body can be destroyed under the drag (its entity
        // dies, a level streams out) with no request behind it, and this is the only game-thread window that
        // would notice. The constraint has to go BEFORE the body it references does.
        DoEnd_DragIfBodyIsGone();

        // Drained by MOVE rather than iterated in place: a handler may not enqueue, but the array must be empty
        // before the next frame whatever a handler does.
        const auto Requests = MoveTemp(_DragRequests);
        _DragRequests.Reset();

        for (const auto& Request : Requests)
        {
            switch (Request._Type)
            {
                case FDragRequest::EType::Begin:
                { DoBegin_Drag(Request._BodyKey, Request._Point); break; }

                case FDragRequest::EType::Update:
                { DoUpdate_Drag(Request._Point); break; }

                case FDragRequest::EType::End:
                default:
                { DoEnd_Drag(); break; }
            }
        }

        DoRefresh_DragGrabPoint();
    }

    auto
        FJoltWorld::
        DoEnd_DragIfBodyIsGone()
        -> void
    {
        if (NOT _IsDragging)
        { return; }

        const auto PhysicsSystem = _PhysicsSystem.Pin();

        if (NOT PhysicsSystem.IsValid())
        {
            DoEnd_Drag();
            return;
        }

        if (PhysicsSystem->GetBodyLockInterfaceNoLock().TryGetBody(JPH::BodyID{_DraggedBodyId}) == nullptr)
        {
            ck::jolt::Verbose(TEXT("Ending the Jolt debug drag: the dragged body was destroyed under it"));
            DoEnd_Drag();
        }
    }

    auto
        FJoltWorld::
        DoRefresh_DragGrabPoint()
        -> void
    {
        if (NOT _IsDragging)
        { return; }

        const auto PhysicsSystem = _PhysicsSystem.Pin();
        if (NOT PhysicsSystem.IsValid())
        { return; }

        const auto* Body = PhysicsSystem->GetBodyLockInterfaceNoLock().TryGetBody(JPH::BodyID{_DraggedBodyId});
        if (Body == nullptr)
        { return; }

        _DragGrabPointWorld = ck::jolt::Conv(Body->GetWorldTransform() * ck::jolt::Conv(_DragGrabPointLocal));
    }

    auto
        FJoltWorld::
        DoBegin_Drag(
            uint64 InBodyKey,
            const FVector& InWorldGrabPoint)
        -> void
    {
        // One drag at a time: a second Begin is a new grab, not a second spring on the same hand.
        DoEnd_Drag();

        const auto PhysicsSystem = _PhysicsSystem.Pin();
        if (NOT PhysicsSystem.IsValid())
        { return; }

        const auto LayerTableIsAvailable = _LayerTable != nullptr;

        CK_ENSURE_IF_NOT(LayerTableIsAvailable,
            TEXT("Cannot begin a Jolt debug drag: this world was built without a collision-layer table, so its "
                 "anchor body has no non-colliding layer to live on"))
        { return; }

        const auto BodyId = JPH::BodyID{static_cast<JPH::uint32>(InBodyKey)};

        // NoLock mirrors every other read this module makes outside the solve: Apply runs on the game thread,
        // before the step is kicked and after the previous one was consumed, so no worker is mutating bodies.
        auto* Body = PhysicsSystem->GetBodyLockInterfaceNoLock().TryGetBody(BodyId);

        if (Body == nullptr)
        {
            ck::jolt::Verbose(TEXT("Ignoring Jolt debug drag: body key [{}] names no live body"), InBodyKey);
            return;
        }

        // Dynamic ONLY. A static or kinematic body is driven by something else — the level, or the ECS transform —
        // and a spring on it either does nothing or fights the writer that owns it.
        if (NOT Body->IsDynamic())
        {
            ck::jolt::Verbose(TEXT("Ignoring Jolt debug drag: body key [{}] is not a DYNAMIC body"), InBodyKey);
            return;
        }

        if (NOT _DragAnchorLayerRegistered)
        {
            const auto Layer = _LayerTable->Get_OrRegisterLayer(ck_jolt_world::Get_DragAnchorSignature());

            // Already ensured by the table itself on exhaustion; the drag simply does not happen rather than
            // spawning an anchor on a layer that collides with the world.
            if (Layer == JPH::cObjectLayerInvalid)
            { return; }

            _DragAnchorLayer = Layer;
            _DragAnchorLayerRegistered = true;
        }

        const auto GrabPoint = JPH::RVec3{ck::jolt::Conv(InWorldGrabPoint)};

        auto AnchorShape = JPH::Ref<JPH::Shape>{new JPH::SphereShape{ck_jolt_world::DragAnchorRadius}};

        auto AnchorSettings = JPH::BodyCreationSettings{
            AnchorShape.GetPtr(),
            GrabPoint,
            JPH::Quat::sIdentity(),
            JPH::EMotionType::Kinematic,
            JPH::ObjectLayer{_DragAnchorLayer}};

        auto& BodyInterface = PhysicsSystem->GetBodyInterface();
        const auto AnchorId = BodyInterface.CreateAndAddBody(AnchorSettings, JPH::EActivation::Activate);

        if (AnchorId.IsInvalid())
        {
            ck::jolt::Verbose(TEXT("Ignoring Jolt debug drag: the anchor body could not be created "
                                   "(body budget exhausted?)"));
            return;
        }

        auto* Anchor = PhysicsSystem->GetBodyLockInterfaceNoLock().TryGetBody(AnchorId);

        if (Anchor == nullptr)
        {
            BodyInterface.RemoveBody(AnchorId);
            BodyInterface.DestroyBody(AnchorId);
            return;
        }

        auto ConstraintSettings = JPH::DistanceConstraintSettings{};
        ConstraintSettings.mSpace = JPH::EConstraintSpace::WorldSpace;
        ConstraintSettings.mPoint1 = GrabPoint;
        ConstraintSettings.mPoint2 = GrabPoint;
        ConstraintSettings.mMinDistance = 0.0f;
        ConstraintSettings.mMaxDistance = 0.0f;
        ConstraintSettings.mLimitsSpringSettings = JPH::SpringSettings{
            JPH::ESpringMode::FrequencyAndDamping,
            ck_jolt_world::DragSpringFrequencyHz,
            ck_jolt_world::DragSpringDamping};

        _DragConstraint = ConstraintSettings.Create(*Body, *Anchor);
        PhysicsSystem->AddConstraint(_DragConstraint.GetPtr());

        _DraggedBodyId = BodyId.GetIndexAndSequenceNumber();
        _DragAnchorBodyId = AnchorId.GetIndexAndSequenceNumber();
        _DragGrabPointLocal = ck::jolt::Conv(
            Body->GetWorldTransform().InversedRotationTranslation() * JPH::Vec3{GrabPoint});
        _DragGrabPointWorld = InWorldGrabPoint;
        _DragAnchorPointWorld = InWorldGrabPoint;
        _IsDragging = true;

        _DebugInternalBodyKeys.Emplace(ck::jolt::debug_draw::Make_BodyKey(_DragAnchorBodyId));

        // A body that fell asleep before the grab would otherwise ignore the spring entirely.
        BodyInterface.ActivateBody(BodyId);
    }

    auto
        FJoltWorld::
        DoUpdate_Drag(
            const FVector& InWorldTargetPoint)
        -> void
    {
        if (NOT _IsDragging)
        {
            ck::jolt::Verbose(TEXT("Ignoring Jolt debug drag update: nothing is being dragged"));
            return;
        }

        const auto PhysicsSystem = _PhysicsSystem.Pin();
        if (NOT PhysicsSystem.IsValid())
        { return; }

        auto& BodyInterface = PhysicsSystem->GetBodyInterface();

        // SetPosition rather than MoveKinematic: the anchor carries no momentum of its own — the spring is what
        // produces the force, and a kinematic velocity would add a second one nobody asked for.
        BodyInterface.SetPosition(JPH::BodyID{_DragAnchorBodyId},
            JPH::RVec3{ck::jolt::Conv(InWorldTargetPoint)}, JPH::EActivation::Activate);

        // Every update, not just the first: a body dragged slowly enough can settle back to sleep mid-drag and
        // would then hang off a spring it no longer responds to.
        BodyInterface.ActivateBody(JPH::BodyID{_DraggedBodyId});

        _DragAnchorPointWorld = InWorldTargetPoint;
    }

    auto
        FJoltWorld::
        DoEnd_Drag()
        -> void
    {
        if (NOT _IsDragging)
        { return; }

        if (const auto PhysicsSystem = _PhysicsSystem.Pin(); PhysicsSystem.IsValid())
        {
            if (_DragConstraint.GetPtr() != nullptr)
            { PhysicsSystem->RemoveConstraint(_DragConstraint.GetPtr()); }

            auto& BodyInterface = PhysicsSystem->GetBodyInterface();
            const auto AnchorId = JPH::BodyID{_DragAnchorBodyId};

            if (BodyInterface.IsAdded(AnchorId))
            { BodyInterface.RemoveBody(AnchorId); }

            BodyInterface.DestroyBody(AnchorId);
        }

        // AFTER RemoveConstraint: the manager holds the only other reference, and dropping ours first would free a
        // constraint it still has registered.
        _DragConstraint = nullptr;

        _DebugInternalBodyKeys.Reset();
        _DragAnchorBodyId = 0;
        _DraggedBodyId = 0;
        _DragGrabPointLocal = FVector::ZeroVector;
        _DragGrabPointWorld = FVector::ZeroVector;
        _DragAnchorPointWorld = FVector::ZeroVector;
        _IsDragging = false;
    }

    auto
        FJoltWorld::
        Get_IsDragging() const
        -> bool
    {
        return _IsDragging;
    }

    auto
        FJoltWorld::
        Get_DragState() const
        -> TOptional<ck::jolt::FCk_Jolt_DebugDragState>
    {
        if (NOT _IsDragging)
        { return {}; }

        // Cached values ONLY. A consumer of this is a Slate tick, and reading a JPH body from there races the
        // step every other part of this facility is careful not to.
        auto State = ck::jolt::FCk_Jolt_DebugDragState{};
        State.Set_BodyKey(ck::jolt::debug_draw::Make_BodyKey(_DraggedBodyId));
        State.Set_GrabPointWorld(_DragGrabPointWorld);
        State.Set_AnchorPointWorld(_DragAnchorPointWorld);

        return State;
    }

#endif

    auto
        FJoltWorld::
        Get_DebugInternalBodyKeys() const
        -> const TSet<uint64>&
    {
        return _DebugInternalBodyKeys;
    }

    auto
        FJoltWorld::
        Note_ContactPair()
        -> void
    {
        _ContactPairsThisStep.fetch_add(1, std::memory_order_relaxed);
    }

    auto
        FJoltWorld::
        Get_ContactPairsLastStep() const
        -> int32
    {
        return _ContactPairsThisStep.load(std::memory_order_relaxed);
    }

#if !UE_BUILD_SHIPPING
    auto
        FJoltWorld::
        Get_SensorContactBodyKeys() const
        -> const TSet<uint64>&
    {
        return _SensorContactBodyKeys;
    }
#endif

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

        // Per STEP, not per frame: a frame that runs four sub-steps would otherwise report four steps' worth of
        // pairs as one step's, which is exactly the number a stats panel would misread as a spike.
        _ContactPairsThisStep.store(0, std::memory_order_relaxed);

#if JPH_DEBUG_RENDERER
        // Contacts exist ONLY inside the solve — there is nothing left to read once Update returns — so this is
        // the one draw the capture cannot do for itself. The scope is a no-op unless some debug-draw target asks
        // for contacts; when one does, Jolt's own contact draw emits lines through the singleton renderer, which
        // has no bound target here and appends them to a lock-guarded frame buffer instead.
        //
        // This runs on the task-graph thread in async mode and Jolt's solve is itself multi-threaded, which is
        // exactly why the buffer is guarded and why the REPLAY happens elsewhere, on the game thread.
        // Keyed by THIS world: a PIE session runs several, and one world's manifolds replayed into another's
        // targets would be a lie. Only one world may hold the scope per step (P5-D64/F3).
        ck::jolt::debug_draw::Begin_ContactRecord(PhysicsSystem.Get());
#endif

        PhysicsSystem->Update(InFixedDt, _CollisionSteps, _TempAllocator, _JobSystem);

#if JPH_DEBUG_RENDERER
        ck::jolt::debug_draw::End_ContactRecord(PhysicsSystem.Get());
#endif
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

            // Body UserData is a raw (versioned) entity id, and a snapshot load can leave one dead in the
            // fresh registry — non-ensuring liveness check first.
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

            // An entity may own MORE Jolt bodies than its JoltBody (e.g. a Probe), all sharing the entity id
            // as UserData. Only the JoltBody's own body may write the entity's StepPose.
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
        Find_CharacterEntry(
            uint64 InUserData)
        -> FCk_Jolt_CharacterEntry*
    {
        return _CharacterRegistry.FindByPredicate([&](const FCk_Jolt_CharacterEntry& InEntry)
        {
            return InEntry.UserData == InUserData;
        });
    }

    auto
        FJoltWorld::
        Register_Character(
            const FCk_Jolt_CharacterEntry& InEntry)
        -> void
    {
        _CharacterRegistry.Emplace(InEntry);
    }

    auto
        FJoltWorld::
        Unregister_Character(
            uint64 InUserData)
        -> void
    {
        _CharacterRegistry.RemoveAll([&](const FCk_Jolt_CharacterEntry& InEntry)
        {
            return InEntry.UserData == InUserData;
        });
    }

    auto
        FJoltWorld::
        Push_CharacterIntent(
            uint64 InUserData,
            const FVector& InMoveVelocity,
            ECk_JoltCharacter_PushPolicy InPushPolicy,
            bool InArmJump,
            float InJumpVelocity)
        -> void
    {
        auto* Entry = Find_CharacterEntry(InUserData);
        if (Entry == nullptr)
        { return; }

        Entry->MoveVelocity = InMoveVelocity;
        Entry->PushPolicy = InPushPolicy;

        // A jump is one-shot: only ARM here (the step loop consumes it). Never clear HasJump on a no-arm
        // frame, else a jump landing on a zero-step frame would be dropped before the next step consumes it.
        if (InArmJump)
        {
            Entry->HasJump = true;
            Entry->JumpVelocity = InJumpVelocity;
        }
    }

    auto
        FJoltWorld::
        Snap_CharacterOutPose(
            uint64 InUserData,
            const FVector& InLocation,
            const FQuat& InRotation)
        -> void
    {
        auto* Entry = Find_CharacterEntry(InUserData);
        if (Entry == nullptr)
        { return; }

        Entry->OutLocation = InLocation;
        Entry->OutRotation = InRotation;
        Entry->DirtyThisFrame = false;
    }

    auto
        FJoltWorld::
        Get_CharacterPushPolicy(
            const JPH::CharacterVirtual* InCharacter) const
        -> ECk_JoltCharacter_PushPolicy
    {
        const auto* Entry = _CharacterRegistry.FindByPredicate([&](const FCk_Jolt_CharacterEntry& InEntry)
        {
            return InEntry.Character == InCharacter;
        });

        return Entry != nullptr ? Entry->PushPolicy : ECk_JoltCharacter_PushPolicy::PushAndBePushed;
    }

    auto
        FJoltWorld::
        Get_CharacterContactListener() const
        -> JPH::CharacterContactListener*
    {
        return _CharacterContactListener.Get();
    }

    auto
        FJoltWorld::
        DoStepCharacters_AnyThread(
            float InFixedDt)
        -> void
    {
        if (_CharacterRegistry.IsEmpty())
        { return; }

        const auto PhysicsSystem = _PhysicsSystem.Pin();
        if (NOT PhysicsSystem.IsValid() || _TempAllocator == nullptr)
        { return; }

        const auto Gravity = PhysicsSystem->GetGravity();

        // Jolt's ExtendedUpdateSettings defaults are Y-up METRES (CharacterVirtual.h); this world is Z-up
        // CENTIMETRES, so each length below is the converted default. Remaining scalars keep theirs.
        constexpr auto StickToFloorStepDownCm      = -50.0f;   // Jolt default { 0, -0.5, 0 } m Y-up
        constexpr auto WalkStairsStepUpCm          =  40.0f;   // Jolt default { 0,  0.4, 0 } m Y-up
        constexpr auto WalkStairsMinStepForwardCm  =   2.0f;   // Jolt default 0.02 m
        constexpr auto WalkStairsStepForwardTestCm =  15.0f;   // Jolt default 0.15 m

        auto ExtendedSettings = JPH::CharacterVirtual::ExtendedUpdateSettings{};
        ExtendedSettings.mStickToFloorStepDown     = JPH::Vec3(0.0f, 0.0f, StickToFloorStepDownCm);
        ExtendedSettings.mWalkStairsStepUp         = JPH::Vec3(0.0f, 0.0f, WalkStairsStepUpCm);
        ExtendedSettings.mWalkStairsMinStepForward = WalkStairsMinStepForwardCm;
        ExtendedSettings.mWalkStairsStepForwardTest = WalkStairsStepForwardTestCm;

        for (auto& Entry : _CharacterRegistry)
        {
            auto* Character = Entry.Character;
            if (Character == nullptr)
            { continue; }

            auto Velocity = Character->GetLinearVelocity();
            const auto GroundState = Character->GetGroundState();

            if (GroundState == JPH::CharacterBase::EGroundState::OnGround)
            {
                // Grounded: desired horizontal input + the ground's own velocity (moving platforms), no fall.
                Velocity = ck::jolt::Conv(Entry.MoveVelocity) + Character->GetGroundVelocity();
            }
            else
            {
                Velocity = JPH::Vec3(Velocity.GetX(), Velocity.GetY(), Velocity.GetZ() + Gravity.GetZ() * InFixedDt);
            }

            if (Entry.HasJump && Character->IsSupported())
            {
                // JumpVelocity is uu/s (+Z); Conv is a Z-up passthrough so it adds straight onto the Z axis.
                Velocity = JPH::Vec3(Velocity.GetX(), Velocity.GetY(), Velocity.GetZ() + Entry.JumpVelocity);
                Entry.HasJump = false;
            }

            Character->SetLinearVelocity(Velocity);

            const auto BroadPhaseFilter = PhysicsSystem->GetDefaultBroadPhaseLayerFilter(JPH::ObjectLayer{Entry.ObjectLayer});
            const auto LayerFilter      = PhysicsSystem->GetDefaultLayerFilter(JPH::ObjectLayer{Entry.ObjectLayer});

            Character->ExtendedUpdate(
                InFixedDt,
                Gravity,
                ExtendedSettings,
                BroadPhaseFilter,
                LayerFilter,
                JPH::BodyFilter{},
                JPH::ShapeFilter{},
                *_TempAllocator);

            Entry.OutLocation       = ck::jolt::Conv(Character->GetPosition());
            Entry.OutRotation       = ck::jolt::Conv(Character->GetRotation());
            Entry.OutGroundState    = Character->GetGroundState();
            Entry.OutGroundNormal   = ck::jolt::Conv(Character->GetGroundNormal());
            Entry.OutGroundVelocity = ck::jolt::Conv(Character->GetGroundVelocity());
            Entry.DirtyThisFrame    = true;
        }
    }

    auto
        FJoltWorld::
        DoApplyCharacterPoses_GameThread(
            const FCk_Handle& InTransientEntity)
        -> void
    {
        if (_CharacterRegistry.IsEmpty())
        { return; }

        const auto RegView = InTransientEntity.Get_RegistryView();

        for (auto& Entry : _CharacterRegistry)
        {
            if (NOT Entry.DirtyThisFrame)
            { continue; }

            // UserData 0 = NO entity, but raw id 0 is ALWAYS the registry's transient root (a LIVE entity),
            // so resolving it would mis-attribute the pose — guard before the liveness check.
            if (Entry.UserData == 0)
            {
                Entry.DirtyThisFrame = false;
                continue;
            }

            // A snapshot load can leave a stale id — non-ensuring liveness check first.
            const auto Entity = FCk_Entity{FCk_Entity::IdType{static_cast<FCk_Entity::IdType>(Entry.UserData)}};
            if (NOT RegView.IsValid(Entity))
            {
                Entry.DirtyThisFrame = false;
                continue;
            }

            auto Handle = InTransientEntity.Get_ValidHandle(Entity.Get_ID());
            if (ck::Is_NOT_Valid(Handle) ||
                NOT Handle.Has<ck::FFragment_JoltCharacter_Current>() ||
                NOT Handle.Has<ck::FFragment_JoltBody_StepPose>())
            {
                Entry.DirtyThisFrame = false;
                continue;
            }

            // A character rides the JoltBody interpolation path (FProcessor_JoltBody_WritebackInterpolated),
            // so it writes the SAME StepPose fragment + dirty tag.
            auto& Pose = Handle.Get<ck::FFragment_JoltBody_StepPose>();
            Pose.Set_PrevLocation(Pose.Get_CurrLocation())
                .Set_PrevRotation(Pose.Get_CurrRotation())
                .Set_CurrLocation(Entry.OutLocation)
                .Set_CurrRotation(Entry.OutRotation);

            Handle.AddOrGet<ck::FTag_JoltBody_TransformDirty>();

            auto& Current = Handle.Get<ck::FFragment_JoltCharacter_Current>();
            Current.Set_GroundNormalMirror(Entry.OutGroundNormal);
            Current.Set_GroundVelocityMirror(Entry.OutGroundVelocity);

            const auto NewGroundState = ck::jolt::Conv(Entry.OutGroundState);
            if (Current.Get_GroundStateMirror() != NewGroundState)
            {
                Current.Set_GroundStateMirror(NewGroundState);

                // The game-thread apply pass is the deterministic broadcast point for the ground-state edge.
                auto CharHandle = UCk_Utils_JoltCharacter_UE::Cast(Handle);
                UUtils_Signal_OnJoltCharacterGroundStateChanged::Broadcast(
                    CharHandle, ck::MakePayload(CharHandle, NewGroundState));
            }

            Entry.DirtyThisFrame = false;
        }
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

    auto
        FJoltWorld::
        Get_TempAllocator() const
        -> JPH::TempAllocatorImpl*
    {
        return _TempAllocator;
    }
}

// --------------------------------------------------------------------------------------------------------------------
