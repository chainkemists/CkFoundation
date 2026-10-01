#include "CkPso_Subsystem.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Format/CkFormat.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkLoadingScreen/Subsystem/CkLoadingScreen_Subsystem.h"

#include "CkPso/CkPso_Log.h"
#include "CkPso/Settings/CkPso_Settings.h"

#include <Engine/Engine.h>
#include <Engine/GameInstance.h>
#include <Engine/World.h>
#include <HAL/IConsoleManager.h>
#include <PipelineStateCache.h>
#include <ProfilingDebugging/CsvProfiler.h>
#include <PSOPrecacheFwd.h>
#include <ShaderPipelineCache.h>
#include <UObject/ScriptInterface.h>

#if PSO_PRECACHING_VALIDATE && UE_WITH_PSO_PRECACHING
#include <PSOPrecacheValidation.h>
#endif

// --------------------------------------------------------------------------------------------------------------------

CSV_DEFINE_CATEGORY(CkPso, true);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_pso_subsystem
{
    // A blocking LoadMap arrives as one enormous tick. Charging it whole would burn the MaxWait budget before the
    // first PSO of the new map is even requested, so no single tick may advance the tracker further than this.
    constexpr auto MaxTrackerStep = ck::time::Seconds(0.1);

    auto DoSaturate_ToInt32(
        uint64 InValue) -> int32
    {
        return static_cast<int32>(FMath::Min(InValue, static_cast<uint64>(MAX_int32)));
    }

    // Counters dropping below the baseline mean something called ResetPSOHitchTrackingStats (or reset the
    // validation collectors) inside the window; everything counted since that reset happened inside it too.
    auto DoGet_CounterDelta(
        int32 InCurrent,
        int32 InBaseline) -> int32
    {
        return InCurrent >= InBaseline ? InCurrent - InBaseline : InCurrent;
    }

    auto DoSample_HitchCounters() -> FCk_Pso_HitchReport
    {
        const auto RuntimeStats = PipelineStateCache::GetPSORuntimeCreationStats();

        auto Sample = FCk_Pso_HitchReport{}
            .Set_TotalPsoCreations(DoSaturate_ToInt32(RuntimeStats.TotalPSOCreations))
            .Set_GraphicsPsoHitches(DoSaturate_ToInt32(RuntimeStats.GraphicsPSOHitches))
            .Set_ComputePsoHitches(DoSaturate_ToInt32(RuntimeStats.ComputePSOHitches))
            .Set_PreviouslyPrecachedPsoHitches(DoSaturate_ToInt32(RuntimeStats.PreviouslyPrecachedPSOHitches))
            .Set_SuspectedUnhealthyDriverCachePsoHitches(DoSaturate_ToInt32(RuntimeStats.SuspectedUnhealthyDriverCachePSOHitches))
            .Set_DriverCacheHealth(RuntimeStats.bDriverCacheSuspectedUnhealthy
                ? ECk_Pso_DriverCacheHealth::SuspectedUnhealthy
                : ECk_Pso_DriverCacheHealth::Healthy);

#if PSO_PRECACHING_VALIDATE && UE_WITH_PSO_PRECACHING
        if (PSOCollectorStats::IsPrecachingValidationEnabled())
        {
            const auto& FullPsoStats = PSOCollectorStats::GetFullPSOPrecacheStatsCollector().GetStats();

            Sample
                .Set_Validation(ECk_EnableDisable::Enable)
                .Set_FullPsoHits(DoSaturate_ToInt32(FullPsoStats.HitData.GetTotalCount()))
                .Set_FullPsoMisses(DoSaturate_ToInt32(FullPsoStats.MissData.GetTotalCount()))
                .Set_FullPsoTooLate(DoSaturate_ToInt32(FullPsoStats.TooLateData.GetTotalCount()))
                .Set_FullPsoUntracked(DoSaturate_ToInt32(FullPsoStats.UntrackedData.GetTotalCount()));
        }
#endif

        return Sample;
    }

    auto DoGet_HitchDelta(
        const FCk_Pso_HitchReport& InCurrent,
        const FCk_Pso_HitchReport& InBaseline) -> FCk_Pso_HitchReport
    {
        return FCk_Pso_HitchReport{}
            .Set_TotalPsoCreations(DoGet_CounterDelta(InCurrent.Get_TotalPsoCreations(), InBaseline.Get_TotalPsoCreations()))
            .Set_GraphicsPsoHitches(DoGet_CounterDelta(InCurrent.Get_GraphicsPsoHitches(), InBaseline.Get_GraphicsPsoHitches()))
            .Set_ComputePsoHitches(DoGet_CounterDelta(InCurrent.Get_ComputePsoHitches(), InBaseline.Get_ComputePsoHitches()))
            .Set_PreviouslyPrecachedPsoHitches(DoGet_CounterDelta(
                InCurrent.Get_PreviouslyPrecachedPsoHitches(), InBaseline.Get_PreviouslyPrecachedPsoHitches()))
            .Set_SuspectedUnhealthyDriverCachePsoHitches(DoGet_CounterDelta(
                InCurrent.Get_SuspectedUnhealthyDriverCachePsoHitches(), InBaseline.Get_SuspectedUnhealthyDriverCachePsoHitches()))
            .Set_DriverCacheHealth(InCurrent.Get_DriverCacheHealth())
            .Set_Validation(InCurrent.Get_Validation())
            .Set_FullPsoHits(DoGet_CounterDelta(InCurrent.Get_FullPsoHits(), InBaseline.Get_FullPsoHits()))
            .Set_FullPsoMisses(DoGet_CounterDelta(InCurrent.Get_FullPsoMisses(), InBaseline.Get_FullPsoMisses()))
            .Set_FullPsoTooLate(DoGet_CounterDelta(InCurrent.Get_FullPsoTooLate(), InBaseline.Get_FullPsoTooLate()))
            .Set_FullPsoUntracked(DoGet_CounterDelta(InCurrent.Get_FullPsoUntracked(), InBaseline.Get_FullPsoUntracked()));
    }

    auto DoFormat_HitchReport(
        const FCk_Pso_HitchReport& InReport) -> FString
    {
        return ck::Format_UE(
            TEXT("runtime PSO creations [{}], graphics hitches [{}], compute hitches [{}], previously-precached hitches [{}], ")
            TEXT("unhealthy-driver-cache hitches [{}], driver cache [{}], validation [{}]: full-PSO hit [{}] miss [{}] too-late [{}] untracked [{}]"),
            InReport.Get_TotalPsoCreations(),
            InReport.Get_GraphicsPsoHitches(),
            InReport.Get_ComputePsoHitches(),
            InReport.Get_PreviouslyPrecachedPsoHitches(),
            InReport.Get_SuspectedUnhealthyDriverCachePsoHitches(),
            InReport.Get_DriverCacheHealth(),
            InReport.Get_Validation(),
            InReport.Get_FullPsoHits(),
            InReport.Get_FullPsoMisses(),
            InReport.Get_FullPsoTooLate(),
            InReport.Get_FullPsoUntracked());
    }

    // Elapsed is deliberately not part of the comparison: it advances every tick while a window holds, so including
    // it would turn a change event into a per-frame one.
    auto DoGet_HasDrainProgressChanged(
        const FCk_Pso_DrainProgress& InPrevious,
        const FCk_Pso_DrainProgress& InCurrent) -> bool
    {
        return InPrevious.Get_State() != InCurrent.Get_State() ||
               InPrevious.Get_TimeoutReason() != InCurrent.Get_TimeoutReason() ||
               InPrevious.Get_NumRemaining() != InCurrent.Get_NumRemaining() ||
               InPrevious.Get_NumPeak() != InCurrent.Get_NumPeak() ||
               InPrevious.Get_ProgressRatio() != InCurrent.Get_ProgressRatio();
    }

    // One log call per line: the log truncates long messages.
    auto DoLog_Report(
        const FString& InReport) -> void
    {
        constexpr auto CullEmptyLines = false;

        auto Lines = TArray<FString>{};
        InReport.ParseIntoArrayLines(Lines, CullEmptyLines);

        for (const auto& Line : Lines)
        { ck::pso::Log(TEXT("{}"), Line); }
    }

    static FAutoConsoleCommandWithWorld ConsoleCommand_DumpReport(
        TEXT("ck.Pso.DumpReport"),
        TEXT("Print the PSO drain window snapshot and the runtime PSO hitch report for the current gameplay window ")
        TEXT("(loading screen hidden -> now) of this world's game instance."),
        FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* InWorld) -> void
        {
            const auto WorldIsValid = ck::IsValid(InWorld);

            CK_ENSURE_IF_NOT(WorldIsValid, TEXT("ck.Pso.DumpReport ran without a World"))
            { return; }

            const auto GameInstance = InWorld->GetGameInstance();
            const auto PsoSubsystem = ck::IsValid(GameInstance)
                ? GameInstance->GetSubsystem<UCk_Pso_Subsystem_UE>()
                : nullptr;

            if (ck::Is_NOT_Valid(PsoSubsystem))
            {
                ck::pso::Log(TEXT("ck.Pso.DumpReport: World [{}] has no CkPso subsystem (no game instance, or a dedicated server)"),
                    InWorld);
                return;
            }

            DoLog_Report(PsoSubsystem->Get_DebugReport());
        }));
}

// --------------------------------------------------------------------------------------------------------------------

UCk_Pso_Subsystem_UE::UCk_Pso_Subsystem_UE()
    : FTickableGameObject(ETickableTickType::Never)
{
}

auto
    UCk_Pso_Subsystem_UE::
    Initialize(
        FSubsystemCollectionBase& Collection)
    -> void
{
    const auto LoadingScreenSubsystem = Collection.InitializeDependency<UCk_LoadingScreen_Subsystem_UE>();
    const auto LoadingScreenSubsystemIsValid = ck::IsValid(LoadingScreenSubsystem);

    CK_ENSURE_IF_NOT(LoadingScreenSubsystemIsValid,
        TEXT("CkPso subsystem [{}] could not initialize its CkLoadingScreen dependency - the PSO drain gate stays off for this game instance"),
        this)
    { return; }

    _LoadingScreenSubsystem = LoadingScreenSubsystem;

    LoadingScreenSubsystem->Register_LoadingProcessor(this);
    LoadingScreenSubsystem->BindTo_OnVisibilityChanged(DoMake_VisibilityChangedDelegate());

    FCoreUObjectDelegates::PreLoadMapWithContext.AddUObject(this, &ThisType::DoHandle_PreLoadMap);

    _HitchBaseline = ck_pso_subsystem::DoSample_HitchCounters();

    SetTickableTickType(GetTickableTickType());
}

auto
    UCk_Pso_Subsystem_UE::
    Deinitialize()
    -> void
{
    DoClose_GameplayWindow();

    FCoreUObjectDelegates::PreLoadMapWithContext.RemoveAll(this);

    if (const auto LoadingScreenSubsystem = _LoadingScreenSubsystem.Get();
        ck::IsValid(LoadingScreenSubsystem))
    {
        LoadingScreenSubsystem->UnbindFrom_OnVisibilityChanged(DoMake_VisibilityChangedDelegate());
        LoadingScreenSubsystem->Unregister_LoadingProcessor(this);
    }

    _LoadingScreenSubsystem.Reset();
    _DrainTracker.Request_Reset();

    SetTickableTickType(ETickableTickType::Never);
}

auto
    UCk_Pso_Subsystem_UE::
    ShouldCreateSubsystem(
        UObject* InOuter) const
    -> bool
{
    const auto GameInstance = CastChecked<UGameInstance>(InOuter);
    return NOT GameInstance->IsDedicatedServerInstance();
}

auto
    UCk_Pso_Subsystem_UE::
    Tick(
        float InDeltaTime)
    -> void
{
    if (DoGet_IsInert())
    {
        if (_DrainTracker.Get_State() != ECk_Pso_DrainState::Idle)
        { _DrainTracker.Request_Reset(); }
    }
    else if (NOT DoGet_IsWindowClosed())
    {
        const auto TrackerDeltaT = FCk_Time{FMath::Clamp(
            static_cast<double>(InDeltaTime), 0.0, ck_pso_subsystem::MaxTrackerStep.Get_Seconds())};

        _DrainTracker.Update(DoGet_NumRemaining(), TrackerDeltaT);
    }

    DoHandle_DrainStateTransition();
    DoBroadcast_DrainProgressIfChanged();

    if (UCk_Utils_Pso_Settings_UE::Get_LogDrainEveryFrame() && _DrainTracker.Get_IsHolding())
    {
        ck::pso::Log(TEXT("PSO drain window [{}]: [{}] pending, peak [{}], progress [{}], elapsed [{}]"),
            _DrainTracker.Get_State(),
            _DrainTracker.Get_NumRemaining(),
            _DrainTracker.Get_NumPeak(),
            _DrainTracker.Get_ProgressRatio(),
            _DrainTracker.Get_Elapsed());
    }
}

auto
    UCk_Pso_Subsystem_UE::
    GetTickableTickType() const
    -> ETickableTickType
{
    if (IsTemplate())
    { return ETickableTickType::Never; }

    return ETickableTickType::Conditional;
}

auto
    UCk_Pso_Subsystem_UE::
    IsTickable() const
    -> bool
{
    return ck::IsValid(GetGameInstance());
}

auto
    UCk_Pso_Subsystem_UE::
    GetStatId() const
    -> TStatId
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UCk_Pso_Subsystem_UE, STATGROUP_Tickables);
}

auto
    UCk_Pso_Subsystem_UE::
    GetTickableGameObjectWorld() const
    -> UWorld*
{
    return GetGameInstance()->GetWorld();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Pso_Subsystem_UE::
    Get_ShouldShowLoadingScreen(
        FString& OutReason) const
    -> bool
{
    if (NOT DoGet_IsGateEnabled())
    { return false; }

    if (DoGet_IsInert())
    { return false; }

    if (NOT _DrainTracker.Get_IsHolding())
    { return false; }

    OutReason = ck::Format_UE(TEXT("PSO precache: {} remaining ({}%) [{}]"),
        _DrainTracker.Get_NumRemaining(),
        FMath::RoundToInt(_DrainTracker.Get_ProgressRatio() * 100.0f),
        _DrainTracker.Get_State());

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Pso_Subsystem_UE::
    Get_DrainProgress() const
    -> FCk_Pso_DrainProgress
{
    return FCk_Pso_DrainProgress{
        _DrainTracker.Get_State(),
        _DrainTracker.Get_TimeoutReason(),
        ck_pso_subsystem::DoSaturate_ToInt32(_DrainTracker.Get_NumRemaining()),
        ck_pso_subsystem::DoSaturate_ToInt32(_DrainTracker.Get_NumPeak()),
        _DrainTracker.Get_ProgressRatio(),
        _DrainTracker.Get_Elapsed()};
}

auto
    UCk_Pso_Subsystem_UE::
    Get_IsDraining() const
    -> bool
{
    return _DrainTracker.Get_IsHolding();
}

auto
    UCk_Pso_Subsystem_UE::
    Get_IsPrecachingSupported() const
    -> bool
{
    return PipelineStateCache::IsPSOPrecachingEnabled();
}

auto
    UCk_Pso_Subsystem_UE::
    Get_HitchReport() const
    -> FCk_Pso_HitchReport
{
    return ck_pso_subsystem::DoGet_HitchDelta(ck_pso_subsystem::DoSample_HitchCounters(), _HitchBaseline);
}

auto
    UCk_Pso_Subsystem_UE::
    Get_DebugReport() const
    -> FString
{
    const auto Progress = Get_DrainProgress();
    const auto SimulatedRemaining = UCk_Utils_Pso_Settings_UE::Get_SimulatedRemaining();

    const auto DrainLine = ck::Format_UE(
        TEXT("PSO drain window: state [{}], timeout reason [{}], pending [{}], peak [{}], progress [{}%], elapsed [{}]"),
        Progress.Get_State(),
        Progress.Get_TimeoutReason(),
        Progress.Get_NumRemaining(),
        Progress.Get_NumPeak(),
        FMath::RoundToInt(Progress.Get_ProgressRatio() * 100.0f),
        Progress.Get_Elapsed());

    const auto GateLine = ck::Format_UE(
        TEXT("PSO gate: wait for precache [{}], inert [{}], precaching supported [{}], bundled cache precompiling [{}], simulated pending [{}]"),
        UCk_Utils_Pso_Settings_UE::Get_WaitForPsoPrecache(),
        DoGet_IsInert(),
        Get_IsPrecachingSupported(),
        FShaderPipelineCache::IsPrecompiling(),
        SimulatedRemaining.IsSet() ? ck::Format_UE(TEXT("{}"), SimulatedRemaining.GetValue()) : FString{TEXT("off")});

    const auto HitchLine = ck::Format_UE(
        TEXT("PSO {}: {}"),
        _GameplayWindowOpen ? TEXT("gameplay window (loading screen hidden -> now)") : TEXT("counters since the last baseline"),
        ck_pso_subsystem::DoFormat_HitchReport(Get_HitchReport()));

    return ck::Format_UE(TEXT("{}\n{}\n{}"), DrainLine, GateLine, HitchLine);
}

auto
    UCk_Pso_Subsystem_UE::
    Request_BeginDrainWindow()
    -> void
{
    DoTryBegin_DrainWindow(TEXT("Request_BeginDrainWindow"));
}

auto
    UCk_Pso_Subsystem_UE::
    BindTo_OnDrainProgressChanged(
        const FCk_Delegate_Pso_OnDrainProgressChanged& InDelegate)
    -> void
{
    _OnDrainProgressChanged.AddUnique(InDelegate);
}

auto
    UCk_Pso_Subsystem_UE::
    UnbindFrom_OnDrainProgressChanged(
        const FCk_Delegate_Pso_OnDrainProgressChanged& InDelegate)
    -> void
{
    _OnDrainProgressChanged.Remove(InDelegate);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Pso_Subsystem_UE::
    DoHandle_LoadingScreenVisibilityChanged(
        ECk_LoadingScreen_Visibility InVisibility)
    -> void
{
    if (InVisibility == ECk_LoadingScreen_Visibility::Visible)
    {
        DoClose_GameplayWindow();
        DoTryBegin_DrainWindow(TEXT("LoadingScreenVisible"));
        return;
    }

    DoOpen_GameplayWindow();
}

auto
    UCk_Pso_Subsystem_UE::
    DoHandle_PreLoadMap(
        const FWorldContext& InWorldContext,
        const FString& InMapName)
    -> void
{
    if (InWorldContext.OwningGameInstance != GetGameInstance())
    { return; }

    DoTryBegin_DrainWindow(ck::Format_UE(TEXT("PreLoadMap [{}]"), InMapName));
}

auto
    UCk_Pso_Subsystem_UE::
    DoMake_VisibilityChangedDelegate()
    -> FCk_Delegate_LoadingScreen_OnVisibilityChanged
{
    auto Delegate = FCk_Delegate_LoadingScreen_OnVisibilityChanged{};
    Delegate.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(UCk_Pso_Subsystem_UE, DoHandle_LoadingScreenVisibilityChanged));
    return Delegate;
}

auto
    UCk_Pso_Subsystem_UE::
    DoTryBegin_DrainWindow(
        const FString& InTrigger)
    -> void
{
    if (DoGet_IsInert())
    {
        ck::pso::Verbose(TEXT("Not arming a PSO drain window for [{}] - the gate is inert (PSO precaching disabled, no bundled cache precompiling, no simulated count)"),
            InTrigger);
        return;
    }

    if (_DrainTracker.Get_IsHolding())
    {
        ck::pso::Verbose(TEXT("PSO drain window already holding in [{}] - [{}] does not restart it"),
            _DrainTracker.Get_State(), InTrigger);
        return;
    }

    const auto Params = FCk_Pso_DrainTracker_Params{
        UCk_Utils_Pso_Settings_UE::Get_DrainQuietPeriod(),
        UCk_Utils_Pso_Settings_UE::Get_DrainMaxWait(),
        UCk_Utils_Pso_Settings_UE::Get_DrainStallTimeout()};

    _DrainTracker.Request_Begin(Params);
    _LastObservedDrainState = _DrainTracker.Get_State();

    // Seeds the count now, so the first reason string and the first broadcast carry the real pending count instead
    // of the empty window's zero. A zero DeltaT charges no time to any budget.
    _DrainTracker.Update(DoGet_NumRemaining(), FCk_Time::ZeroSecond());

    CSV_EVENT(CkPso, TEXT("DrainWindowBegin"));

    ck::pso::Log(TEXT("PSO drain window armed by [{}] with [{}] pending (quiet period [{}], max wait [{}], stall timeout [{}])"),
        InTrigger,
        _DrainTracker.Get_NumRemaining(),
        Params.Get_QuietPeriod(),
        Params.Get_MaxWait(),
        Params.Get_StallTimeout());

    DoHandle_DrainStateTransition();
    DoBroadcast_DrainProgressIfChanged();
}

auto
    UCk_Pso_Subsystem_UE::
    DoGet_IsInert() const
    -> bool
{
    if (UCk_Utils_Pso_Settings_UE::Get_SimulatedRemaining().IsSet())
    { return false; }

    return NOT PipelineStateCache::IsPSOPrecachingEnabled() && NOT FShaderPipelineCache::IsPrecompiling();
}

// PSO work that arrives after the screen has dropped belongs to gameplay: feeding it to a Complete tracker would
// re-open the window and put the loading screen back up mid-game.
auto
    UCk_Pso_Subsystem_UE::
    DoGet_IsWindowClosed() const
    -> bool
{
    if (_DrainTracker.Get_State() != ECk_Pso_DrainState::Complete)
    { return false; }

    const auto LoadingScreenSubsystem = _LoadingScreenSubsystem.Get();
    return ck::Is_NOT_Valid(LoadingScreenSubsystem) || NOT LoadingScreenSubsystem->Get_IsLoadingScreenShowing();
}

auto
    UCk_Pso_Subsystem_UE::
    DoGet_IsGateEnabled() const
    -> bool
{
    return UCk_Utils_Pso_Settings_UE::Get_WaitForPsoPrecache() == ECk_EnableDisable::Enable;
}

auto
    UCk_Pso_Subsystem_UE::
    DoGet_NumRemaining() const
    -> uint32
{
    if (const auto SimulatedRemaining = UCk_Utils_Pso_Settings_UE::Get_SimulatedRemaining();
        SimulatedRemaining.IsSet())
    { return SimulatedRemaining.GetValue(); }

    return FShaderPipelineCache::NumPrecompilesRemaining();
}

auto
    UCk_Pso_Subsystem_UE::
    DoHandle_DrainStateTransition()
    -> void
{
    const auto State = _DrainTracker.Get_State();
    if (State == _LastObservedDrainState)
    { return; }

    const auto PreviousState = _LastObservedDrainState;
    _LastObservedDrainState = State;

    if (State == ECk_Pso_DrainState::Complete)
    {
        CSV_EVENT(CkPso, TEXT("DrainComplete"));

        ck::pso::Log(TEXT("PSO drain window complete after [{}] (peak [{}] pending)"),
            _DrainTracker.Get_Elapsed(),
            _DrainTracker.Get_NumPeak());
        return;
    }

    // A timeout is an environment outcome (a cold driver cache on a slow machine), not a programming error, so it
    // warns and fails open rather than ensuring.
    if (State == ECk_Pso_DrainState::TimedOut)
    {
        CSV_EVENT(CkPso, TEXT("DrainTimedOut"));

        ck::pso::Warning(TEXT("PSO drain window timed out [{}] after [{}] with [{}] still pending (peak [{}]) - releasing the loading screen hold (fail-open)"),
            _DrainTracker.Get_TimeoutReason(),
            _DrainTracker.Get_Elapsed(),
            _DrainTracker.Get_NumRemaining(),
            _DrainTracker.Get_NumPeak());
        return;
    }

    if (State == ECk_Pso_DrainState::Draining && PreviousState == ECk_Pso_DrainState::Complete)
    {
        ck::pso::Log(TEXT("PSO drain window re-opened with [{}] pending after completing"),
            _DrainTracker.Get_NumRemaining());
        return;
    }

    ck::pso::Verbose(TEXT("PSO drain window [{}] -> [{}]"), PreviousState, State);
}

auto
    UCk_Pso_Subsystem_UE::
    DoBroadcast_DrainProgressIfChanged()
    -> void
{
    const auto Progress = Get_DrainProgress();

    if (NOT ck_pso_subsystem::DoGet_HasDrainProgressChanged(_LastBroadcastDrainProgress, Progress))
    { return; }

    _LastBroadcastDrainProgress = Progress;
    _OnDrainProgressChanged.Broadcast(Progress);
}

auto
    UCk_Pso_Subsystem_UE::
    DoOpen_GameplayWindow()
    -> void
{
    _HitchBaseline = ck_pso_subsystem::DoSample_HitchCounters();
    _GameplayWindowOpen = true;

    ck::pso::Log(TEXT("Loading screen hidden - last PSO drain window [{}] after [{}] (peak [{}] pending, timeout reason [{}]); hitch counters baselined"),
        _DrainTracker.Get_State(),
        _DrainTracker.Get_Elapsed(),
        _DrainTracker.Get_NumPeak(),
        _DrainTracker.Get_TimeoutReason());
}

auto
    UCk_Pso_Subsystem_UE::
    DoClose_GameplayWindow()
    -> void
{
    if (NOT _GameplayWindowOpen)
    { return; }

    _GameplayWindowOpen = false;

    ck::pso::Log(TEXT("PSO gameplay window ended: {}"),
        ck_pso_subsystem::DoFormat_HitchReport(Get_HitchReport()));
}

// --------------------------------------------------------------------------------------------------------------------
