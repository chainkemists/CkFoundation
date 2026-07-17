#include "CkJolt_Subsystem.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Debug/CkDebugDraw_Utils.h"

#include "CkEcs/Registry/CkRegistry.h"
#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"

#include "CkJolt/CkJolt_Log.h"
#include "CkJolt/CkJolt_Stats.h"
#include "CkJolt/CkJolt_Utils.h"
#include "CkJolt/Settings/CkJolt_ProjectSettings.h"

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
#include <Jolt/Renderer/DebugRendererSimple.h>

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("Jolt_Subsystem_Tick"), STAT_CkJolt_SubsystemTick, STATGROUP_CkJolt);

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
    }

    auto
        OnBodyDeactivated(
            const JPH::BodyID& inBodyID,
            uint64 inBodyUserData)
            -> void override
    {
        ck::jolt::Verbose(TEXT("Body [{}] just DE-ACTIVATED"), inBodyID.GetIndex());
    }
};

// --------------------------------------------------------------------------------------------------------------------

class CkJoltDebugger : public JPH::DebugRendererSimple
{
    auto
    DrawLine(
        JPH::RVec3Arg inFrom,
        JPH::RVec3Arg inTo,
        JPH::ColorArg inColor)
    -> void override
    {
        if (ck::Is_NOT_Valid(_World))
        { return; }

        UCk_Utils_DebugDraw_UE::DrawDebugLine(_World.Get(), ck::jolt::Conv(inFrom), ck::jolt::Conv(inTo),
            ck::jolt::Conv(inColor));
    }

    auto
    DrawText3D(
        JPH::RVec3Arg inPosition,
        const JPH::string_view& inString,
        JPH::ColorArg inColor = JPH::Color::sWhite,
        float inHeight = 0.5f)
    -> void
    {
        if (ck::Is_NOT_Valid(_World))
        { return; }

        UCk_Utils_DebugDraw_UE::DrawDebugString(_World.Get(), ck::jolt::Conv(inPosition),
            FString{static_cast<int32>(inString.length()), inString.data()}, ck::jolt::Conv(inColor));
    }

public:
    TWeakObjectPtr<UWorld> _World;
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
    });

    _EcsWorldSubsystem->Get_Registry().SetContext<TSharedPtr<ck::FJoltWorld>>(_JoltWorld);

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
        constexpr auto DrawShapeWireframe = true;
        constexpr auto DrawShapeColor = JPH::BodyManager::EShapeColor::MotionTypeColor;
        constexpr auto DrawBoundingBox = false;
        constexpr auto DrawCenterOfMassTransform = false;
        constexpr auto DrawWorldTransform = true;
        constexpr auto DrawVelocity = true;
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

        constexpr auto DrawSettings = JPH::BodyManager::DrawSettings
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

        if (ck::IsValid(_Debugger, ck::IsValid_Policy_NullptrOnly{}) &&
            _DebugDrawGate && _DebugDrawGate())
        { _PhysicsSystem->DrawBodies(DrawSettings, _Debugger.Get()); }
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
