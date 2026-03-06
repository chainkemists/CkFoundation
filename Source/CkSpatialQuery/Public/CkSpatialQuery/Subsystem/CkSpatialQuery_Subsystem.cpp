#include "CkSpatialQuery_Subsystem.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Debug/CkDebugDraw_Utils.h"

#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"

#include "CkSpatialQuery/CkSpatialQuery_Log.h"
#include "CkSpatialQuery/CkSpatialQuery_Utils.h"
#include "CkSpatialQuery/Probe/CkProbe_Fragment.h"
#include "CkSpatialQuery/Probe/CkProbe_Utils.h"
#include "CkSpatialQuery/Settings/CkSpatialQuery_Settings.h"
#include "CkSpatialQuery/Settings/CkSpatialQuery_ProjectSettings.h"

#include <Async/Async.h>
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

namespace object_layers
{
    static constexpr JPH::ObjectLayer Non_Moving = 0;
    static constexpr JPH::ObjectLayer Moving = 1;
    static constexpr JPH::ObjectLayer Num_Layers = 2;
};

namespace broad_phase_layers
{
    static constexpr JPH::BroadPhaseLayer Non_Moving(0);
    static constexpr JPH::BroadPhaseLayer Moving(1);
    static constexpr JPH::uint Num_Layers(2);
};

namespace contact_surface
{
    auto Get_ContactPhysicalMaterial(const FCk_Handle_Probe& InProbe) -> UPhysicalMaterial*
    {
        const auto& SurfaceInfo = UCk_Utils_Probe_UE::Get_SurfaceInfo(InProbe);

        switch (const auto& PhysicalMaterialSource = SurfaceInfo.Get_PhysicalMaterialSource())
        {
            case ECk_PhysicalMaterialSource::Direct:
            {
                return SurfaceInfo.Get_PhysicalMaterial();
            }
            case ECk_PhysicalMaterialSource::Trace:
            {
                CK_TRIGGER_ENSURE(TEXT("Probe Physical Material Source [{}] is NOT supported yet"), PhysicalMaterialSource);
                return {};
            }
            default:
            {
                CK_INVALID_ENUM(PhysicalMaterialSource);
                return {};
            }
        }
    };
}

class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface
{
public:
    BPLayerInterfaceImpl()
    {
        // Create a mapping table from object to broad phase layer
        _ObjectToBroadPhase[object_layers::Non_Moving] = broad_phase_layers::Non_Moving;
        _ObjectToBroadPhase[object_layers::Moving] = broad_phase_layers::Moving;
    }

    auto
        GetNumBroadPhaseLayers() const
            -> JPH::uint override
    {
        return broad_phase_layers::Num_Layers;
    }

    auto
        GetBroadPhaseLayer(
            JPH::ObjectLayer inLayer) const
            -> JPH::BroadPhaseLayer override
    {
        JPH_ASSERT(inLayer < broad_phase_layers::Num_Layers);
        return _ObjectToBroadPhase[inLayer];
    }

private:
    JPH::BroadPhaseLayer _ObjectToBroadPhase[object_layers::Num_Layers];
};

// --------------------------------------------------------------------------------------------------------------------

class ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter
{
public:
    auto
        ShouldCollide(
            const JPH::ObjectLayer inLayer1,
            const JPH::BroadPhaseLayer inLayer2) const
            -> bool override
    {
        switch (inLayer1)
        {
            case object_layers::Non_Moving: return inLayer2 == broad_phase_layers::Moving;
            case object_layers::Moving: return true;
            default:
                JPH_ASSERT(false);
                return false;
        }
    }
};

// --------------------------------------------------------------------------------------------------------------------

class CkObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter
{
public:
    virtual bool
        ShouldCollide(
            const JPH::ObjectLayer inObject1,
            const JPH::ObjectLayer inObject2) const override
    {
        switch (inObject1)
        {
            case object_layers::Non_Moving: return inObject2 == object_layers::Moving;
            case object_layers::Moving: return true;
            default:
                JPH_ASSERT(false);
                return false;
        }
    }
};

// --------------------------------------------------------------------------------------------------------------------

// Contact event data captured during Jolt callbacks for deferred processing on the game thread.
// This is necessary because Jolt's JobSystemThreadPool fires contact callbacks from worker threads,
// and ECS mutations are not thread-safe.
struct FCk_ContactEvent
{
    enum class EType : uint8 { Added, Persisted, Removed };

    EType Type;

    // Entity identification (captured from body UserData during callback)
    uint64 Body1UserData;
    uint64 Body2UserData;

    // Contact geometry (only meaningful for Added/Persisted)
    TArray<FVector> ContactPointsOn1;
    TArray<FVector> ContactPointsOn2;
    FVector WorldSpaceNormal;

    // Body ID index+sequence for BodyIdToUserData map management (only for Added/Removed)
    uint32 Body1IndexAndSeq;
    uint32 Body2IndexAndSeq;
};

// Thread-safe contact listener that queues events for deferred processing.
// All ECS mutations happen on the game thread via ProcessQueuedContacts().
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
        ck::spatialquery::VeryVerbose(TEXT("Body [{}] and Body [{}] and SUB-SHAPE [{}] and SUB-SHAPE [{}] NEW Contact"),
            inBody1.GetID().GetIndex(), inBody2.GetID().GetIndex(),
            inManifold.mSubShapeID1.GetValue(), inManifold.mSubShapeID2.GetValue());

        auto Event = FCk_ContactEvent{};
        Event.Type = FCk_ContactEvent::EType::Added;
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
        ck::spatialquery::VeryVerbose(TEXT("Body [{}] and Body [{}] and SUB-SHAPE [{}] and SUB-SHAPE [{}] PERSISTED Contact"),
            inBody1.GetID().GetIndex(), inBody2.GetID().GetIndex(),
            inManifold.mSubShapeID1.GetValue(), inManifold.mSubShapeID2.GetValue());

        auto Event = FCk_ContactEvent{};
        Event.Type = FCk_ContactEvent::EType::Persisted;
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
        ck::spatialquery::VeryVerbose(TEXT("Body [{}] and Body [{}] and SUB-SHAPE [{}] and SUB-SHAPE [{}] REMOVED Contact"),
            inSubShapePair.GetBody1ID().GetIndex(), inSubShapePair.GetBody2ID().GetIndex(),
            inSubShapePair.GetSubShapeID1().GetValue(), inSubShapePair.GetSubShapeID2().GetValue());

        auto Event = FCk_ContactEvent{};
        Event.Type = FCk_ContactEvent::EType::Removed;
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
    auto DrainQueue(TArray<FCk_ContactEvent>& OutEvents) -> void
    {
        FScopeLock Lock(&_QueueLock);
        OutEvents = MoveTemp(_ContactEventQueue);
        _ContactEventQueue.Reset();
    }

    // Remove body ID entries from the lookup map. Called from game thread only.
    auto RemoveBodyMapping(uint32 InBodyIndexAndSeq) -> void
    {
        FScopeLock Lock(&_QueueLock);
        _BodyIdToUserData.Remove(InBodyIndexAndSeq);
    }

private:
    FCriticalSection _QueueLock;
    TArray<FCk_ContactEvent> _ContactEventQueue;

    // Maps BodyID (index+sequence) -> UserData (entity ID).
    // Populated during OnContactAdded, read during OnContactRemoved,
    // cleaned up during ProcessQueuedContacts. Protected by _QueueLock.
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
        ck::spatialquery::Verbose(TEXT("Body [{}] just ACTIVATED"), inBodyID.GetIndex());
    }

    auto
        OnBodyDeactivated(
            const JPH::BodyID& inBodyID,
            uint64 inBodyUserData)
            -> void override
    {
        ck::spatialquery::Verbose(TEXT("Body [{}] just DE-ACTIVATED"), inBodyID.GetIndex());
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

namespace ck_spatialquery_subsystem
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
            ck::spatialquery::Log(TEXT("Jolt: [{}] overridden by {} to [{}]"), InSettingName, Source, bOverrideValue);
            return bOverrideValue;
        }
        return InProjectSettingValue;
    }

    // Reference count for global Jolt initialization (RegisterDefaultAllocator, Factory, RegisterTypes).
    // These are process-global and must only be called once, but multiple world subsystem instances
    // may Initialize/Deinitialize (e.g. PIE with multiple clients).
    static int32 GJoltRefCount = 0;

    auto
        CustomTraceFunction(
            const char* inFMT,
            ...)
        -> void
    {
        va_list List;
        va_start(List, inFMT);
        char Buffer[1024];
        vsnprintf(Buffer, sizeof(Buffer), inFMT, List);
        va_end(List);

        ck::spatialquery::Verbose(TEXT("Jolt Trace: [{}]"), FString{Buffer});
    }

    auto
        CustomAssertFunction(
            const char* inExpression,
            const char* inMessage,
            const char* inFile,
            JPH::uint inLine)
        -> bool
    {
        CK_TRIGGER_ENSURE(TEXT("Jolt FAILED [{}] with Message [{}].\n{}:{}"), FString{inExpression}, FString{inMessage},
            FString{inFile}, inLine);
        return false;
    }
}

auto
    UCk_SpatialQuery_Subsystem::
    Initialize(
        FSubsystemCollectionBase& InCollection)
        -> void
{
    Super::Initialize(InCollection);
    _EcsWorldSubsystem = InCollection.InitializeDependency<UCk_EcsWorld_Subsystem_UE>();

    using namespace JPH;

    if (ck_spatialquery_subsystem::GJoltRefCount++ == 0)
    {
        RegisterDefaultAllocator();
        Factory::sInstance = new Factory{};
        RegisterTypes();

        JPH::Trace = ck_spatialquery_subsystem::CustomTraceFunction;
        JPH::AssertFailed = ck_spatialquery_subsystem::CustomAssertFunction;
    }

    const auto MaxBodies = static_cast<uint>(UCk_Utils_SpatialQuery_ProjectSettings::Get_MaxBodies());
    constexpr uint NumBodyMutexes = 0;
    const auto MaxBodyPairs = static_cast<uint>(UCk_Utils_SpatialQuery_ProjectSettings::Get_MaxBodyPairs());
    const auto MaxContactConstraints = static_cast<uint>(UCk_Utils_SpatialQuery_ProjectSettings::Get_MaxContactConstraints());

    const auto MaxPhysicsJobs = UCk_Utils_SpatialQuery_ProjectSettings::Get_MaxPhysicsJobs();
    const auto MaxPhysicsBarriers = UCk_Utils_SpatialQuery_ProjectSettings::Get_MaxPhysicsBarriers();
    const auto TempAllocatorSizeBytes = UCk_Utils_SpatialQuery_ProjectSettings::Get_TempAllocatorSizeMB() * 1024u * 1024u;

    _CollisionSteps = UCk_Utils_SpatialQuery_ProjectSettings::Get_CollisionSteps();

    _TempAllocator = MakePimpl<TempAllocatorImpl>(TempAllocatorSizeBytes);

    const bool bEnableParallel = ck_spatialquery_subsystem::ResolveCVarOverride(
        TEXT("jolt.EnableParallelPhysics"),
        CVarJoltEnableParallelPhysics.GetValueOnGameThread(),
        UCk_Utils_SpatialQuery_ProjectSettings::Get_EnableParallelPhysics(),
        TEXT("EnableParallelPhysics"));

    _ParallelPhysicsEnabled = bEnableParallel;

    if (_ParallelPhysicsEnabled)
    {
        auto NumThreads = UCk_Utils_SpatialQuery_ProjectSettings::Get_NumPhysicsThreads();
        if (NumThreads <= 0)
        {
            NumThreads = FMath::Max(1, static_cast<int32>(std::thread::hardware_concurrency()) - 1);
        }

        _PhysicsThreadCount = NumThreads;

        ck::spatialquery::Log(TEXT("Jolt: Creating JobSystemThreadPool with [{}] threads"), _PhysicsThreadCount);

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
        ck::spatialquery::Log(TEXT("Jolt: Creating JobSystemSingleThreaded"));
        _JobSystem = new JPH::JobSystemSingleThreaded(MaxPhysicsJobs);
    }

    _BroadPhaseLayerInterface = MakePimpl<BPLayerInterfaceImpl>();
    _ObjectVsBroadPhaseLayerFilter = MakePimpl<ObjectVsBroadPhaseLayerFilterImpl>();
    _ObjectVsObjectFilter = MakePimpl<CkObjectLayerPairFilterImpl>();

    _PhysicsSystem = MakeShared<PhysicsSystem>();
    _PhysicsSystem->Init(MaxBodies, NumBodyMutexes, MaxBodyPairs, MaxContactConstraints, *_BroadPhaseLayerInterface,
        *_ObjectVsBroadPhaseLayerFilter, *_ObjectVsObjectFilter);

    _BodyActivationListener = MakePimpl<CkBodyActivationListener>();
    _PhysicsSystem->SetBodyActivationListener(_BodyActivationListener.Get());

    _ContactListener = MakePimpl<CkContactListener>();
    _PhysicsSystem->SetContactListener(&*_ContactListener);

    _AsyncPhysicsUpdate = ck_spatialquery_subsystem::ResolveCVarOverride(
        TEXT("jolt.EnableAsyncPhysicsUpdate"),
        CVarJoltEnableAsyncPhysicsUpdate.GetValueOnGameThread(),
        UCk_Utils_SpatialQuery_ProjectSettings::Get_EnableAsyncPhysicsUpdate(),
        TEXT("EnableAsyncPhysicsUpdate"));

    if (_AsyncPhysicsUpdate)
    {
        ck::spatialquery::Log(TEXT("Jolt: Async physics update ENABLED (one-frame latent)"));
    }

#if JPH_DEBUG_RENDERER
    if (ck::Is_NOT_Valid(JPH::DebugRenderer::sInstance, ck::IsValid_Policy_NullptrOnly{}))
    {
        _Debugger = MakePimpl<CkJoltDebugger>();
        _Debugger->_World = GetWorld();
    }
#endif
}

auto
    UCk_SpatialQuery_Subsystem::
    Tick(
        float InDeltaTime)
        -> void
{
    QUICK_SCOPE_CYCLE_COUNTER(SpatialQuery_Subsystem_Tick);
    Super::Tick(InDeltaTime);

    // Always consume any in-flight async result first (even if paused).
    // This ensures the previous frame's physics is complete before we process contacts.
    if (_PhysicsAsyncFuture.IsValid())
    {
        QUICK_SCOPE_CYCLE_COUNTER(JoltPhysics_WaitForAsync);
        _PhysicsAsyncFuture.Wait();
        _PhysicsAsyncFuture = {};
    }

    // Process contacts from the completed physics update.
    // In async mode, these are from the previous frame's Update().
    // In sync mode, these are from the current frame (processed right after Update below).
    {
        QUICK_SCOPE_CYCLE_COUNTER(JoltPhysics_ProcessQueuedContacts);
        ProcessQueuedContacts();
    }

    if (GetWorld()->IsPaused())
    { return; }

    // Run physics update — either async (off game thread) or sync (blocking).
    if (_AsyncPhysicsUpdate)
    {
        _PhysicsAsyncFuture = Async(EAsyncExecution::TaskGraph,
            [this, DeltaTime = InDeltaTime]()
            {
                QUICK_SCOPE_CYCLE_COUNTER(JoltPhysics_Update_Async);
                _PhysicsSystem->Update(DeltaTime, _CollisionSteps, &*_TempAllocator, _JobSystem);
            });
    }
    else
    {
        QUICK_SCOPE_CYCLE_COUNTER(JoltPhysics_Update);
        _PhysicsSystem->Update(InDeltaTime, _CollisionSteps, &*_TempAllocator, _JobSystem);
    }

#if JPH_DEBUG_RENDERER
    // Debug rendering requires physics state to be stable — only valid after sync update.
    // In async mode, the Update is in-flight so we skip debug draw (results arrive next frame).
    if (NOT _AsyncPhysicsUpdate)
    {
        // Named constants for clear initialization
        constexpr auto DrawGetSupportFeatures = false;
        constexpr auto DrawSupportDirection = false;
        constexpr auto DrawGetSupportingFace = false;
        constexpr auto DrawShape = true;
        constexpr auto DrawShapeWireframe = true;
        constexpr auto DrawShapeColor = JPH::BodyManager::EShapeColor::MotionTypeColor;
        constexpr auto DrawBoundingBox = true;
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
            UCk_Utils_SpatialQuery_Settings::Get_DebugPreviewAllProbesUsingJolt())
        { _PhysicsSystem->DrawBodies(DrawSettings, _Debugger.Get()); }
    }
#endif
}

// Process contact events that were queued during PhysicsSystem::Update().
// This runs on the game thread after Update() returns, so ECS mutations are safe.
auto
    UCk_SpatialQuery_Subsystem::
    ProcessQueuedContacts()
        -> void
{
    auto Events = TArray<FCk_ContactEvent>{};

    {
        QUICK_SCOPE_CYCLE_COUNTER(JoltContacts_DrainQueue);
        _ContactListener->DrainQueue(Events);
    }

    if (Events.IsEmpty())
    { return; }

    const auto TransientEntity = _EcsWorldSubsystem->Get_TransientEntity();

    int32 AddedCount = 0;
    int32 PersistedCount = 0;
    int32 RemovedCount = 0;

    for (const auto& Event : Events)
    {
        switch (Event.Type)
        {
            case FCk_ContactEvent::EType::Added:
            {
                QUICK_SCOPE_CYCLE_COUNTER(JoltContacts_Added);
                ++AddedCount;

                auto Body1Entity = TransientEntity.Get_ValidHandle(
                    FCk_Entity::IdType{static_cast<FCk_Entity::IdType>(Event.Body1UserData)});
                auto Body2Entity = TransientEntity.Get_ValidHandle(
                    FCk_Entity::IdType{static_cast<FCk_Entity::IdType>(Event.Body2UserData)});

                auto Body1 = UCk_Utils_Probe_UE::Cast(Body1Entity);
                auto Body2 = UCk_Utils_Probe_UE::Cast(Body2Entity);

                if (ck::IsValid(Body1) && UCk_Utils_Probe_UE::Get_CanOverlapWith(Body1, Body2))
                {
                    UCk_Utils_Probe_UE::Request_BeginOverlap(Body1,
                        FCk_Request_Probe_BeginOverlap{
                            Body2,
                            Event.ContactPointsOn1,
                            -Event.WorldSpaceNormal,
                            contact_surface::Get_ContactPhysicalMaterial(Body2)
                        });
                }

                if (ck::IsValid(Body2) && UCk_Utils_Probe_UE::Get_CanOverlapWith(Body2, Body1))
                {
                    UCk_Utils_Probe_UE::Request_BeginOverlap(Body2,
                        FCk_Request_Probe_BeginOverlap{
                            Body1,
                            Event.ContactPointsOn2,
                            Event.WorldSpaceNormal,
                            contact_surface::Get_ContactPhysicalMaterial(Body1)
                        });
                }
                break;
            }

            case FCk_ContactEvent::EType::Persisted:
            {
                QUICK_SCOPE_CYCLE_COUNTER(JoltContacts_Persisted);
                ++PersistedCount;

                auto Body1Entity = TransientEntity.Get_ValidHandle(
                    FCk_Entity::IdType{static_cast<FCk_Entity::IdType>(Event.Body1UserData)});
                auto Body2Entity = TransientEntity.Get_ValidHandle(
                    FCk_Entity::IdType{static_cast<FCk_Entity::IdType>(Event.Body2UserData)});

                auto Body1 = UCk_Utils_Probe_UE::Cast(Body1Entity);
                auto Body2 = UCk_Utils_Probe_UE::Cast(Body2Entity);

                if (ck::IsValid(Body1) && UCk_Utils_Probe_UE::Get_CanOverlapWith(Body1, Body2))
                {
                    UCk_Utils_Probe_UE::Request_OverlapUpdated(Body1,
                        FCk_Request_Probe_OverlapUpdated{
                            Body2,
                            Event.ContactPointsOn1,
                            -Event.WorldSpaceNormal,
                            contact_surface::Get_ContactPhysicalMaterial(Body2)
                        });
                }

                if (ck::IsValid(Body2) && UCk_Utils_Probe_UE::Get_CanOverlapWith(Body2, Body1))
                {
                    UCk_Utils_Probe_UE::Request_OverlapUpdated(Body2,
                        FCk_Request_Probe_OverlapUpdated{
                            Body1,
                            Event.ContactPointsOn2,
                            Event.WorldSpaceNormal,
                            contact_surface::Get_ContactPhysicalMaterial(Body1)
                        });
                }
                break;
            }

            case FCk_ContactEvent::EType::Removed:
            {
                QUICK_SCOPE_CYCLE_COUNTER(JoltContacts_Removed);
                ++RemovedCount;

                auto Body1Entity = TransientEntity.Get_ValidHandle(
                    FCk_Entity::IdType{static_cast<FCk_Entity::IdType>(Event.Body1UserData)});
                auto Body2Entity = TransientEntity.Get_ValidHandle(
                    FCk_Entity::IdType{static_cast<FCk_Entity::IdType>(Event.Body2UserData)});

                auto Body1 = UCk_Utils_Probe_UE::Cast(Body1Entity);
                auto Body2 = UCk_Utils_Probe_UE::Cast(Body2Entity);

                if (ck::IsValid(Body1) && UCk_Utils_Probe_UE::Get_CanOverlapWith(Body1, Body2))
                {
                    UCk_Utils_Probe_UE::Request_EndOverlap(Body1, FCk_Request_Probe_EndOverlap{Body2});
                }

                if (ck::IsValid(Body2) && UCk_Utils_Probe_UE::Get_CanOverlapWith(Body2, Body1))
                {
                    UCk_Utils_Probe_UE::Request_EndOverlap(Body2, FCk_Request_Probe_EndOverlap{Body1});
                }

                // Clean up body-to-entity mapping for removed contacts
                _ContactListener->RemoveBodyMapping(Event.Body1IndexAndSeq);
                _ContactListener->RemoveBodyMapping(Event.Body2IndexAndSeq);
                break;
            }
        }
    }

    ck::spatialquery::VeryVerbose(TEXT("ProcessQueuedContacts: [{}] events (Added: [{}], Persisted: [{}], Removed: [{}])"),
        Events.Num(), AddedCount, PersistedCount, RemovedCount);
}

auto
    UCk_SpatialQuery_Subsystem::
    Deinitialize()
        -> void
{
    // Wait for any in-flight async physics before destroying the system
    if (_PhysicsAsyncFuture.IsValid())
    {
        _PhysicsAsyncFuture.Wait();
        _PhysicsAsyncFuture = {};
    }

    _ContactListener.Reset();
    _BodyActivationListener.Reset();
    _PhysicsSystem.Reset();
    _ObjectVsObjectFilter.Reset();
    _ObjectVsBroadPhaseLayerFilter.Reset();
    _BroadPhaseLayerInterface.Reset();
    delete _JobSystem;
    _JobSystem = nullptr;
    _TempAllocator.Reset();

    if (--ck_spatialquery_subsystem::GJoltRefCount == 0)
    {
        JPH::UnregisterTypes();
        delete JPH::Factory::sInstance;
        JPH::Factory::sInstance = nullptr;
    }

    Super::Deinitialize();
}

auto
    UCk_SpatialQuery_Subsystem::
    Get_PhysicsSystem() const
        -> TWeakPtr<JPH::PhysicsSystem>
{
    return _PhysicsSystem;
}

// --------------------------------------------------------------------------------------------------------------------
