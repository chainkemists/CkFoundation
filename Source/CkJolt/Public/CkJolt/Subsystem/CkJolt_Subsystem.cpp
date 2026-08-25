#include "CkJolt_Subsystem.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Diagnostics/CkDiagnosticVisibility.h"

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
#include <Jolt/Physics/Body/Body.h>
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
    /*
     * The per-step contact-pair counter this listener feeds (P6-D48). The counter itself lives on the Jolt WORLD
     * rather than here, because the world is what owns the step that resets it — this listener only ever knows
     * that a pair happened. Null until the world exists (it is built after the listener) and again after it dies.
     */
    auto Set_ContactPairSink(ck::FJoltWorld* InSink) -> void
    {
        _ContactPairSink = InSink;
    }

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

        if (_ContactPairSink != nullptr)
        { _ContactPairSink->Note_ContactPair(); }

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
#if !UE_BUILD_SHIPPING
            _BodyIdToIsSensor.Add(Event.Body1IndexAndSeq, Event.IsSensor1);
            _BodyIdToIsSensor.Add(Event.Body2IndexAndSeq, Event.IsSensor2);
#endif
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
        // Counted before the interest gate below: the debug-draw step stat wants every touching manifold,
        // and this is a lone relaxed atomic increment — none of the build/lock/queue cost the gate exists to skip.
        if (_ContactPairSink != nullptr)
        { _ContactPairSink->Note_ContactPair(); }

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
#if !UE_BUILD_SHIPPING
            _BodyIdToIsSensor.Add(Event.Body1IndexAndSeq, Event.IsSensor1);
            _BodyIdToIsSensor.Add(Event.Body2IndexAndSeq, Event.IsSensor2);
#endif
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

#if !UE_BUILD_SHIPPING
            if (const auto* IsSensor1 = _BodyIdToIsSensor.Find(Event.Body1IndexAndSeq))
            { Event.IsSensor1 = *IsSensor1; }

            if (const auto* IsSensor2 = _BodyIdToIsSensor.Find(Event.Body2IndexAndSeq))
            { Event.IsSensor2 = *IsSensor2; }
#endif

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

    // Populated on Added/Persisted, read on ContactRemoved (which carries no bodies). Entries persist for the
    // listener's lifetime: per-contact removal would break a body's other simultaneous end-overlaps.
    TMap<uint32, uint64> _BodyIdToUserData;
#if !UE_BUILD_SHIPPING
    TMap<uint32, bool> _BodyIdToIsSensor;
#endif

    // Non-owning, and outlives this listener by construction: the subsystem destroys the listener BEFORE the
    // world. Touched from worker threads, but only to bump an atomic on the far side.
    ck::FJoltWorld* _ContactPairSink = nullptr;
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
            TEXT("Color bodies by sleep state rather than by body class. Selects the in-world target's colour "
                 "mode (SleepState vs BodyClass): with it on, static and kinematic bodies fall back to one "
                 "neutral colour each and the only distinction drawn is awake vs asleep."));

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

        static bool DebugDrawContacts = false;
        static FAutoConsoleVariableRef CVar_DebugDrawContacts(TEXT("ck.Jolt.DebugDraw.Contacts"),
            DebugDrawContacts,
            TEXT("When drawing the Jolt world, also draw contact points and manifold normals.\n")
            TEXT("PROCESS-WIDE, unlike every other draw CVar: Jolt's contact draw switches are statics with no "
                 "per-world variant, so this arms emission for every Jolt world and every debugger preview at "
                 "once. Off by default — a dense pile emits contact lines by the hundred thousand."));
    }

#if JPH_DEBUG_RENDERER
    /*
     * The CVars remain the in-world draw's source of truth; the draw flags are how they reach the shared capture
     * code. Everything not listed stays off, matching what the deleted DrawSettings aggregate hard-coded to
     * false. AngularVelocity rides ck.Jolt.DebugDraw.Velocity because Jolt's single mDrawVelocity emitted BOTH
     * the linear and the angular arrow — splitting them into two flags without mapping both would silently drop
     * one of the two lines this CVar has always drawn.
     *
     * SleepColoring is deliberately absent HERE: it selected EShapeColor::SleepColor vs MotionTypeColor, which
     * is a COLOUR question, not a draw-vocabulary one. It drives the target's colour mode instead — see
     * Get_InWorldColorMode.
     */
    auto
        Get_InWorldDrawFlags()
        -> ECk_Jolt_DebugDrawFlags
    {
        auto Flags = ECk_Jolt_DebugDrawFlags::Shape;

        if (cvar::DebugDrawVelocity)
        { Flags |= ECk_Jolt_DebugDrawFlags::Velocity | ECk_Jolt_DebugDrawFlags::AngularVelocity; }

        if (cvar::DebugDrawWorldTransform)
        { Flags |= ECk_Jolt_DebugDrawFlags::WorldTransform; }

        if (cvar::DebugDrawConstraints)
        { Flags |= ECk_Jolt_DebugDrawFlags::Constraints; }

        // Points AND normals together: "contacts" is one question to a user, and Jolt emits the manifold normal
        // from the same solve pass as the point, so splitting them into two CVars would only ever be turned on
        // together (P5-D63 v).
        if (cvar::DebugDrawContacts)
        { Flags |= ECk_Jolt_DebugDrawFlags::ContactPoints | ECk_Jolt_DebugDrawFlags::ContactNormals; }

        return Flags;
    }

    /// The other half of the CVar mapping: ck.Jolt.DebugDraw.SleepColoring is a COLOUR question, so it selects
    /// the in-world target's colour mode rather than a draw flag (P5-D62).
    auto
        Get_InWorldColorMode()
        -> ECk_Jolt_DebugDrawColorMode
    {
        return cvar::DebugDrawSleepColoring
            ? ECk_Jolt_DebugDrawColorMode::SleepState
            : ECk_Jolt_DebugDrawColorMode::BodyClass;
    }
#endif

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

    // Jolt's stock combine is max(r1, r2); Chaos reads UPhysicsSettingsCore::RestitutionCombineMode,
    // which is Average in a default UE project. Symmetric pairs agree either way, so the divergence is
    // invisible until two surfaces disagree -- and there it is total: a restitution-1.0 bouncy material
    // landing on a default-0.3 floor keeps 100% of its normal velocity under max() (a perfectly elastic
    // collision -- it never stops bouncing, and it hops straight back off any surface it is set down on)
    // against 65% under Average. A physical material carries its numbers over from Chaos unchanged, so
    // the mode has to come over too; the project setting defaults to Average for exactly that reason.
    //
    // Friction is deliberately LEFT on Jolt's sqrt(f1 * f2). It diverges from Chaos's average on the
    // same asymmetric pairs, but the geometric and arithmetic means sit within a few percent over the
    // range real materials use, which does not justify re-tuning every contact in the game.
    const auto RestitutionCombineMode = UCk_Utils_Jolt_ProjectSettings::Get_RestitutionCombineMode();
    ck::jolt::Log(TEXT("Jolt: RestitutionCombineMode [{}]"), RestitutionCombineMode);

    switch (RestitutionCombineMode)
    {
        case ECk_Jolt_RestitutionCombineMode::Average:
        {
            _PhysicsSystem->SetCombineRestitution(
                [](const Body& InBodyA, const SubShapeID&, const Body& InBodyB, const SubShapeID&)
                { return 0.5f * (InBodyA.GetRestitution() + InBodyB.GetRestitution()); });
            break;
        }
        case ECk_Jolt_RestitutionCombineMode::Min:
        {
            _PhysicsSystem->SetCombineRestitution(
                [](const Body& InBodyA, const SubShapeID&, const Body& InBodyB, const SubShapeID&)
                { return FMath::Min(InBodyA.GetRestitution(), InBodyB.GetRestitution()); });
            break;
        }
        case ECk_Jolt_RestitutionCombineMode::Multiply:
        {
            _PhysicsSystem->SetCombineRestitution(
                [](const Body& InBodyA, const SubShapeID&, const Body& InBodyB, const SubShapeID&)
                { return InBodyA.GetRestitution() * InBodyB.GetRestitution(); });
            break;
        }
        case ECk_Jolt_RestitutionCombineMode::Max:
        {
            _PhysicsSystem->SetCombineRestitution(
                [](const Body& InBodyA, const SubShapeID&, const Body& InBodyB, const SubShapeID&)
                { return FMath::Max(InBodyA.GetRestitution(), InBodyB.GetRestitution()); });
            break;
        }
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
        .LayerTable = _LayerTable.Get(),
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

    // The world is built after the listener, so the counter it feeds can only be wired here. Cleared in
    // Deinitialize is unnecessary — the listener is destroyed first.
    _ContactListener->Set_ContactPairSink(_JoltWorld.Get());

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
        const auto ConsumerGateOpen = _DebugDrawGate && _DebugDrawGate();
        const auto CVarDrawEnabled  = ck_jolt_subsystem::cvar::DebugDrawEnabled;
        const auto IsSuppressedForStreamerMode = ck::diagnostic_visibility::Is_HiddenForStreamerMode();

        if (_DefaultDebugDrawTarget.IsValid())
        {
            if (NOT IsSuppressedForStreamerMode && (ConsumerGateOpen || CVarDrawEnabled))
            {
                auto& Renderer = FCk_Jolt_DebugRenderer::Get_OrCreate();

                // The capture processor pushes these too, but it early-outs whenever no registered target is
                // demanding — which is exactly the case where the in-world draw is the only thing capturing, and
                // the drag anchor still has to be invisible in it.
                _DefaultDebugDrawTarget->Set_InternalBodyKeys(_JoltWorld->Get_DebugInternalBodyKeys());
#if !UE_BUILD_SHIPPING
                _DefaultDebugDrawTarget->Set_SensorContactBodyKeys(_JoltWorld->Get_SensorContactBodyKeys());
#endif
                _DefaultDebugDrawTarget->Set_StepStats(_JoltWorld->Get_LastStepDurationMs(),
                    _JoltWorld->Get_ContactPairsLastStep());

                _DefaultDebugDrawTarget->Set_Opacity(ck_jolt_subsystem::cvar::DebugDrawOpacity);
                _DefaultDebugDrawTarget->Set_DrawFlags(ck_jolt_subsystem::Get_InWorldDrawFlags());
                _DefaultDebugDrawTarget->Set_ColorMode(ck_jolt_subsystem::Get_InWorldColorMode());

                const auto Revisions = ck::jolt::debug_draw::FCaptureRevisions{
                    _JoltWorld->Get_StaticSceneRevision(),
                    _JoltWorld->Get_BodyRemovedRevision()};

                // The same transient entity the capture processor passes: it is what resolves a baked static's
                // attribution entity and what the character pass iterates. Absent outside a live ECS world,
                // which the capture degrades on gracefully.
                const auto TransientEntity = ck::IsValid(_EcsWorldSubsystem)
                    ? _EcsWorldSubsystem->Get_TransientEntity()
                    : FCk_Handle{};

                Renderer.Capture_JoltWorld(*_DefaultDebugDrawTarget, *_PhysicsSystem, Revisions, TransientEntity);
                Renderer.NextFrame();
            }
            else
            {
                // Flags first: the contact ones drive Jolt's PROCESS-WIDE statics, so an in-world draw that has
                // been switched off must stop declaring a contact demand or every world keeps recording
                // manifolds nothing will ever draw.
                _DefaultDebugDrawTarget->Set_DrawFlags(ECk_Jolt_DebugDrawFlags::Shape);

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

auto
    UCk_Jolt_Subsystem::
    Get_DefaultDebugDrawTarget() const
        -> TSharedPtr<FCk_Jolt_DebugDrawTarget>
{
    return _DefaultDebugDrawTarget;
}
#endif

auto
    UCk_Jolt_Subsystem::
    Request_SetDebugPaused(
        bool InIsDebugPaused)
        -> void
{
    if (NOT _JoltWorld.IsValid())
    { return; }

    _JoltWorld->Request_SetDebugPaused(InIsDebugPaused);
}

auto
    UCk_Jolt_Subsystem::
    Request_StepOnce()
        -> void
{
    if (NOT _JoltWorld.IsValid())
    { return; }

    _JoltWorld->Request_StepOnce();
}

auto
    UCk_Jolt_Subsystem::
    Get_IsDebugPaused() const
        -> bool
{
    if (NOT _JoltWorld.IsValid())
    { return false; }

    return _JoltWorld->Get_IsDebugPaused();
}

auto
    UCk_Jolt_Subsystem::
    Get_LastStepDurationMs() const
        -> float
{
    if (NOT _JoltWorld.IsValid())
    { return 0.0f; }

    return _JoltWorld->Get_LastStepDurationMs();
}

auto
    UCk_Jolt_Subsystem::
    Get_ContactPairsLastStep() const
        -> int32
{
    if (NOT _JoltWorld.IsValid())
    { return 0; }

    return _JoltWorld->Get_ContactPairsLastStep();
}

#if !UE_BUILD_SHIPPING

auto
    UCk_Jolt_Subsystem::
    Request_BeginDrag(
        uint64 InBodyKey,
        const FVector& InWorldGrabPoint)
        -> void
{
    if (NOT _JoltWorld.IsValid())
    { return; }

    _JoltWorld->Request_BeginDrag(InBodyKey, InWorldGrabPoint);
}

auto
    UCk_Jolt_Subsystem::
    Request_UpdateDrag(
        const FVector& InWorldTargetPoint)
        -> void
{
    if (NOT _JoltWorld.IsValid())
    { return; }

    _JoltWorld->Request_UpdateDrag(InWorldTargetPoint);
}

auto
    UCk_Jolt_Subsystem::
    Request_EndDrag()
        -> void
{
    if (NOT _JoltWorld.IsValid())
    { return; }

    _JoltWorld->Request_EndDrag();
}

auto
    UCk_Jolt_Subsystem::
    Get_IsDragging() const
        -> bool
{
    if (NOT _JoltWorld.IsValid())
    { return false; }

    return _JoltWorld->Get_IsDragging();
}

auto
    UCk_Jolt_Subsystem::
    Get_DragState() const
        -> TOptional<ck::jolt::FCk_Jolt_DebugDragState>
{
    if (NOT _JoltWorld.IsValid())
    { return {}; }

    return _JoltWorld->Get_DragState();
}

#endif

auto
    UCk_Jolt_Subsystem::
    Get_DebugInternalBodyKeys() const
        -> const TSet<uint64>&
{
    if (NOT _JoltWorld.IsValid())
    {
        static const auto Empty = TSet<uint64>{};
        return Empty;
    }

    return _JoltWorld->Get_DebugInternalBodyKeys();
}

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
