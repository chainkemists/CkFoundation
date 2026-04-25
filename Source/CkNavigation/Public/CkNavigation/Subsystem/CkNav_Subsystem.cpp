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

    auto* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    auto* NavData = NavSys != nullptr ? Cast<ARecastNavMesh>(NavSys->GetDefaultNavDataInstance()) : nullptr;

    if (ck::Is_NOT_Valid(NavData, ck::IsValid_Policy_NullptrOnly{}))
    {
        ck::nav::Warning(TEXT("CkNavigation subsystem init: no ARecastNavMesh present. Nav features will be no-ops until a navmesh is baked."));
        return;
    }

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
        ck::nav::Warning(TEXT("CkNavigation subsystem init: dtCrowd allocation failed. Will retry on next OnNavigationGenerationFinishedDelegate."));
    }

    // P3-2 sentinel: warn-once if budget is disabled (zero is a footgun, NOT an "unbounded" mode).
    if (UCk_Utils_Nav_ProjectSettings::Get_MaxPathQueriesPerFrame() == 0)
    {
        ck::nav::Warning(TEXT("CkNavigation: _MaxPathQueriesPerFrame is 0 (DISABLED). No path queries will be processed. "
                              "Set to a positive value (default 8) in Project Settings -> Navigation."));
    }

    // Hook navmesh-regen so we don't crash when UE rebuilds the navmesh (P3-3 with debounce).
    if (ck::IsValid(NavSys, ck::IsValid_Policy_NullptrOnly{}))
    {
        NavSys->OnNavigationGenerationFinishedDelegate.AddDynamic(
            this, &UCk_Navigation_Subsystem::HandleNavmeshRegenerated);
    }
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
    // Cheap guard — ignore unrelated nav-data objects.
    if (InNavData != _NavMesh.Get())
    { return; }

    // P3-3: pointer-equality check. Tile-only updates leave the dtNavMesh* identical;
    // we only rebuild when the underlying mesh pointer changes (full nav-data rebuild).
    auto* CurrentDetour = _NavMesh.IsValid() ? ck_nav_subsystem::Resolve_DetourNavMesh(_NavMesh.Get()) : nullptr;

    if (CurrentDetour == _CachedDetourNavMesh)
    {
        ck::nav::VeryVerbose(TEXT("Nav-regen fired but dtNavMesh* unchanged (tile update); skipping rebuild."));
        return;
    }

    const auto DebounceSeconds = UCk_Utils_Nav_ProjectSettings::Get_NavRebuildDebounceSeconds();

    ck::nav::Verbose(TEXT("Nav-regen fired with new dtNavMesh*; debouncing rebuild ([{}]s)."), DebounceSeconds);

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

        // 2. Mark every existing nav agent for re-setup; CrowdSetup will re-register them
        //    once context is republished by DoReallocateCrowdAndPublishContext.
        // Empty tags are dropped from the ForEach lambda parameter list — only the
        // non-empty FFragment_Nav_AgentParams appears.
        Registry.View<ck::FFragment_Nav_AgentParams, ck::FTag_Nav_CrowdRegistered>().ForEach(
            [&Registry](FCk_Entity InEntity, const ck::FFragment_Nav_AgentParams&)
            {
                auto Handle = ck::MakeHandle(InEntity, Registry);
                Handle.Add<ck::FTag_Nav_NeedsSetup>();
                Handle.Remove<ck::FTag_Nav_CrowdRegistered>();
                Handle.Remove<ck::FFragment_Nav_CrowdAgent>();
            });
    }

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

    _Crowd = TSharedPtr<dtCrowd>{RawCrowd, FCk_DtCrowd_Deleter{}};
    _CachedDetourNavMesh = DetourNavMesh;

    // V1 REQUIREMENT: configure all 4 obstacle-avoidance profiles so ECk_Nav_AvoidanceQuality
    // actually means something. dtCrowd::init populates index 0..N with identical defaults;
    // we override indices 0-3 with distinct values per the spec table.
    DoConfigureObstacleAvoidanceProfiles(*_Crowd);

    // Publish weak refs into registry context for processor factories.
    if (_EcsWorldSubsystem.IsValid())
    {
        auto& Registry = _EcsWorldSubsystem->Get_Registry();
        Registry.SetContext<TWeakPtr<dtCrowd>>(_Crowd);
        Registry.SetContext<TWeakObjectPtr<ARecastNavMesh>>(_NavMesh);
    }

    ck::nav::Verbose(TEXT("CkNavigation: dtCrowd initialized (MaxAgents=[{}], MaxRadius=[{}])"), MaxAgents, MaxRadius);

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
