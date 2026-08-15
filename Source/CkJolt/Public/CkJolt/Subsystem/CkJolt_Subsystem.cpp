#include "CkJolt_Subsystem.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Debug/CkDebugDraw_Utils.h"

#include "CkEcs/Registry/CkRegistry.h"
#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"

#include "CkJolt/Body/CkJoltBody_ContactRouter.h"
#include "CkJolt/Body/CkJoltBody_Fragment.h"
#include "CkJolt/Body/CkJoltBody_Fragment_Data.h"
#include "CkJolt/CkJolt_ActivationEvent.h"
#include "CkJolt/CkJolt_Log.h"
#include "CkJolt/CkJolt_Stats.h"
#include "CkJolt/CkJolt_Utils.h"
#include "CkJolt/Settings/CkJolt_ProjectSettings.h"
#include "CkJolt/Subsystem/CkJolt_DebugDrawTarget.h"
#include "CkJolt/Subsystem/CkJolt_DebugRenderer.h"

#include <HAL/IConsoleManager.h>

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/JobSystemSingleThreaded.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>

#include <atomic>

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("Jolt_Subsystem_Tick"), STAT_CkJolt_SubsystemTick, STATGROUP_CkJolt);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_jolt_contactlistener
{
    static auto Fill_ContactDetail(
        FCk_Jolt_ContactEvent& InOutEvent,
        const JPH::Body& InBody1,
        const JPH::Body& InBody2,
        const JPH::ContactManifold& InManifold)
        -> void
    {
        InOutEvent.IsSensor1 = InBody1.IsSensor();
        InOutEvent.IsSensor2 = InBody2.IsSensor();
        InOutEvent.PenetrationDepth = InManifold.mPenetrationDepth;

        const auto RelativeVelocity = [&]() -> JPH::Vec3
        {
            if (InManifold.mRelativeContactPointsOn1.size() > 0)
            {
                const auto ContactPoint = InManifold.GetWorldSpaceContactPointOn1(0);
                return InBody2.GetPointVelocity(ContactPoint) - InBody1.GetPointVelocity(ContactPoint);
            }
            return InBody2.GetLinearVelocity() - InBody1.GetLinearVelocity();
        }();

        // ck::jolt::Conv is a Z-up passthrough with no unit scale (UE cm == Jolt units in this world), so the
        // normal-projected speed is already in UE units/s; use the same vector Conv on both operands.
        InOutEvent.RelativeNormalVelocity = static_cast<float>(
            FVector::DotProduct(ck::jolt::Conv(RelativeVelocity), ck::jolt::Conv(InManifold.mWorldSpaceNormal)));
    }
}

// --------------------------------------------------------------------------------------------------------------------

// Thread-safe contact listener: Jolt workers queue events, consumers drain and mutate ECS on the game thread.
class CkContactListener : public JPH::ContactListener
{
public:
    auto
        OnContactValidate(
            const JPH::Body& inBody1,
            const JPH::Body& inBody2,
            JPH::RVec3Arg inBaseOffset,
            const JPH::CollideShapeResult& inCollisionResult)
            -> JPH::ValidateResult override
    {
        return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
    }

    auto
        OnContactAdded(
            const JPH::Body& inBody1,
            const JPH::Body& inBody2,
            const JPH::ContactManifold& inManifold,
            JPH::ContactSettings& ioSettings)
            -> void override
    {
        ck::jolt::VeryVerbose(TEXT("Body [{}] and Body [{}] and SUB-SHAPE [{}] and SUB-SHAPE [{}] NEW Contact"),
            inBody1.GetID().GetIndex(), inBody2.GetID().GetIndex(),
            inManifold.mSubShapeID1.GetValue(), inManifold.mSubShapeID2.GetValue());

        auto Event = FCk_Jolt_ContactEvent{};
        Event.Type = FCk_Jolt_ContactEvent::EType::Added;
        Event.Body1UserData = inBody1.GetUserData();
        Event.Body2UserData = inBody2.GetUserData();
        Event.Body1IndexAndSeq = inBody1.GetID().GetIndexAndSequenceNumber();
        Event.Body2IndexAndSeq = inBody2.GetID().GetIndexAndSequenceNumber();

        Event.ContactPointsOn1 = ck::algo::Transform<TArray<FVector>>(
            inManifold.mRelativeContactPointsOn1.begin(),
            inManifold.mRelativeContactPointsOn1.end(),
            [&](const auto& ContactPoint)
            {
                return ck::jolt::Conv(ContactPoint + inManifold.mBaseOffset);
            });

        Event.ContactPointsOn2 = ck::algo::Transform<TArray<FVector>>(
            inManifold.mRelativeContactPointsOn2.begin(),
            inManifold.mRelativeContactPointsOn2.end(),
            [&](const auto& ContactPoint)
            {
                return ck::jolt::Conv(ContactPoint + inManifold.mBaseOffset);
            });

        Event.WorldSpaceNormal = ck::jolt::Conv(inManifold.mWorldSpaceNormal);

        ck_jolt_contactlistener::Fill_ContactDetail(Event, inBody1, inBody2, inManifold);

        {
            FScopeLock Lock(&_QueueLock);
            _BodyIdToUserData.Add(Event.Body1IndexAndSeq, Event.Body1UserData);
            _BodyIdToUserData.Add(Event.Body2IndexAndSeq, Event.Body2UserData);
            _ContactEventQueue.Emplace(MoveTemp(Event));
        }
    }

    auto
        OnContactPersisted(
            const JPH::Body& inBody1,
            const JPH::Body& inBody2,
            const JPH::ContactManifold& inManifold,
            JPH::ContactSettings& ioSettings)
            -> void override
    {
        // Persisted fires per still-touching manifold per sub-step on the Jolt workers. With nobody
        // interested every event below is built, locked, queued and then discarded on the game
        // thread — so bail before the log and before the first allocation. Reading the registry
        // here would be illegal (worker thread); only this pre-published atomic is.
        if (NOT _PersistedContactsWanted.load(std::memory_order_relaxed))
        { return; }

        ck::jolt::VeryVerbose(TEXT("Body [{}] and Body [{}] and SUB-SHAPE [{}] and SUB-SHAPE [{}] PERSISTED Contact"),
            inBody1.GetID().GetIndex(), inBody2.GetID().GetIndex(),
            inManifold.mSubShapeID1.GetValue(), inManifold.mSubShapeID2.GetValue());

        auto Event = FCk_Jolt_ContactEvent{};
        Event.Type = FCk_Jolt_ContactEvent::EType::Persisted;
        Event.Body1UserData = inBody1.GetUserData();
        Event.Body2UserData = inBody2.GetUserData();
        // Populated for Persisted too, so routers can tell WHICH of an entity's bodies (JoltBody vs Probe) this is.
        Event.Body1IndexAndSeq = inBody1.GetID().GetIndexAndSequenceNumber();
        Event.Body2IndexAndSeq = inBody2.GetID().GetIndexAndSequenceNumber();

        Event.ContactPointsOn1 = ck::algo::Transform<TArray<FVector>>(
            inManifold.mRelativeContactPointsOn1.begin(),
            inManifold.mRelativeContactPointsOn1.end(),
            [&](const auto& ContactPoint)
            {
                return ck::jolt::Conv(ContactPoint + inManifold.mBaseOffset);
            });

        Event.ContactPointsOn2 = ck::algo::Transform<TArray<FVector>>(
            inManifold.mRelativeContactPointsOn2.begin(),
            inManifold.mRelativeContactPointsOn2.end(),
            [&](const auto& ContactPoint)
            {
                return ck::jolt::Conv(ContactPoint + inManifold.mBaseOffset);
            });

        Event.WorldSpaceNormal = ck::jolt::Conv(inManifold.mWorldSpaceNormal);

        ck_jolt_contactlistener::Fill_ContactDetail(Event, inBody1, inBody2, inManifold);

        {
            FScopeLock Lock(&_QueueLock);
            _ContactEventQueue.Emplace(MoveTemp(Event));
        }
    }

    auto
        OnContactRemoved(
            const JPH::SubShapeIDPair& inSubShapePair)
            -> void override
    {
        ck::jolt::VeryVerbose(TEXT("Body [{}] and Body [{}] and SUB-SHAPE [{}] and SUB-SHAPE [{}] REMOVED Contact"),
            inSubShapePair.GetBody1ID().GetIndex(), inSubShapePair.GetBody2ID().GetIndex(),
            inSubShapePair.GetSubShapeID1().GetValue(), inSubShapePair.GetSubShapeID2().GetValue());

        auto Event = FCk_Jolt_ContactEvent{};
        Event.Type = FCk_Jolt_ContactEvent::EType::Removed;
        Event.Body1IndexAndSeq = inSubShapePair.GetBody1ID().GetIndexAndSequenceNumber();
        Event.Body2IndexAndSeq = inSubShapePair.GetBody2ID().GetIndexAndSequenceNumber();

        {
            FScopeLock Lock(&_QueueLock);

            if (const auto* UserData1 = _BodyIdToUserData.Find(Event.Body1IndexAndSeq))
            { Event.Body1UserData = *UserData1; }

            if (const auto* UserData2 = _BodyIdToUserData.Find(Event.Body2IndexAndSeq))
            { Event.Body2UserData = *UserData2; }

            _ContactEventQueue.Emplace(MoveTemp(Event));
        }
    }

    // Drain the queued events. Must be called from the game thread after Update() returns.
    auto DrainQueue(TArray<FCk_Jolt_ContactEvent>& OutEvents) -> void
    {
        FScopeLock Lock(&_QueueLock);
        OutEvents = MoveTemp(_ContactEventQueue);
        _ContactEventQueue.Reset();
    }

    // Written on the game thread in DrainEventsAndRoute, which orders before this frame's step, so
    // the workers never see a value from their own frame. One frame of staleness on a persisted
    // stream is immaterial; relaxed is therefore sufficient.
    auto Set_PersistedContactsWanted(bool InWanted) -> void
    {
        _PersistedContactsWanted.store(InWanted, std::memory_order_relaxed);
    }

private:
    FCriticalSection _QueueLock;
    TArray<FCk_Jolt_ContactEvent> _ContactEventQueue;

    std::atomic<bool> _PersistedContactsWanted{false};

    // Populated on ContactAdded, read on ContactRemoved (which carries no bodies). Entries persist for the
    // listener's lifetime: per-contact removal would break a body's other simultaneous end-overlaps.
    TMap<uint32, uint64> _BodyIdToUserData;
};

// --------------------------------------------------------------------------------------------------------------------

// Thread-safe activation listener (mirrors CkContactListener): Jolt workers queue, and
// FProcessor_JoltBody_SleepStateMirror drains + mutates ECS on the game thread.
class CkBodyActivationListener : public JPH::BodyActivationListener
{
public:
    auto
        OnBodyActivated(
            const JPH::BodyID& inBodyID,
            uint64 inBodyUserData)
            -> void override
    {
        ck::jolt::Verbose(TEXT("Body [{}] just ACTIVATED"), inBodyID.GetIndex());

        auto Event = FCk_Jolt_ActivationEvent{};
        Event.BodyIndexAndSeq = inBodyID.GetIndexAndSequenceNumber();
        Event.UserData = inBodyUserData;
        Event.NewState = ECk_Jolt_SleepState::Awake;

        {
            FScopeLock Lock(&_QueueLock);
            _ActivationEventQueue.Emplace(MoveTemp(Event));
        }
    }

    auto
        OnBodyDeactivated(
            const JPH::BodyID& inBodyID,
            uint64 inBodyUserData)
            -> void override
    {
        ck::jolt::Verbose(TEXT("Body [{}] just DE-ACTIVATED"), inBodyID.GetIndex());

        auto Event = FCk_Jolt_ActivationEvent{};
        Event.BodyIndexAndSeq = inBodyID.GetIndexAndSequenceNumber();
        Event.UserData = inBodyUserData;
        Event.NewState = ECk_Jolt_SleepState::Asleep;

        {
            FScopeLock Lock(&_QueueLock);
            _ActivationEventQueue.Emplace(MoveTemp(Event));
        }
    }

    // Drain the queued events. Must be called from the game thread after Update() returns.
    auto DrainQueue(TArray<FCk_Jolt_ActivationEvent>& OutEvents) -> void
    {
        FScopeLock Lock(&_QueueLock);
        OutEvents = MoveTemp(_ActivationEventQueue);
        _ActivationEventQueue.Reset();
    }

private:
    FCriticalSection _QueueLock;
    TArray<FCk_Jolt_ActivationEvent> _ActivationEventQueue;
};

// --------------------------------------------------------------------------------------------------------------------

static TAutoConsoleVariable<int32> CVarJoltEnableParallelPhysics(
    TEXT("jolt.EnableParallelPhysics"),
    -1,
    TEXT("Override parallel physics. -1 = use project setting (default), 0 = disable, 1 = enable.\n")
    TEXT("Uses JobSystemThreadPool when enabled, JobSystemSingleThreaded when disabled.\n")
    TEXT("Only evaluated at subsystem initialization; runtime changes have no effect.")
);

static TAutoConsoleVariable<int32> CVarJoltEnableAsyncPhysicsUpdate(
    TEXT("jolt.EnableAsyncPhysicsUpdate"),
    -1,
    TEXT("Override async physics update. -1 = use project setting (default), 0 = disable, 1 = enable.\n")
    TEXT("When enabled, PhysicsSystem::Update() runs on a background thread (one-frame latent results).\n")
    TEXT("Only evaluated at subsystem initialization; runtime changes have no effect.")
);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_jolt_subsystem
{
    namespace cvar
    {
        static bool DebugDrawEnabled = false;
        static FAutoConsoleVariableRef CVar_DebugDrawEnabled(TEXT("ck.Jolt.DebugDraw.Enabled"),
            DebugDrawEnabled,
            TEXT("Draw the Jolt physics world (static AND dynamic bodies) via Jolt's debug renderer, "
                 "independent of any consumer opt-in gate. Skipped during async physics frames."));

        static bool DebugDrawSleepColoring = false;
        static FAutoConsoleVariableRef CVar_DebugDrawSleepColoring(TEXT("ck.Jolt.DebugDraw.SleepColoring"),
            DebugDrawSleepColoring,
            TEXT("When drawing the Jolt world, color bodies by sleep state (JPH SleepColor: static grey, "
                 "keyframed green, dynamic yellow, sleeping red) instead of motion type (MotionTypeColor)."));

        static bool DebugDrawVelocity = true;
        static FAutoConsoleVariableRef CVar_DebugDrawVelocity(TEXT("ck.Jolt.DebugDraw.Velocity"),
            DebugDrawVelocity,
            TEXT("When drawing the Jolt world, draw each active body's linear-velocity vector "
                 "(immediate-mode lines; one per moving body)."));

        static bool DebugDrawWorldTransform = false;
        static FAutoConsoleVariableRef CVar_DebugDrawWorldTransform(TEXT("ck.Jolt.DebugDraw.WorldTransform"),
            DebugDrawWorldTransform,
            TEXT("When drawing the Jolt world, draw each body's world-transform axes. Off by default: "
                 "the per-body arrow lines are immediate-mode and dominate the line batcher at "
                 "stress-gym body counts."));

        static float DebugDrawOpacity = 0.5f;
        static FAutoConsoleVariableRef CVar_DebugDrawOpacity(TEXT("ck.Jolt.DebugDraw.Opacity"),
            DebugDrawOpacity,
            TEXT("Opacity of the batched Jolt debug-draw meshes (0..1). Applies live."));

        static bool DebugDrawConstraints = true;
        static FAutoConsoleVariableRef CVar_DebugDrawConstraints(TEXT("ck.Jolt.DebugDraw.Constraints"),
            DebugDrawConstraints,
            TEXT("When drawing the Jolt world, also draw constraints (anchors, hinge axes, limits)."));
    }

    static auto ResolveCVarOverride(const TCHAR* InCVarName, int32 InCVarValue, bool InProjectSettingValue, const TCHAR* InSettingName) -> bool
    {
        // Command line first: FParse::Value works at any startup point, unlike the engine's CVar pass.
        int32 CmdLineValue = -1;
        FParse::Value(FCommandLine::Get(), *ck::Format_UE(TEXT("{}="), InCVarName), CmdLineValue);

        const int32 EffectiveValue = (CmdLineValue >= 0) ? CmdLineValue : InCVarValue;

        if (EffectiveValue >= 0)
        {
            const bool bOverrideValue = (EffectiveValue != 0);
            const TCHAR* Source = (CmdLineValue >= 0) ? TEXT("command line") : TEXT("CVar");
            ck::jolt::Log(TEXT("Jolt: [{}] overridden by {} to [{}]"), InSettingName, Source, bOverrideValue);
            return bOverrideValue;
        }
        return InProjectSettingValue;
    }
}

auto
    UCk_Jolt_Subsystem::
    Initialize(
        FSubsystemCollectionBase& InCollection)
        -> void
{
    Super::Initialize(InCollection);
    _EcsWorldSubsystem = InCollection.InitializeDependency<UCk_EcsWorld_Subsystem_UE>();

    using namespace JPH;

    ck::jolt::Request_GlobalJoltInit();

    const auto MaxBodies = static_cast<uint>(UCk_Utils_Jolt_ProjectSettings::Get_MaxBodies());
    constexpr uint NumBodyMutexes = 0;
    const auto MaxBodyPairs = static_cast<uint>(UCk_Utils_Jolt_ProjectSettings::Get_MaxBodyPairs());
    const auto MaxContactConstraints = static_cast<uint>(UCk_Utils_Jolt_ProjectSettings::Get_MaxContactConstraints());

    const auto MaxPhysicsJobs = UCk_Utils_Jolt_ProjectSettings::Get_MaxPhysicsJobs();
    const auto MaxPhysicsBarriers = UCk_Utils_Jolt_ProjectSettings::Get_MaxPhysicsBarriers();
    const auto TempAllocatorSizeBytes = UCk_Utils_Jolt_ProjectSettings::Get_TempAllocatorSizeMB() * 1024u * 1024u;

    _CollisionSteps = UCk_Utils_Jolt_ProjectSettings::Get_CollisionSteps();

    _TempAllocator = MakePimpl<TempAllocatorImpl>(TempAllocatorSizeBytes);

    const bool bEnableParallel = ck_jolt_subsystem::ResolveCVarOverride(
        TEXT("jolt.EnableParallelPhysics"),
        CVarJoltEnableParallelPhysics.GetValueOnGameThread(),
        UCk_Utils_Jolt_ProjectSettings::Get_EnableParallelPhysics(),
        TEXT("EnableParallelPhysics"));

    _ParallelPhysicsEnabled = bEnableParallel;

    if (_ParallelPhysicsEnabled)
    {
        auto NumThreads = UCk_Utils_Jolt_ProjectSettings::Get_NumPhysicsThreads();
        if (NumThreads <= 0)
        {
            NumThreads = FMath::Max(1, static_cast<int32>(std::thread::hardware_concurrency()) - 1);
        }

        _PhysicsThreadCount = NumThreads;

        ck::jolt::Log(TEXT("Jolt: Creating JobSystemThreadPool with [{}] threads"), _PhysicsThreadCount);

        // Two-step: SetThreadInitFunction must be set before Init() (names the workers for Insights).
        auto* ThreadPool = new JobSystemThreadPool();

        ThreadPool->SetThreadInitFunction([](int InThreadIndex)
        {
            const auto ThreadName = ck::Format_UE(TEXT("JoltWorker_{}"), InThreadIndex);
            FPlatformProcess::SetThreadName(*ThreadName);
        });

        ThreadPool->Init(MaxPhysicsJobs, MaxPhysicsBarriers, _PhysicsThreadCount);
        _JobSystem = ThreadPool;
    }
    else
    {
        _PhysicsThreadCount = 0;
        ck::jolt::Log(TEXT("Jolt: Creating JobSystemSingleThreaded"));
        _JobSystem = new JPH::JobSystemSingleThreaded(MaxPhysicsJobs);
    }

    // The filters hold references to the table AND are referenced by the PhysicsSystem, so both must
    // outlive it (they do: Deinitialize resets these members after the PhysicsSystem).
    _LayerTable = MakeUnique<ck::jolt::FCk_Jolt_CollisionLayerTable>();
    _LayerTable->Build_FromCollisionProfiles();

    _BroadPhaseLayerInterface = MakeUnique<ck::jolt::FCk_Jolt_BroadPhaseLayerInterface_Table>(*_LayerTable);
    _ObjectVsBroadPhaseLayerFilter = MakeUnique<ck::jolt::FCk_Jolt_ObjectVsBroadPhaseLayerFilter_Table>(*_LayerTable);
    _ObjectVsObjectFilter = MakeUnique<ck::jolt::FCk_Jolt_ObjectLayerPairFilter_Table>(*_LayerTable);

    _PhysicsSystem = MakeShared<PhysicsSystem>();
    _PhysicsSystem->Init(MaxBodies, NumBodyMutexes, MaxBodyPairs, MaxContactConstraints, *_BroadPhaseLayerInterface,
        *_ObjectVsBroadPhaseLayerFilter, *_ObjectVsObjectFilter);

    // Jolt's default gravity is (0, -9.81, 0) — Y-down in METERS. This world is Z-up passthrough in UE
    // centimeters, so take the UE world's own gravity (Chaos parity; respects per-world overrides).
    _PhysicsSystem->SetGravity(ck::jolt::Conv(FVector{0.0, 0.0, GetWorld()->GetGravityZ()}));

    // Jolt's PhysicsSettings defaults are METERS-tuned; this world is CENTIMETERS. Every length/velocity
    // field below is the converted default (x100; the squared manifold tolerance x100^2) — the trailing
    // comments are the originals. Ratios, iteration counts, and times keep their defaults.
    {
        auto PhysicsSettings = _PhysicsSystem->GetPhysicsSettings();
        PhysicsSettings.mSpeculativeContactDistance   = 2.0f;      // 0.02 m
        PhysicsSettings.mPenetrationSlop              = 2.0f;      // 0.02 m
        PhysicsSettings.mMaxPenetrationDistance       = 20.0f;     // 0.2 m
        PhysicsSettings.mManifoldToleranceSq          = 1.0e-2f;   // 1.0e-6 m^2
        PhysicsSettings.mPointVelocitySleepThreshold  = 3.0f;      // 0.03 m/s
        PhysicsSettings.mMinVelocityForRestitution    = 100.0f;    // 1 m/s
        _PhysicsSystem->SetPhysicsSettings(PhysicsSettings);
    }

    _BodyActivationListener = MakePimpl<CkBodyActivationListener>();
    _PhysicsSystem->SetBodyActivationListener(_BodyActivationListener.Get());

    _ContactListener = MakePimpl<CkContactListener>();
    _PhysicsSystem->SetContactListener(&*_ContactListener);

    _EcsWorldSubsystem->Get_Registry().SetContext<TWeakPtr<JPH::PhysicsSystem>>(_PhysicsSystem);
    _EcsWorldSubsystem->Get_Registry().SetContext<ck::jolt::FCk_Jolt_LayerContext>(
        ck::jolt::FCk_Jolt_LayerContext{_LayerTable.Get(), _LayerTable->Get_ProbeLayer()});

    _AsyncPhysicsUpdate = ck_jolt_subsystem::ResolveCVarOverride(
        TEXT("jolt.EnableAsyncPhysicsUpdate"),
        CVarJoltEnableAsyncPhysicsUpdate.GetValueOnGameThread(),
        UCk_Utils_Jolt_ProjectSettings::Get_EnableAsyncPhysicsUpdate(),
        TEXT("EnableAsyncPhysicsUpdate"));

    if (_AsyncPhysicsUpdate)
    {
        ck::jolt::Log(TEXT("Jolt: Async physics update ENABLED (one-frame latent)"));
    }

    _JoltWorld = MakeShared<ck::FJoltWorld>(ck::FJoltWorld::FInitParams
    {
        .PhysicsSystem = _PhysicsSystem,
        .TempAllocator = _TempAllocator.Get(),
        .JobSystem = _JobSystem,
        .World = GetWorld(),
        .CollisionSteps = _CollisionSteps,
        .AsyncMode = _AsyncPhysicsUpdate,
        .DrainQueueFn = [this](TArray<FCk_Jolt_ContactEvent>& OutEvents)
        {
            _ContactListener->DrainQueue(OutEvents);
        },
        .DrainActivationQueueFn = [this](TArray<FCk_Jolt_ActivationEvent>& OutEvents)
        {
            _BodyActivationListener->DrainQueue(OutEvents);
        },
        .SetPersistedContactsWantedFn = [this](bool InWanted)
        {
            _ContactListener->Set_PersistedContactsWanted(InWanted);
        },
    });

    _EcsWorldSubsystem->Get_Registry().SetContext<TSharedPtr<ck::FJoltWorld>>(_JoltWorld);

    // Weak self-capture so a torn-down subsystem is never invoked by the router registry; the transient
    // entity is resolved at drain time, game-thread, inside FProcessor_JoltWorld_DrainEvents.
    const auto WeakThis = TWeakObjectPtr<UCk_Jolt_Subsystem>{this};
    RegisterContactRouter(TEXT("JoltBody.Signals"),
        [WeakThis](const TArray<FCk_Jolt_ContactEvent>& InEvents)
        {
            auto* Self = WeakThis.Get();
            if (Self == nullptr || ck::Is_NOT_Valid(Self->_EcsWorldSubsystem))
            { return; }

            ck::jolt_body::RouteContactEvents(Self->_EcsWorldSubsystem->Get_TransientEntity(), InEvents);
        });

    Register_PersistedContactInterestProvider(TEXT("JoltBody.PersistContacts"),
        [WeakThis]() -> bool
        {
            auto* Self = WeakThis.Get();
            if (Self == nullptr || ck::Is_NOT_Valid(Self->_EcsWorldSubsystem))
            { return false; }

            return Self->_EcsWorldSubsystem->Get_Registry().Has_AnyLiveEntityWith<ck::FTag_JoltBody_PersistContacts>();
        });

#if JPH_DEBUG_RENDERER
    FCk_Jolt_DebugRenderer::Get_OrCreate();
    _DefaultDebugDrawTarget = MakeShared<FCk_Jolt_DebugDrawTarget>(GetWorld());
#endif
}

auto
    UCk_Jolt_Subsystem::
    Tick(
        float InDeltaTime)
        -> void
{
    SCOPE_CYCLE_COUNTER(STAT_CkJolt_SubsystemTick);
    Super::Tick(InDeltaTime);

    // Only the debug draw lives here; the physics step runs in the FGroup_Transform processors. This
    // Tick and the ECS group order are unpinned, so the draw may lag the step by one frame — accepted.

#if JPH_DEBUG_RENDERER
    // Async mode: the step is in flight and physics state is not stable, so skip the draw entirely.
    if (_JoltWorld.IsValid() && NOT _JoltWorld->Get_AsyncMode())
    {
        constexpr auto DrawGetSupportFeatures = false;
        constexpr auto DrawSupportDirection = false;
        constexpr auto DrawGetSupportingFace = false;
        constexpr auto DrawShape = true;
        // The batched renderer draws real instanced meshes; the EDrawMode wireframe hint is ignored there.
        constexpr auto DrawShapeWireframe = false;
        const auto DrawShapeColor = ck_jolt_subsystem::cvar::DebugDrawSleepColoring
            ? JPH::BodyManager::EShapeColor::SleepColor
            : JPH::BodyManager::EShapeColor::MotionTypeColor;
        constexpr auto DrawBoundingBox = false;
        constexpr auto DrawCenterOfMassTransform = false;
        const auto DrawWorldTransform = ck_jolt_subsystem::cvar::DebugDrawWorldTransform;
        const auto DrawVelocity = ck_jolt_subsystem::cvar::DebugDrawVelocity;
        constexpr auto DrawMassAndInertia = false;
        constexpr auto DrawSleepStats = false;
        constexpr auto DrawSoftBodyVertices = false;
        constexpr auto DrawSoftBodyVertexVelocities = false;
        constexpr auto DrawSoftBodyEdgeConstraints = false;
        constexpr auto DrawSoftBodyBendConstraints = false;
        constexpr auto DrawSoftBodyVolumeConstraints = false;
        constexpr auto DrawSoftBodySkinConstraints = false;
        constexpr auto DrawSoftBodyLraConstraints = false;
        constexpr auto DrawSoftBodyPredictedBounds = false;
        constexpr auto DrawSoftBodyConstraintColor = JPH::ESoftBodyConstraintColor::ConstraintType;

        const auto DrawSettings = JPH::BodyManager::DrawSettings
        {
            DrawGetSupportFeatures,
            DrawSupportDirection,
            DrawGetSupportingFace,
            DrawShape,
            DrawShapeWireframe,
            DrawShapeColor,
            DrawBoundingBox,
            DrawCenterOfMassTransform,
            DrawWorldTransform,
            DrawVelocity,
            DrawMassAndInertia,
            DrawSleepStats,
            DrawSoftBodyVertices,
            DrawSoftBodyVertexVelocities,
            DrawSoftBodyEdgeConstraints,
            DrawSoftBodyBendConstraints,
            DrawSoftBodyVolumeConstraints,
            DrawSoftBodySkinConstraints,
            DrawSoftBodyLraConstraints,
            DrawSoftBodyPredictedBounds,
            DrawSoftBodyConstraintColor
        };

        const auto ConsumerGateOpen = _DebugDrawGate && _DebugDrawGate();
        const auto CVarDrawEnabled  = ck_jolt_subsystem::cvar::DebugDrawEnabled;

        if (_DefaultDebugDrawTarget.IsValid())
        {
            if (ConsumerGateOpen || CVarDrawEnabled)
            {
                auto& Renderer = FCk_Jolt_DebugRenderer::Get_OrCreate();

                _DefaultDebugDrawTarget->Set_Opacity(ck_jolt_subsystem::cvar::DebugDrawOpacity);

                Renderer.BeginFrame(*_DefaultDebugDrawTarget);
                _PhysicsSystem->DrawBodies(DrawSettings, &Renderer);

                if (ck_jolt_subsystem::cvar::DebugDrawConstraints)
                { _PhysicsSystem->DrawConstraints(&Renderer); }

                Renderer.EndFrame();
                Renderer.NextFrame();
            }
            else
            {
                // Without this the last frame's instanced meshes linger frozen once the gate closes.
                _DefaultDebugDrawTarget->HideAll();
            }
        }
    }
#endif
}

auto
    UCk_Jolt_Subsystem::
    Deinitialize()
        -> void
{
    // Wait any in-flight async step and null the Jolt world's non-owning pointers BEFORE destroying the
    // objects they reference. The registry context keeps its TSharedPtr (SetContext has no overwrite
    // variant) — safe: the shut-down world is inert, and the registry dies with the world anyway.
    if (_JoltWorld.IsValid())
    { _JoltWorld->Shutdown(); }

    _DebugDrawGate = {};

#if JPH_DEBUG_RENDERER
    // Destroyed while the world is still valid so the target can release its instanced components.
    _DefaultDebugDrawTarget.Reset();
    _RegisteredDebugDrawTargets.Reset();
#endif

    _ContactListener.Reset();
    _BodyActivationListener.Reset();
    _PhysicsSystem.Reset();
    _ObjectVsObjectFilter.Reset();
    _ObjectVsBroadPhaseLayerFilter.Reset();
    _BroadPhaseLayerInterface.Reset();
    _LayerTable.Reset();
    delete _JobSystem;
    _JobSystem = nullptr;
    _TempAllocator.Reset();

    _JoltWorld.Reset();

    ck::jolt::Request_GlobalJoltShutdown();

    Super::Deinitialize();
}

auto
    UCk_Jolt_Subsystem::
    Get_PhysicsSystem() const
        -> TWeakPtr<JPH::PhysicsSystem>
{
    return _PhysicsSystem;
}

auto
    UCk_Jolt_Subsystem::
    RegisterContactRouter(
        FName InName,
        ck::FCk_Jolt_ContactEventRouter InRouter)
        -> void
{
    CK_ENSURE_IF_NOT(_JoltWorld.IsValid(),
        TEXT("Cannot register contact router [{}] — the Jolt world does not exist (called before Initialize or after Deinitialize?)"), InName)
    { return; }

    _JoltWorld->RegisterContactRouter(InName, MoveTemp(InRouter));
}

auto
    UCk_Jolt_Subsystem::
    UnregisterContactRouter(
        FName InName)
        -> void
{
    if (NOT _JoltWorld.IsValid())
    { return; }

    _JoltWorld->UnregisterContactRouter(InName);
}

auto
    UCk_Jolt_Subsystem::
    Register_PersistedContactInterestProvider(
        FName InName,
        TFunction<bool()> InProvider)
        -> void
{
    CK_ENSURE_IF_NOT(_JoltWorld.IsValid(),
        TEXT("Cannot register persisted-contact interest provider [{}] — the Jolt world does not exist (called before Initialize or after Deinitialize?)"), InName)
    { return; }

    _JoltWorld->Register_PersistedContactInterestProvider(InName, MoveTemp(InProvider));
}

auto
    UCk_Jolt_Subsystem::
    Unregister_PersistedContactInterestProvider(
        FName InName)
        -> void
{
    if (NOT _JoltWorld.IsValid())
    { return; }

    _JoltWorld->Unregister_PersistedContactInterestProvider(InName);
}

auto
    UCk_Jolt_Subsystem::
    Set_DebugDrawGate(
        TFunction<bool()> InGate)
        -> void
{
    _DebugDrawGate = MoveTemp(InGate);
}

auto
    UCk_Jolt_Subsystem::
    Get_LayerTable()
        -> ck::jolt::FCk_Jolt_CollisionLayerTable&
{
    return *_LayerTable;
}

#if JPH_DEBUG_RENDERER
auto
    UCk_Jolt_Subsystem::
    Register_DebugDrawTarget(
        const TSharedRef<FCk_Jolt_DebugDrawTarget>& InTarget)
        -> void
{
    const auto* TargetPtr = &InTarget.Get();

    const auto AlreadyRegistered = _RegisteredDebugDrawTargets.ContainsByPredicate(
        [&](const TWeakPtr<FCk_Jolt_DebugDrawTarget>& InExisting) -> bool
        {
            return InExisting.Pin().Get() == TargetPtr;
        });

    CK_ENSURE_IF_NOT(NOT AlreadyRegistered,
        TEXT("The same Jolt debug-draw target was registered twice — ignoring the second registration"))
    { return; }

    _RegisteredDebugDrawTargets.Emplace(TWeakPtr<FCk_Jolt_DebugDrawTarget>{InTarget});
}

auto
    UCk_Jolt_Subsystem::
    Unregister_DebugDrawTarget(
        const TSharedRef<FCk_Jolt_DebugDrawTarget>& InTarget)
        -> void
{
    const auto* TargetPtr = &InTarget.Get();

    _RegisteredDebugDrawTargets.RemoveAll(
        [&](const TWeakPtr<FCk_Jolt_DebugDrawTarget>& InExisting) -> bool
        {
            const auto Pinned = InExisting.Pin();
            return NOT Pinned.IsValid() || Pinned.Get() == TargetPtr;
        });
}

auto
    UCk_Jolt_Subsystem::
    Get_DemandingDebugDrawTargets(
        TArray<TSharedPtr<FCk_Jolt_DebugDrawTarget>>& OutTargets)
        -> void
{
    OutTargets.Reset();

    _RegisteredDebugDrawTargets.RemoveAll(
        [](const TWeakPtr<FCk_Jolt_DebugDrawTarget>& InExisting) -> bool
        {
            return NOT InExisting.IsValid();
        });

    for (const auto& WeakTarget : _RegisteredDebugDrawTargets)
    {
        const auto Target = WeakTarget.Pin();
        if (NOT Target.IsValid())
        { continue; }

        if (NOT Target->Get_IsDesired())
        {
            // Belt-and-braces for demand dropping through some path other than Set_IsDesired: a target that is
            // no longer pumped must not leave a frozen snapshot of the world behind it. Self-early-outs.
            Target->HideAll();
            continue;
        }

        OutTargets.Emplace(Target);
    }
}
#endif

auto
    UCk_Jolt_Subsystem::
    Request_NoteStaticSceneChanged()
        -> void
{
    if (NOT _JoltWorld.IsValid())
    { return; }

    _JoltWorld->Request_NoteStaticSceneChanged();
}

auto
    UCk_Jolt_Subsystem::
    Request_NoteBodyRemoved()
        -> void
{
    if (NOT _JoltWorld.IsValid())
    { return; }

    _JoltWorld->Request_NoteBodyRemoved();
}

auto
    UCk_Jolt_Subsystem::
    Request_OptimizeBroadPhaseBeforeNextUpdate()
        -> void
{
    if (NOT _JoltWorld.IsValid())
    { return; }

    _JoltWorld->Request_OptimizeBroadPhaseBeforeNextUpdate();
}

// --------------------------------------------------------------------------------------------------------------------
