#include "CkInsightsAnalyzer/Core/CkTraceSession.h"
#include "CkInsightsAnalyzer_Log.h"

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Validation/CkIsValid.h"

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
    , _GameThreadIdCached(Other._GameThreadIdCached)
{
    Other._GameThreadIdCached = false;
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
        _GameThreadIdCached = Other._GameThreadIdCached;
        Other._GameThreadIdCached = false;
    }
    return *this;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_TraceSession::
    Open(const FString& FilePath)
    -> bool
{
    if (NOT PrepareAnalysisService())
    {
        return false;
    }
    if (NOT AnalyzeFile(FilePath))
    {
        return false;
    }

    // Log basic info (need read scope for provider access)
    {
        TraceServices::FAnalysisSessionReadScope ReadScope(*_Session.Get());
        const auto FrameCount = GetFrameProvider()
            ? GetFrameProvider()->GetFrameCount(ETraceFrameType::TraceFrameType_Game) : 0;
        const auto Duration = _Session->GetDurationSeconds();
        ck::insights_analyzer::Log(
            TEXT("Trace opened: {:.1f}s duration, {} game frames"), Duration, FrameCount);
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

    if (ck::Is_NOT_Valid(_AnalysisService))
    {
        _AnalysisService = TraceServicesModule.CreateAnalysisService();
    }

    if (ck::Is_NOT_Valid(_AnalysisService))
    {
        ck::insights_analyzer::Error(TEXT("Failed to create TraceServices analysis service"));
        return false;
    }

    return true;
}

auto
    FCk_TraceSession::
    AnalyzeFile(const FString& FilePath)
    -> bool
{
    if (ck::Is_NOT_Valid(_AnalysisService))
    {
        ck::insights_analyzer::Error(TEXT("AnalyzeFile called without PrepareAnalysisService"));
        return false;
    }

    if (NOT FPaths::FileExists(FilePath))
    {
        ck::insights_analyzer::Error(TEXT("Trace file not found: {}"), FilePath);
        return false;
    }

    ck::insights_analyzer::Log(TEXT("Analyzing trace: {}"), FilePath);

    _Session = _AnalysisService->Analyze(*FilePath);

    if (ck::Is_NOT_Valid(_Session))
    {
        ck::insights_analyzer::Error(TEXT("Failed to analyze trace: {}"), FilePath);
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
    _GameThreadIdCached = false;
    _CachedGameThreadId = static_cast<uint32>(INDEX_NONE);
}

auto
    FCk_TraceSession::
    IsOpen() const
    -> bool
{
    return ck::IsValid(_Session) && _Session->IsAnalysisComplete();
}

auto
    FCk_TraceSession::
    GetDurationSeconds() const
    -> double
{
    if (ck::Is_NOT_Valid(_Session)) return 0.0;

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
    check(ck::IsValid(_Session));
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
    if (ck::Is_NOT_Valid(_Session)) return nullptr;
    return TraceServices::ReadTimingProfilerProvider(*_Session.Get());
}

auto
    FCk_TraceSession::
    GetFrameProvider() const
    -> const TraceServices::IFrameProvider*
{
    if (ck::Is_NOT_Valid(_Session)) return nullptr;
    return &TraceServices::ReadFrameProvider(*_Session.Get());
}

auto
    FCk_TraceSession::
    GetThreadProvider() const
    -> const TraceServices::IThreadProvider*
{
    if (ck::Is_NOT_Valid(_Session)) return nullptr;
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
    if (_GameThreadIdCached)
    {
        return _CachedGameThreadId;
    }

    if (NOT IsOpen())
    {
        return static_cast<uint32>(INDEX_NONE);
    }

    TraceServices::FAnalysisSessionReadScope ReadScope(*_Session.Get());

    const auto ThreadProvider = GetThreadProvider();
    const auto TimingProvider = GetTimingProvider();

    if (ck::Is_NOT_Valid(ThreadProvider, ck::IsValid_Policy_NullptrOnly{}) ||
        ck::Is_NOT_Valid(TimingProvider, ck::IsValid_Policy_NullptrOnly{}))
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
        _GameThreadIdCached = true;
        return _CachedGameThreadId;
    }

    // Strategy 2: Fallback — thread with most events
    ck::insights_analyzer::Warning(
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
    _GameThreadIdCached = true;
    return _CachedGameThreadId;
}

auto
    FCk_TraceSession::
    GetTimelineIndex(uint32 ThreadId) const
    -> uint32
{
    if (NOT IsOpen()) return static_cast<uint32>(INDEX_NONE);

    const auto TimingProvider = GetTimingProvider();
    if (ck::Is_NOT_Valid(TimingProvider, ck::IsValid_Policy_NullptrOnly{}))
    { return static_cast<uint32>(INDEX_NONE); }

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
    if (NOT IsOpen()) return 0;

    TraceServices::FAnalysisSessionReadScope ReadScope(*_Session.Get());
    const TraceServices::IFrameProvider* FrameProvider = GetFrameProvider();
    return FrameProvider ? FrameProvider->GetFrameCount(ETraceFrameType::TraceFrameType_Game) : 0;
}

auto
    FCk_TraceSession::
    GetFrame(uint64 Index) const
    -> const TraceServices::FFrame*
{
    if (NOT IsOpen()) return nullptr;

    const TraceServices::IFrameProvider* FrameProvider = GetFrameProvider();
    return FrameProvider ? FrameProvider->GetFrame(ETraceFrameType::TraceFrameType_Game, Index) : nullptr;
}

auto
    FCk_TraceSession::
    ReadTimers(TFunctionRef<void(const TraceServices::ITimingProfilerTimerReader&)> Callback) const
    -> void
{
    if (NOT IsOpen()) return;

    const auto TimingProvider = GetTimingProvider();
    if (ck::IsValid(TimingProvider, ck::IsValid_Policy_NullptrOnly{}))
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
    if (NOT IsOpen()) return false;

    const auto TimingProvider = GetTimingProvider();
    if (ck::Is_NOT_Valid(TimingProvider, ck::IsValid_Policy_NullptrOnly{}))
    { return false; }

    return TimingProvider->ReadTimeline(TimelineIndex, Callback);
}

auto
    FCk_TraceSession::
    GetThreadInfos() const
    -> TArray<TraceServices::FThreadInfo>
{
    TArray<TraceServices::FThreadInfo> Result;
    if (NOT IsOpen()) return Result;

    TraceServices::FAnalysisSessionReadScope ReadScope(*_Session.Get());
    const auto ThreadProvider = GetThreadProvider();
    if (ck::IsValid(ThreadProvider, ck::IsValid_Policy_NullptrOnly{}))
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
