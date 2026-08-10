#include "CkInsightsAnalyzer/Core/CkTraceSession.h"
#include "CkInsightsAnalyzer_Log.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Validation/CkIsValid.h"

#include <TraceServices/ITraceServicesModule.h>
#include <TraceServices/Model/Log.h>
#include <TraceServices/Model/Screenshot.h>

namespace ck_trace_session
{
    auto
        Map_ScreenshotTimestampToFrame(const TraceServices::IFrameProvider& FrameProvider,
                                       ETraceFrameType FrameType,
                                       double TimestampSeconds,
                                       int64& OutFrameIndex,
                                       bool& OutIsInsideFrame)
        -> void
    {
        OutFrameIndex = INDEX_NONE;
        OutIsInsideFrame = false;

        if (FrameProvider.GetFrameCount(FrameType) == 0)
        {
            return;
        }

        const auto FrameIndex = FrameProvider.GetFrameNumberForTimestamp(FrameType, TimestampSeconds);
        const auto Frame = FrameProvider.GetFrame(FrameType, FrameIndex);
        if (ck::Is_NOT_Valid(Frame, ck::IsValid_Policy_NullptrOnly{}) || TimestampSeconds < Frame->StartTime)
        {
            return;
        }

        OutFrameIndex = static_cast<int64>(Frame->Index);
        OutIsInsideFrame = TimestampSeconds <= Frame->EndTime;
    }
}

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
    StartAnalysis(const FString& FilePath)
    -> bool
{
    CK_ENSURE_IF_NOT(IsInGameThread(),
        TEXT("StartAnalysis must be called on the game thread — registered trace modules "
             "(e.g. ChaosVD) ensure(IsInGameThread()) inside OnAnalysisBegin, which fires "
             "synchronously here"))
    { return false; }

    if (ck::Is_NOT_Valid(_AnalysisService))
    {
        ck::insights_analyzer::Error(TEXT("StartAnalysis called without PrepareAnalysisService"));
        return false;
    }

    if (NOT FPaths::FileExists(FilePath))
    {
        ck::insights_analyzer::Error(TEXT("Trace file not found: {}"), FilePath);
        return false;
    }

    ck::insights_analyzer::Log(TEXT("Starting trace analysis: {}"), FilePath);

    _Session = _AnalysisService->StartAnalysis(*FilePath);

    if (ck::Is_NOT_Valid(_Session))
    {
        ck::insights_analyzer::Error(TEXT("Failed to start analysis for trace: {}"), FilePath);
        _AnalysisService.Reset();
        return false;
    }

    _FilePath = FilePath;
    return true;
}

auto
    FCk_TraceSession::
    IsAnalysisComplete() const
    -> bool
{
    return _Session.IsValid() && _Session->IsAnalysisComplete();
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
    GetRenderFrameCount() const
    -> uint64
{
    if (NOT IsOpen()) return 0;

    TraceServices::FAnalysisSessionReadScope ReadScope(*_Session.Get());
    const TraceServices::IFrameProvider* FrameProvider = GetFrameProvider();
    return FrameProvider ? FrameProvider->GetFrameCount(ETraceFrameType::TraceFrameType_Rendering) : 0;
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

auto
    FCk_TraceSession::
    GetScreenshots() const
    -> TArray<FCk_TraceScreenshot>
{
    TArray<FCk_TraceScreenshot> Result;
    if (NOT IsOpen())
    {
        return Result;
    }

    TraceServices::FAnalysisSessionReadScope ReadScope(*_Session.Get());

    const TraceServices::ILogProvider* LogProvider =
        _Session->ReadProvider<TraceServices::ILogProvider>(TraceServices::GetLogProviderName());
    const TraceServices::IScreenshotProvider* ScreenshotProvider =
        _Session->ReadProvider<TraceServices::IScreenshotProvider>(TraceServices::GetScreenshotProviderName());
    const TraceServices::IFrameProvider* FrameProvider =
        _Session->ReadProvider<TraceServices::IFrameProvider>(TraceServices::GetFrameProviderName());

    if (LogProvider == nullptr || ScreenshotProvider == nullptr)
    {
        return Result;
    }

    const TraceServices::FLogCategoryInfo* ScreenshotCategory = nullptr;
    LogProvider->EnumerateCategories(
        [&ScreenshotCategory](const TraceServices::FLogCategoryInfo& Category)
        {
            if (Category.Name != nullptr && FCString::Strcmp(Category.Name, TEXT("Screenshot")) == 0)
            {
                ScreenshotCategory = &Category;
            }
        });

    if (ScreenshotCategory == nullptr)
    {
        return Result;
    }

    const uint64 MessageCount = LogProvider->GetMessageCount();
    LogProvider->EnumerateMessagesByIndex(
        0, MessageCount,
        [&Result, ScreenshotCategory, ScreenshotProvider, FrameProvider](const TraceServices::FLogMessageInfo& Message)
        {
            if (Message.Category != ScreenshotCategory || Message.Line < 0)
            {
                return;
            }

            const uint32 ScreenshotId = static_cast<uint32>(Message.Line);
            const TSharedPtr<const TraceServices::FScreenshot> Screenshot =
                ScreenshotProvider->GetScreenshot(ScreenshotId);
            if (ck::Is_NOT_Valid(Screenshot))
            {
                return;
            }

            FCk_TraceScreenshot& Metadata = Result.AddDefaulted_GetRef();
            Metadata.Id = Screenshot->Id;
            Metadata.Name = Screenshot->Name;
            Metadata.TimestampSeconds = Screenshot->Timestamp;
            Metadata.Width = Screenshot->Width;
            Metadata.Height = Screenshot->Height;
            Metadata.ExpectedPayloadByteSize = Screenshot->Size;
            Metadata.PayloadByteSize = static_cast<uint32>(Screenshot->Data.Num());
            Metadata.bIsPayloadComplete =
                Metadata.PayloadByteSize == Metadata.ExpectedPayloadByteSize;

            if (FrameProvider != nullptr)
            {
                ck_trace_session::Map_ScreenshotTimestampToFrame(
                    *FrameProvider,
                    ETraceFrameType::TraceFrameType_Game,
                    Metadata.TimestampSeconds,
                    Metadata.GameFrameIndex,
                    Metadata.bIsInsideGameFrame);
                ck_trace_session::Map_ScreenshotTimestampToFrame(
                    *FrameProvider,
                    ETraceFrameType::TraceFrameType_Rendering,
                    Metadata.TimestampSeconds,
                    Metadata.RenderFrameIndex,
                    Metadata.bIsInsideRenderFrame);
            }
        });

    Result.Sort(
        [](const FCk_TraceScreenshot& Left, const FCk_TraceScreenshot& Right)
        {
            if (Left.TimestampSeconds != Right.TimestampSeconds)
            {
                return Left.TimestampSeconds < Right.TimestampSeconds;
            }
            return Left.Id < Right.Id;
        });
    return Result;
}

auto
    FCk_TraceSession::
    TryCopyScreenshotData(uint32 InScreenshotId, TArray<uint8>& OutData) const
    -> bool
{
    OutData.Reset();
    if (NOT IsOpen())
    {
        return false;
    }

    TraceServices::FAnalysisSessionReadScope ReadScope(*_Session.Get());
    const TraceServices::IScreenshotProvider* ScreenshotProvider =
        _Session->ReadProvider<TraceServices::IScreenshotProvider>(TraceServices::GetScreenshotProviderName());
    if (ScreenshotProvider == nullptr)
    {
        return false;
    }

    const TSharedPtr<const TraceServices::FScreenshot> Screenshot =
        ScreenshotProvider->GetScreenshot(InScreenshotId);
    if (ck::Is_NOT_Valid(Screenshot) || Screenshot->Data.Num() != Screenshot->Size)
    {
        return false;
    }

    OutData = Screenshot->Data;
    return true;
}

// --------------------------------------------------------------------------------------------------------------------
