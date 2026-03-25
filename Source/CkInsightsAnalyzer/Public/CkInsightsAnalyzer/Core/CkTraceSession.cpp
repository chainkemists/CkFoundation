#include "CkInsightsAnalyzer/Core/CkTraceSession.h"
#include "CkInsightsAnalyzer_Log.h"

#include <TraceServices/ITraceServicesModule.h>

// --------------------------------------------------------------------------------------------------------------------

FCk_TraceSession::~FCk_TraceSession()
{
    Close();
}

FCk_TraceSession::FCk_TraceSession(FCk_TraceSession&& Other) noexcept
    : _FilePath(MoveTemp(Other._FilePath))
    , _AnalysisService(MoveTemp(Other._AnalysisService))
    , _Session(MoveTemp(Other._Session))
    , _CachedGameThreadId(Other._CachedGameThreadId)
    , _bGameThreadIdCached(Other._bGameThreadIdCached)
{
    Other._bGameThreadIdCached = false;
}

FCk_TraceSession& FCk_TraceSession::operator=(FCk_TraceSession&& Other) noexcept
{
    if (this != &Other)
    {
        Close();
        _FilePath = MoveTemp(Other._FilePath);
        _AnalysisService = MoveTemp(Other._AnalysisService);
        _Session = MoveTemp(Other._Session);
        _CachedGameThreadId = Other._CachedGameThreadId;
        _bGameThreadIdCached = Other._bGameThreadIdCached;
        Other._bGameThreadIdCached = false;
    }
    return *this;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_TraceSession::
    Open(const FString& FilePath)
    -> bool
{
    if (!PrepareAnalysisService())
    {
        return false;
    }
    if (!AnalyzeFile(FilePath))
    {
        return false;
    }

    // Log basic info (need read scope for provider access)
    {
        TraceServices::FAnalysisSessionReadScope ReadScope(*_Session.Get());
        const uint64 FrameCount = GetFrameProvider()
            ? GetFrameProvider()->GetFrameCount(ETraceFrameType::TraceFrameType_Game) : 0;
        const double Duration = _Session->GetDurationSeconds();
        UE_LOG(CkInsightsAnalyzer, Log,
            TEXT("Trace opened: %.1fs duration, %llu game frames"), Duration, FrameCount);
    }

    return true;
}

auto
    FCk_TraceSession::
    PrepareAnalysisService()
    -> bool
{
    Close();

    ITraceServicesModule& TraceServicesModule =
        FModuleManager::LoadModuleChecked<ITraceServicesModule>(TEXT("TraceServices"));
    _AnalysisService = TraceServicesModule.GetAnalysisService();

    if (!_AnalysisService.IsValid())
    {
        _AnalysisService = TraceServicesModule.CreateAnalysisService();
    }

    if (!_AnalysisService.IsValid())
    {
        UE_LOG(CkInsightsAnalyzer, Error, TEXT("Failed to create TraceServices analysis service"));
        return false;
    }

    return true;
}

auto
    FCk_TraceSession::
    AnalyzeFile(const FString& FilePath)
    -> bool
{
    if (!_AnalysisService.IsValid())
    {
        UE_LOG(CkInsightsAnalyzer, Error, TEXT("AnalyzeFile called without PrepareAnalysisService"));
        return false;
    }

    if (!FPaths::FileExists(FilePath))
    {
        UE_LOG(CkInsightsAnalyzer, Error, TEXT("Trace file not found: %s"), *FilePath);
        return false;
    }

    UE_LOG(CkInsightsAnalyzer, Log, TEXT("Analyzing trace: %s"), *FilePath);

    _Session = _AnalysisService->Analyze(*FilePath);

    if (!_Session.IsValid())
    {
        UE_LOG(CkInsightsAnalyzer, Error, TEXT("Failed to analyze trace: %s"), *FilePath);
        _AnalysisService.Reset();
        return false;
    }

    _FilePath = FilePath;
    return true;
}

auto
    FCk_TraceSession::
    Close()
    -> void
{
    if (_Session.IsValid())
    {
        _Session->Stop(/*bAndWait=*/ true);
        _Session.Reset();
    }
    _AnalysisService.Reset();
    _FilePath.Empty();
    _bGameThreadIdCached = false;
    _CachedGameThreadId = static_cast<uint32>(INDEX_NONE);
}

auto
    FCk_TraceSession::
    IsOpen() const
    -> bool
{
    return _Session.IsValid() && _Session->IsAnalysisComplete();
}

auto
    FCk_TraceSession::
    GetDurationSeconds() const
    -> double
{
    if (!_Session.IsValid()) return 0.0;

    TraceServices::FAnalysisSessionReadScope ReadScope(*_Session.Get());
    return _Session->GetDurationSeconds();
}

// --------------------------------------------------------------------------------------------------------------------
// Provider Access
// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_TraceSession::
    CreateReadScope() const
    -> TraceServices::FAnalysisSessionReadScope
{
    check(_Session.IsValid());
    return TraceServices::FAnalysisSessionReadScope(*_Session.Get());
}

auto
    FCk_TraceSession::
    GetSession() const
    -> const TraceServices::IAnalysisSession*
{
    return _Session.Get();
}

auto
    FCk_TraceSession::
    GetTimingProvider() const
    -> const TraceServices::ITimingProfilerProvider*
{
    if (!_Session.IsValid()) return nullptr;
    return TraceServices::ReadTimingProfilerProvider(*_Session.Get());
}

auto
    FCk_TraceSession::
    GetFrameProvider() const
    -> const TraceServices::IFrameProvider*
{
    if (!_Session.IsValid()) return nullptr;
    return &TraceServices::ReadFrameProvider(*_Session.Get());
}

auto
    FCk_TraceSession::
    GetThreadProvider() const
    -> const TraceServices::IThreadProvider*
{
    if (!_Session.IsValid()) return nullptr;
    return &TraceServices::ReadThreadProvider(*_Session.Get());
}

// --------------------------------------------------------------------------------------------------------------------
// Convenience Accessors
// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_TraceSession::
    GetGameThreadId() const
    -> uint32
{
    if (_bGameThreadIdCached)
    {
        return _CachedGameThreadId;
    }

    if (!IsOpen())
    {
        return static_cast<uint32>(INDEX_NONE);
    }

    TraceServices::FAnalysisSessionReadScope ReadScope(*_Session.Get());

    const TraceServices::IThreadProvider* ThreadProvider = GetThreadProvider();
    const TraceServices::ITimingProfilerProvider* TimingProvider = GetTimingProvider();

    if (!ThreadProvider || !TimingProvider)
    {
        return static_cast<uint32>(INDEX_NONE);
    }

    // Strategy 1: Find by name "GameThread"
    uint32 FoundId = static_cast<uint32>(INDEX_NONE);
    ThreadProvider->EnumerateThreads(
        [&FoundId](const TraceServices::FThreadInfo& Info)
        {
            if (FCString::Strcmp(Info.Name, TEXT("GameThread")) == 0)
            {
                FoundId = Info.Id;
            }
        });

    if (FoundId != static_cast<uint32>(INDEX_NONE))
    {
        _CachedGameThreadId = FoundId;
        _bGameThreadIdCached = true;
        return _CachedGameThreadId;
    }

    // Strategy 2: Fallback — thread with most events
    UE_LOG(CkInsightsAnalyzer, Warning,
        TEXT("GameThread not found by name, falling back to most-events heuristic"));

    uint64 MaxEvents = 0;
    ThreadProvider->EnumerateThreads(
        [&](const TraceServices::FThreadInfo& Info)
        {
            uint32 TimelineIdx = 0;
            if (TimingProvider->GetCpuThreadTimelineIndex(Info.Id, TimelineIdx))
            {
                TimingProvider->ReadTimeline(TimelineIdx,
                    [&](const TraceServices::ITimingProfilerProvider::Timeline& Timeline)
                    {
                        const uint64 EventCount = Timeline.GetEventCount();
                        if (EventCount > MaxEvents)
                        {
                            MaxEvents = EventCount;
                            FoundId = Info.Id;
                        }
                    });
            }
        });

    _CachedGameThreadId = FoundId;
    _bGameThreadIdCached = true;
    return _CachedGameThreadId;
}

auto
    FCk_TraceSession::
    GetTimelineIndex(uint32 ThreadId) const
    -> uint32
{
    if (!IsOpen()) return static_cast<uint32>(INDEX_NONE);

    const TraceServices::ITimingProfilerProvider* TimingProvider = GetTimingProvider();
    if (!TimingProvider) return static_cast<uint32>(INDEX_NONE);

    uint32 TimelineIndex = 0;
    if (TimingProvider->GetCpuThreadTimelineIndex(ThreadId, TimelineIndex))
    {
        return TimelineIndex;
    }
    return static_cast<uint32>(INDEX_NONE);
}

auto
    FCk_TraceSession::
    GetFrameCount() const
    -> uint64
{
    if (!IsOpen()) return 0;

    TraceServices::FAnalysisSessionReadScope ReadScope(*_Session.Get());
    const TraceServices::IFrameProvider* FrameProvider = GetFrameProvider();
    return FrameProvider ? FrameProvider->GetFrameCount(ETraceFrameType::TraceFrameType_Game) : 0;
}

auto
    FCk_TraceSession::
    GetFrame(uint64 Index) const
    -> const TraceServices::FFrame*
{
    if (!IsOpen()) return nullptr;

    const TraceServices::IFrameProvider* FrameProvider = GetFrameProvider();
    return FrameProvider ? FrameProvider->GetFrame(ETraceFrameType::TraceFrameType_Game, Index) : nullptr;
}

auto
    FCk_TraceSession::
    ReadTimers(TFunctionRef<void(const TraceServices::ITimingProfilerTimerReader&)> Callback) const
    -> void
{
    if (!IsOpen()) return;

    const TraceServices::ITimingProfilerProvider* TimingProvider = GetTimingProvider();
    if (TimingProvider)
    {
        TimingProvider->ReadTimers(Callback);
    }
}

auto
    FCk_TraceSession::
    ReadTimeline(uint32 TimelineIndex,
                 TFunctionRef<void(const TraceServices::ITimingProfilerProvider::Timeline&)> Callback) const
    -> bool
{
    if (!IsOpen()) return false;

    const TraceServices::ITimingProfilerProvider* TimingProvider = GetTimingProvider();
    if (!TimingProvider) return false;

    return TimingProvider->ReadTimeline(TimelineIndex, Callback);
}

auto
    FCk_TraceSession::
    GetThreadInfos() const
    -> TArray<TraceServices::FThreadInfo>
{
    TArray<TraceServices::FThreadInfo> Result;
    if (!IsOpen()) return Result;

    TraceServices::FAnalysisSessionReadScope ReadScope(*_Session.Get());
    const TraceServices::IThreadProvider* ThreadProvider = GetThreadProvider();
    if (ThreadProvider)
    {
        ThreadProvider->EnumerateThreads(
            [&Result](const TraceServices::FThreadInfo& Info)
            {
                Result.Add(Info);
            });
    }
    return Result;
}

// --------------------------------------------------------------------------------------------------------------------
