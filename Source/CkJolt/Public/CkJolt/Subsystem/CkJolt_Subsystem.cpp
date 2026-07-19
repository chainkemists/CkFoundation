#include "CkJolt_Subsystem.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Debug/CkDebugDraw_Utils.h"

#include "CkEcs/Registry/CkRegistry.h"
#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"

#include "CkJolt/Body/CkJoltBody_ContactRouter.h"
#include "CkJolt/Body/CkJoltBody_Fragment_Data.h"
#include "CkJolt/CkJolt_ActivationEvent.h"
#include "CkJolt/CkJolt_Log.h"
#include "CkJolt/CkJolt_Stats.h"
#include "CkJolt/CkJolt_Utils.h"
#include "CkJolt/Settings/CkJolt_ProjectSettings.h"
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

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("Jolt_Subsystem_Tick"), STAT_CkJolt_SubsystemTick, STATGROUP_CkJolt);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_jolt_contactlistener
{
    // Fills the contact-detail fields shared by Added and Persisted events: sensor flags, penetration depth,
    // and the closing speed along the contact normal (UE units/s).
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

        // Closing velocity along the contact normal. Prefer the velocity at the actual contact point; when
        // the manifold carries no contact points yet, fall back to the bodies' COM linear velocities.
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

// Thread-safe contact listener that queues events for deferred processing.
// All ECS mutations happen on the game thread by consumers of the drained-events broadcast.
class CkContactListener : public JPH::ContactListener
{
public:
    // See: ContactListener
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
        ck::jolt::VeryVerbose(TEXT("Body [{}] and Body [{}] and SUB-SHAPE [{}] and SUB-SHAPE [{}] PERSISTED Contact"),
            inBody1.GetID().GetIndex(), inBody2.GetID().GetIndex(),
            inManifold.mSubShapeID1.GetValue(), inManifold.mSubShapeID2.GetValue());

        auto Event = FCk_Jolt_ContactEvent{};
        Event.Type = FCk_Jolt_ContactEvent::EType::Persisted;
        Event.Body1UserData = inBody1.GetUserData();
        Event.Body2UserData = inBody2.GetUserData();
        // Populated for Persisted (not just Added/Removed) so the per-body signal routers can disambiguate
        // which of an entity's bodies (JoltBody vs. Probe) this contact belongs to.
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

            // Look up entity IDs from the body map (populated during OnContactAdded)
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

private:
    FCriticalSection _QueueLock;
    TArray<FCk_Jolt_ContactEvent> _ContactEventQueue;

    // Maps BodyID (index+sequence) -> UserData (entity ID).
    // Populated during OnContactAdded, read during OnContactRemoved.
    // Entries persist for the listener's lifetime — a body may have simultaneous
    // contacts with multiple other bodies, so per-contact removal would break
    // subsequent end-overlap resolution. Protected by _QueueLock.
    TMap<uint32, uint64> _BodyIdToUserData;
};

// --------------------------------------------------------------------------------------------------------------------

// Thread-safe activation listener that queues activate/deactivate events for deferred processing.
// Jolt fires these from worker threads during Update; ECS mutations happen on the game thread in
// FProcessor_JoltBody_SleepStateMirror, which drains the queue. Mirrors CkContactListener.
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

// Console variables for command-line override of Jolt threading settings.
// Values: -1 = use project setting (default), 0 = disable, 1 = enable.
// These are read-only (startup only) because the JobSystem is created once during Initialize().
// Usage: -jolt.EnableParallelPhysics=0  or  -jolt.EnableAsyncPhysicsUpdate=1
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
    // Runtime debug-draw CVars. Mirrors the house C++ CVar pattern (FAutoConsoleVariableRef over a
    // static in a filename-derived named namespace — see ck.SpatialQuery.PreviewAllProbesUsingJolt in
    // CkSpatialQuery_Settings.cpp). Read game-thread in Tick; the subsystem draws when a consumer gate
    // opts in OR Enabled is true. DrawBodies covers static AND dynamic bodies (no body filter).
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

        static bool DebugDrawConstraints = true;
        static FAutoConsoleVariableRef CVar_DebugDrawConstraints(TEXT("ck.Jolt.DebugDraw.Constraints"),
            DebugDrawConstraints,
            TEXT("When drawing the Jolt world, also draw constraints (anchors, hinge axes, limits)."));
    }

    // Resolve a CVar override against a project setting default.
    // Checks the command line first (timing-independent), then the CVar value.
    // Returns the override if explicitly set (0 or 1), otherwise the project setting.
    static auto ResolveCVarOverride(const TCHAR* InCVarName, int32 InCVarValue, bool InProjectSettingValue, const TCHAR* InSettingName) -> bool
    {
        // FParse::Value works at any point during startup — no dependency on the
        // engine's CVar command-line processing pass.
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

        // Two-step construction so we can register Jolt worker threads with Unreal Insights
        // before they start. SetThreadInitFunction/SetThreadExitFunction must be set before Init().
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

    // Signature-driven layers, seeded from the project's own collision profiles — never
    // hand-authored. The filter instances hold references to the table AND are referenced by
    // the PhysicsSystem, so both must outlive it (they do: members reset in Deinitialize after
    // the PhysicsSystem).
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
    // Latent until Phase 3: probes are gravity-less kinematic sensors, so nothing fell before dynamic bodies.
    _PhysicsSystem->SetGravity(ck::jolt::Conv(FVector{0.0, 0.0, GetWorld()->GetGravityZ()}));

    // Jolt's PhysicsSettings defaults are METERS-tuned; this world is CENTIMETERS. Convert every
    // length/velocity-based field (x100; the squared manifold tolerance x100^2). Unconverted, the
    // 0.02cm penetration slop keeps stacked bodies in permanent micro-jitter and the 0.03cm/s sleep
    // threshold makes stacks effectively unable to sleep (exposed by BoxStackOfFiveSettlesAndStays
    // once it gated on real velocity quiescence). Ratios (mBaumgarte, mLinearCast*), iteration
    // counts, and times keep their defaults.
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

    // The step engine. Holds non-owning pointers into the objects created above; the FGroup_Transform
    // step processors read it from the registry context (published ALONGSIDE the two existing contexts).
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
    });

    _EcsWorldSubsystem->Get_Registry().SetContext<TSharedPtr<ck::FJoltWorld>>(_JoltWorld);

    // Route drained contact events into per-JoltBody contact signals. Weak self-capture so a torn-down
    // subsystem is never invoked by the router registry; the transient entity is resolved at drain time
    // (mirrors the SpatialQuery probe bridge). Runs game-thread inside FProcessor_JoltWorld_DrainEvents.
    const auto WeakThis = TWeakObjectPtr<UCk_Jolt_Subsystem>{this};
    RegisterContactRouter(TEXT("JoltBody.Signals"),
        [WeakThis](const TArray<FCk_Jolt_ContactEvent>& InEvents)
        {
            auto* Self = WeakThis.Get();
            if (Self == nullptr || ck::Is_NOT_Valid(Self->_EcsWorldSubsystem))
            { return; }

            ck::jolt_body::RouteContactEvents(Self->_EcsWorldSubsystem->Get_TransientEntity(), InEvents);
        });

#if JPH_DEBUG_RENDERER
    if (ck::Is_NOT_Valid(JPH::DebugRenderer::sInstance, ck::IsValid_Policy_NullptrOnly{}))
    {
        _Debugger = MakePimpl<CkJoltDebugger>();
        _Debugger->_World = GetWorld();
    }
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

    // The physics step (wait-async → drain → optimize → update) now runs in ECS processors
    // (FGroup_Transform, after Transform request handling). Only the debug draw remains here. The
    // subsystem Tick and the ECS group order are unpinned, so this draw may lag the step by one
    // frame; accepted.

#if JPH_DEBUG_RENDERER
    // Debug rendering requires physics state to be stable — skip in async mode (the step is in flight;
    // results arrive next frame), mirroring the pre-split "NOT _AsyncPhysicsUpdate" gate.
    if (_JoltWorld.IsValid() && NOT _JoltWorld->Get_AsyncMode())
    {
        // Named constants for clear initialization
        constexpr auto DrawGetSupportFeatures = false;
        constexpr auto DrawSupportDirection = false;
        constexpr auto DrawGetSupportingFace = false;
        constexpr auto DrawShape = true;
        // The batched renderer draws real instanced meshes; wireframe mode is meaningless there and
        // stays off (it only changed the EDrawMode hint, which the batch path ignores).
        constexpr auto DrawShapeWireframe = false;
        // Sleep coloring switches the shape-color mode: SleepColor tints sleeping dynamic bodies red,
        // MotionTypeColor tints by motion type. Runtime-selected, so DrawSettings below is const not constexpr.
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

        // Gate ORs: an installed consumer opt-in (CkSpatialQuery's PreviewAllProbesUsingJolt) OR the
        // ck.Jolt.DebugDraw.Enabled CVar. The probe-only path keeps working unchanged when the CVar is off.
        const auto ConsumerGateOpen = _DebugDrawGate && _DebugDrawGate();
        const auto CVarDrawEnabled  = ck_jolt_subsystem::cvar::DebugDrawEnabled;

        if (ck::IsValid(_Debugger, ck::IsValid_Policy_NullptrOnly{}))
        {
            if (ConsumerGateOpen || CVarDrawEnabled)
            {
                _Debugger->BeginFrame();
                _PhysicsSystem->DrawBodies(DrawSettings, _Debugger.Get());

                if (ck_jolt_subsystem::cvar::DebugDrawConstraints)
                { _PhysicsSystem->DrawConstraints(_Debugger.Get()); }

                _Debugger->EndFrame();
                _Debugger->NextFrame();
            }
            else
            {
                // One-shot when the gate closes: without this, the last frame's instanced meshes
                // linger frozen in the world. Idempotent and free once cleared.
                _Debugger->HideAll();
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
    // objects they reference. The registry context still holds a TSharedPtr<ck::FJoltWorld>, but
    // FCk_Registry::SetContext wraps entt ctx::emplace (CkRegistry.h) — try_emplace semantics that do NOT
    // overwrite an existing entry, and there is no overwrite variant — so the context cannot be cleared
    // here. That is safe: the shut-down world is inert (pointers nulled), and the registry (with its
    // context) is destroyed alongside the world during teardown.
    if (_JoltWorld.IsValid())
    { _JoltWorld->Shutdown(); }

    _DebugDrawGate = {};

#if JPH_DEBUG_RENDERER
    // Destroyed while the world is still valid so the renderer can release its instanced components.
    _Debugger.Reset();
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
