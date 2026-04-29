#include "CkNav_Subsystem.h"

#include "CkNavigation/CkNavigation_Log.h"
#include "CkNavigation/Nav/CkNav_Fragment.h"
#include "CkNavigation/Settings/CkNav_ProjectSettings.h"

#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/Handle/CkHandle.h"

#include <Engine/World.h>
#include <NavigationSystem.h>
#include <NavMesh/RecastNavMesh.h>
#include <TimerManager.h>

#include "DetourCrowd/DetourCrowd.h"
#include "DetourCrowd/DetourObstacleAvoidance.h"

// --------------------------------------------------------------------------------------------------------------------

void
    FCk_DtCrowd_Deleter::
    operator()(dtCrowd* InCrowd) const
{
    if (InCrowd != nullptr)
    { dtFreeCrowd(InCrowd); }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck_nav_subsystem
{
    auto Resolve_DetourNavMesh(ARecastNavMesh* InNavData) -> dtNavMesh*
    {
        // VERIFIED A1: ARecastNavMesh::GetRecastMesh() (NavMesh/RecastNavMesh.h:1296), NAVIGATIONSYSTEM_API.
        return ck::IsValid(InNavData, ck::IsValid_Policy_NullptrOnly{}) ? InNavData->GetRecastMesh() : nullptr;
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Navigation_Subsystem::
    Initialize(
        FSubsystemCollectionBase& InCollection)
        -> void
{
    Super::Initialize(InCollection);

    _EcsWorldSubsystem = InCollection.InitializeDependency<UCk_EcsWorld_Subsystem_UE>();

    // P3-2 sentinel: warn-once if budget is disabled (zero is a footgun, NOT an "unbounded" mode).
    // Independent of nav-data availability — fire it at init.
    if (UCk_Utils_Nav_ProjectSettings::Get_MaxPathQueriesPerFrame() == 0)
    {
        ck::nav::Warning(TEXT("CkNavigation: _MaxPathQueriesPerFrame is 0 (DISABLED). No path queries will be processed. "
                              "Set to a positive value (default 8) in Project Settings -> Navigation."));
    }

    // NavSys discovery + delegate binding happens in Tick (deferred). World-subsystem
    // Initialize runs BEFORE UNavigationSystemV1 is created in the world, so trying to
    // resolve NavSys here always misses on PIE startup. The Tick poll latches once
    // NavSys exists and the delegate is registered.
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Navigation_Subsystem::
    Tick(
        float InDeltaSeconds)
    -> void
{
    Super::Tick(InDeltaSeconds);

    if (NOT _NavSystemBound)
    { DoTryBindNavSystem(); }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Navigation_Subsystem::
    DoTryBindNavSystem()
    -> void
{
    auto* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());

    if (ck::Is_NOT_Valid(NavSys, ck::IsValid_Policy_NullptrOnly{}))
    { return; }   // World still warming up — try again next tick.

    // Bind the regen delegate FIRST so we never miss a regen fire, even if NavData
    // isn't ready yet at this point.
    NavSys->OnNavigationGenerationFinishedDelegate.AddDynamic(
        this, &UCk_Navigation_Subsystem::HandleNavmeshRegenerated);
    _NavSystemBound = true;

    ck::nav::Warning(TEXT("CkNavigation: bound OnNavigationGenerationFinishedDelegate (NavSys is now available)."));

    // Fast path — if NavData is already up, adopt it immediately and allocate Crowd
    // without waiting for the next regen fire. (Common for already-baked levels.)
    auto* NavData = Cast<ARecastNavMesh>(NavSys->GetDefaultNavDataInstance());

    if (ck::Is_NOT_Valid(NavData, ck::IsValid_Policy_NullptrOnly{}))
    {
        ck::nav::Warning(TEXT("CkNavigation: NavSys ready but no ARecastNavMesh yet; awaiting regen fire."));
        return;
    }

    ck::nav::Warning(TEXT("CkNavigation: NavData also ready at bind time; taking fast path to alloc Crowd."));
    _NavMesh = NavData;

    // P3-5: warn loudly on multi-navmesh projects. v1 binds to default only.
    if (NavSys->NavDataSet.Num() > 1)
    {
        ck::nav::Warning(TEXT("CkNavigation v1 binds to the default navmesh only ([{}] nav-data instances detected). "
                              "Agents whose AgentParams don't match the default nav-agent class will path on the wrong navmesh. "
                              "Multi-crowd support is v1.1."),
            NavSys->NavDataSet.Num());
    }

    if (NOT DoReallocateCrowdAndPublishContext())
    {
        ck::nav::Warning(TEXT("CkNavigation: dtCrowd allocation failed at first bind. Will retry on next OnNavigationGenerationFinishedDelegate."));
        return;
    }

    // Saved-bake fast path: any agents that called utils_nav::Add before this tick already had
    // FTag_Nav_NeedsSetup set, but FProcessor_Nav_CrowdSetup ran once on an empty Crowd weak
    // ref and consumed the dirty event without registering them. Re-stamp the tag so the
    // processor refires now that _Crowd is alive.
    DoReMarkAgentsForCrowdSetup();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Navigation_Subsystem::
    Deinitialize()
        -> void
{
    // Cancel any pending debounced rebuild before tearing down.
    if (auto* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(_RebuildTimerHandle);
    }

    // Unbind nav-regen first — this delegate owns a hard ref back to us.
    if (auto* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
    {
        NavSys->OnNavigationGenerationFinishedDelegate.RemoveDynamic(
            this, &UCk_Navigation_Subsystem::HandleNavmeshRegenerated);
    }

    // Clear registry context entries FIRST so late-firing processors see a null weak ref
    // rather than a dangling strong ref.
    if (_EcsWorldSubsystem.IsValid())
    {
        auto& Registry = _EcsWorldSubsystem->Get_Registry();
        // Zero-arg call default-constructs an empty TWeakPtr / TWeakObjectPtr in the registry context.
        // Passing {} fails template-arg deduction for the variadic SetContext<T, Args...>(Args&&...).
        Registry.SetContext<TWeakPtr<dtCrowd>>();
        Registry.SetContext<TWeakObjectPtr<ARecastNavMesh>>();
    }

    _Crowd.Reset();   // Custom deleter fires, dtFreeCrowd is called
    _NavMesh.Reset();
    _CachedDetourNavMesh = nullptr;

    Super::Deinitialize();
}

// --------------------------------------------------------------------------------------------------------------------

void
    UCk_Navigation_Subsystem::
    HandleNavmeshRegenerated(
        ANavigationData* InNavData)
{
    auto* AsRecast = Cast<ARecastNavMesh>(InNavData);
    if (ck::Is_NOT_Valid(AsRecast, ck::IsValid_Policy_NullptrOnly{}))
    { return; }

    // First-time adopt: subsystem initialized before RecastNavMesh-Default existed (typical
    // PIE warm-up). Latch onto the now-existing nav-data; the dtNavMesh* equality check
    // below sees null vs new and falls through to debounced rebuild as expected.
    if (NOT _NavMesh.IsValid())
    {
        ck::nav::Warning(TEXT("CkNavigation: adopting navmesh on first regen (subsystem init ran before RecastNavMesh-Default existed)."));
        _NavMesh = AsRecast;
    }
    else if (AsRecast != _NavMesh.Get())
    {
        // Different nav-data instance regenerated — v1 binds to default only.
        return;
    }

    // P3-3: pointer-equality check. Tile-only updates leave the dtNavMesh* identical;
    // we only rebuild when the underlying mesh pointer changes (full nav-data rebuild).
    auto* CurrentDetour = _NavMesh.IsValid() ? ck_nav_subsystem::Resolve_DetourNavMesh(_NavMesh.Get()) : nullptr;

    if (CurrentDetour == _CachedDetourNavMesh)
    {
        ck::nav::VeryVerbose(TEXT("Nav-regen fired but dtNavMesh* unchanged (tile update); skipping rebuild."));
        return;
    }

    const auto DebounceSeconds = UCk_Utils_Nav_ProjectSettings::Get_NavRebuildDebounceSeconds();

    ck::nav::Warning(TEXT("Nav-regen fired with new dtNavMesh*; debouncing rebuild ([{}]s)."), DebounceSeconds);

    if (auto* World = GetWorld())
    {
        auto& TM = World->GetTimerManager();
        TM.ClearTimer(_RebuildTimerHandle);
        TM.SetTimer(_RebuildTimerHandle, this,
            &UCk_Navigation_Subsystem::DoExecuteCrowdRebuild,
            DebounceSeconds, false);
    }
}

// --------------------------------------------------------------------------------------------------------------------

void
    UCk_Navigation_Subsystem::
    DoExecuteCrowdRebuild()
{
    ck::nav::Warning(TEXT("Rebuilding dtCrowd after navmesh regeneration."));

    // 1. Drop context so every processor bails on the next tick.
    if (_EcsWorldSubsystem.IsValid())
    {
        auto& Registry = _EcsWorldSubsystem->Get_Registry();
        Registry.SetContext<TWeakPtr<dtCrowd>>();   // empty TWeakPtr published
    }

    // 2. Re-mark all agents for setup (handles both already-registered and warm-up populations).
    DoReMarkAgentsForCrowdSetup();

    // 3. Free old crowd.
    _Crowd.Reset();

    // 4. Re-alloc + republish context. On failure we'll retry on next regen-fire.
    if (NOT DoReallocateCrowdAndPublishContext())
    {
        ck::nav::Warning(TEXT("DoExecuteCrowdRebuild: dtCrowd re-allocation failed. Agents will remain stuck in NeedsSetup until the next successful nav-regen."));
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Navigation_Subsystem::
    DoReMarkAgentsForCrowdSetup()
    -> void
{
    if (NOT _EcsWorldSubsystem.IsValid())
    { return; }

    // FProcessor_Nav_CrowdSetup uses MarkedDirtyBy = FTag_Nav_NeedsSetup, which gates the
    // processor on tag-ADD events — merely "having the tag set" does not requeue the entity.
    // Two populations need a fresh dirty marker:
    //   a) Already-registered agents (e.g. mid-game nav-regen): drop CrowdRegistered tag +
    //      stale CrowdAgent fragment, then re-stamp NeedsSetup.
    //   b) Agents that ran utils_nav::Add BEFORE _Crowd was first allocated (saved-bake fast
    //      path or PIE warm-up): NeedsSetup was set on Add, but CrowdSetup ran ONCE on the
    //      original dirty event with an empty Crowd weak ref and bailed — consuming the
    //      dirty event. Without re-stamping, CrowdSetup never re-fires for these agents.
    auto& Registry = _EcsWorldSubsystem->Get_Registry();
    auto Count = 0;
    Registry.View<ck::FFragment_Nav_AgentParams>().ForEach(
        [&Registry, &Count](FCk_Entity InEntity, const ck::FFragment_Nav_AgentParams&)
        {
            auto Handle = ck::MakeHandle(InEntity, Registry);
            Handle.Try_Remove<ck::FTag_Nav_CrowdRegistered>();
            Handle.Try_Remove<ck::FFragment_Nav_CrowdAgent>();
            Handle.Try_Remove<ck::FTag_Nav_NeedsSetup>();
            Handle.Add<ck::FTag_Nav_NeedsSetup>();
            ++Count;
        });

    ck::nav::Warning(TEXT("CkNavigation: re-marked [{}] nav agent(s) for CrowdSetup."), Count);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Navigation_Subsystem::
    DoReallocateCrowdAndPublishContext()
    -> bool
{
    auto* DetourNavMesh = ck_nav_subsystem::Resolve_DetourNavMesh(_NavMesh.Get());

    if (ck::Is_NOT_Valid(DetourNavMesh, ck::IsValid_Policy_NullptrOnly{}))
    {
        ck::nav::Warning(TEXT("CkNavigation: ARecastNavMesh has no dtNavMesh yet. dtCrowd init skipped."));
        return false;
    }

    const auto MaxAgents = UCk_Utils_Nav_ProjectSettings::Get_MaxCrowdAgents();
    const auto MaxRadius = UCk_Utils_Nav_ProjectSettings::Get_MaxAgentRadius();

    auto* RawCrowd = dtAllocCrowd();
    if (ck::Is_NOT_Valid(RawCrowd, ck::IsValid_Policy_NullptrOnly{}))
    {
        ck::nav::Error(TEXT("dtAllocCrowd failed."));
        return false;
    }

    if (NOT RawCrowd->init(MaxAgents, MaxRadius, DetourNavMesh))
    {
        dtFreeCrowd(RawCrowd);
        ck::nav::Error(TEXT("dtCrowd->init failed (MaxAgents=[{}], MaxRadius=[{}])"), MaxAgents, MaxRadius);
        return false;
    }

    // [UE] explicitly moved obstacle-avoidance query allocation out of dtCrowd::init into a
    // separate initAvoidance(). Without this call, m_obstacleQuery stays nullptr and
    // updateStepAvoidance crashes on its first invocation (DetourCrowd.cpp:1548 -> reset()).
    // Defaults match UCrowdManager (CrowdManager.cpp:170-171, 927):
    //   maxNeighbors = 6, maxWalls = 8, maxCustomPatterns = 1 (no patterns configured)
    // initAvoidance memsets m_obstacleQueryParams to defaults, so it MUST run before
    // DoConfigureObstacleAvoidanceProfiles below — otherwise our profile overrides get wiped.
    constexpr int32 InitAvoidance_MaxNeighbors      = 6;
    constexpr int32 InitAvoidance_MaxWalls          = 8;
    constexpr int32 InitAvoidance_MaxCustomPatterns = 1;
    if (NOT RawCrowd->initAvoidance(InitAvoidance_MaxNeighbors, InitAvoidance_MaxWalls, InitAvoidance_MaxCustomPatterns))
    {
        dtFreeCrowd(RawCrowd);
        ck::nav::Error(TEXT("dtCrowd->initAvoidance failed (MaxNeighbors=[{}], MaxWalls=[{}], MaxCustomPatterns=[{}])"),
            InitAvoidance_MaxNeighbors, InitAvoidance_MaxWalls, InitAvoidance_MaxCustomPatterns);
        return false;
    }

    _Crowd = TSharedPtr<dtCrowd>{RawCrowd, FCk_DtCrowd_Deleter{}};
    _CachedDetourNavMesh = DetourNavMesh;

    // V1 REQUIREMENT: configure all 4 obstacle-avoidance profiles so ECk_Nav_AvoidanceQuality
    // actually means something. dtCrowd::initAvoidance populates index 0..N with identical
    // defaults; we override indices 0-3 with distinct values per the spec table.
    DoConfigureObstacleAvoidanceProfiles(*_Crowd);

    // Publish weak refs into registry context for processor factories.
    if (_EcsWorldSubsystem.IsValid())
    {
        auto& Registry = _EcsWorldSubsystem->Get_Registry();
        Registry.SetContext<TWeakPtr<dtCrowd>>(_Crowd);
        Registry.SetContext<TWeakObjectPtr<ARecastNavMesh>>(_NavMesh);
    }

    ck::nav::Warning(TEXT("CkNavigation: dtCrowd initialized (MaxAgents=[{}], MaxRadius=[{}])"), MaxAgents, MaxRadius);

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Navigation_Subsystem::
    DoConfigureObstacleAvoidanceProfiles(
        dtCrowd& InCrowd)
        -> void
{
    // Verified A8 baseline values from dtCrowd::init (DetourCrowd.cpp:468-481).
    // These are identical across all 4 indices after init; we override below to differentiate
    // the adaptive-sampling depth/divs/rings per ECk_Nav_AvoidanceQuality tier.
    auto MakeBase = []()
    {
        dtObstacleAvoidanceParams P{};
        P.velBias       = 0.4f;
        P.weightDesVel  = 2.0f;
        P.weightCurVel  = 0.75f;
        P.weightSide    = 0.75f;
        P.weightToi     = 2.5f;
        P.horizTime     = 2.5f;
        P.patternIdx    = 0xff;   // adaptive sampling (per UE Detour comment)
        return P;
    };

    // Tier 0 — Low: cheap, far NPCs / ambient crowd
    {
        auto P = MakeBase();
        P.adaptiveDepth = 1;
        P.adaptiveDivs  = 5;
        P.adaptiveRings = 1;
        InCrowd.setObstacleAvoidanceParams(0, &P);
    }

    // Tier 1 — Medium: mid-range NPCs
    {
        auto P = MakeBase();
        P.adaptiveDepth = 2;
        P.adaptiveDivs  = 5;
        P.adaptiveRings = 2;
        InCrowd.setObstacleAvoidanceParams(1, &P);
    }

    // Tier 2 — High: close-combat NPCs (default for FCk_Nav_AgentParams)
    {
        auto P = MakeBase();
        P.adaptiveDepth = 3;
        P.adaptiveDivs  = 7;
        P.adaptiveRings = 2;
        InCrowd.setObstacleAvoidanceParams(2, &P);
    }

    // Tier 3 — Best: hero / player units
    {
        auto P = MakeBase();
        P.adaptiveDepth = 3;
        P.adaptiveDivs  = 7;
        P.adaptiveRings = 3;
        InCrowd.setObstacleAvoidanceParams(3, &P);
    }
}

// --------------------------------------------------------------------------------------------------------------------
