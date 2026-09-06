#include "CkInsightsAnalyzer/Report/CkJsonReport.h"

#include "CkInsightsAnalyzer/Core/CkTraceSession.h"

#include "CkCore/Macros/CkMacros.h"

#include "Dom/JsonObject.h"
#include "Misc/AutomationTest.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace ck_json_report_tests
{
    auto Parse(const FString& InJson, FAutomationTestBase& InTest) -> TSharedPtr<FJsonObject>
    {
        TSharedPtr<FJsonObject> Root;
        const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(InJson);
        InTest.TestTrue(TEXT("JSON parses"), FJsonSerializer::Deserialize(Reader, Root));
        InTest.TestTrue(TEXT("parsed JSON has a root object"), Root.IsValid());
        return Root;
    }

    auto MakeAccounting(uint64 InFrameIndex, double InStart, double InEnd) -> FCk_FrameAccounting
    {
        auto Accounting = FCk_FrameAccounting{};
        Accounting.FrameIndex = InFrameIndex;
        Accounting.ThreadId = 17;
        Accounting.StartTime = InStart;
        Accounting.EndTime = InEnd;
        Accounting.FrameMs = 10.0;
        Accounting.InstrumentedMs = 8.0;
        Accounting.NamedWaitMs = 2.0;
        Accounting.OtherInstrumentedMs = 6.0;
        Accounting.UninstrumentedMs = 2.0;
        Accounting.ExclusiveSumMs = 8.0;
        Accounting.ExclusiveCoverageErrorMs = 0.0;
        return Accounting;
    }
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_JsonReport_Schema3AccountingAndTimerSemantics,
    "Ck.CkInsightsAnalyzer.JsonReport.Schema3AccountingAndTimerSemantics",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_JsonReport_Schema3AccountingAndTimerSemantics::RunTest(const FString&)
{
    using namespace ck_json_report_tests;

    auto Stats = FCk_MultiFrameStats{};
    Stats.FrameCount = 2;
    Stats.AvgFrameMs = 10.0;
    Stats.MinFrameMs = 9.0;
    Stats.MaxFrameMs = 11.0;
    Stats.P95FrameMs = 10.9;
    Stats.P99FrameMs = 10.98;
    Stats.FrameAccounting = {MakeAccounting(12, 1.125123456, 1.135123456), MakeAccounting(13, 2.25, 2.26)};
    Stats.AverageAccounting = MakeAccounting(0, 0.0, 0.0);
    Stats.TotalExclusiveMs = 8.0;
    Stats.ReportedTimerExclusiveMs = 6.0;
    Stats.OmittedTimerExclusiveMs = 2.0;
    Stats.ReportedCategoryExclusiveMs = 5.0;
    Stats.OmittedCategoryExclusiveMs = 3.0;
    auto TimerStats = FCk_MultiFrameStats::FTimerStats{};
    TimerStats.Name = TEXT("RecursiveScope");
    TimerStats.Category = TEXT("Other");
    TimerStats.AvgExclMs = 6.0;
    TimerStats.P95ExclMs = 7.0;
    TimerStats.MaxExclMs = 8.0;
    TimerStats.AvgOuterInclMs = 8.0;
    TimerStats.AvgInclMs = 12.0;
    TimerStats.AvgCount = 1.5;
    TimerStats.FramesPresent = 2;
    Stats.TimerAverages.Add(MoveTemp(TimerStats));

    auto Config = FCk_MultiFrameReportConfig{};
    Config.WorstFrameCount = 7;
    const auto Json = FCk_JsonReport::GenerateMultiFrame(FCk_TraceSession{}, Stats, Config);
    const auto Root = Parse(Json, *this);
    if (NOT Root.IsValid())
    { return false; }

    TestEqual(TEXT("schema version is three"), Root->GetIntegerField(TEXT("schemaVersion")), 3);
    const TSharedPtr<FJsonObject> Generator = Root->GetObjectField(TEXT("generator"));
    TestTrue(TEXT("accounting scope documents GT-only accounting"),
        Generator->GetStringField(TEXT("accountingScope")).Contains(TEXT("GameThread only")));
    TestTrue(TEXT("inclusive semantics document nested and outer values"),
        Generator->GetStringField(TEXT("inclusiveSemantics")).Contains(TEXT("nested")) &&
            Generator->GetStringField(TEXT("inclusiveSemantics")).Contains(TEXT("outer")));

    const TSharedPtr<FJsonObject> Multi = Root->GetObjectField(TEXT("multiFrame"));
    TestFalse(TEXT("wait averages are marked uncomputed"), Multi->GetBoolField(TEXT("waitAveragesComputed")));
    TestFalse(TEXT("uncomputed waits are omitted rather than reported empty"), Multi->HasField(TEXT("waitAverages")));

    const TArray<TSharedPtr<FJsonValue>>& PerFrame = Multi->GetArrayField(TEXT("frameAccounting"));
    TestEqual(TEXT("every accounted frame is exported"), PerFrame.Num(), 2);
    if (PerFrame.Num() != 2)
    { return false; }

    const TSharedPtr<FJsonObject> First = PerFrame[0]->AsObject();
    TestEqual(TEXT("per-frame start timestamp is not rounded"), First->GetNumberField(TEXT("frameStartSeconds")), 1.125123456);
    TestEqual(TEXT("per-frame end timestamp is not rounded"), First->GetNumberField(TEXT("frameEndSeconds")), 1.135123456);
    TestEqual(TEXT("instrumented partitions into named wait plus other"),
        First->GetNumberField(TEXT("namedWaitMs")) + First->GetNumberField(TEXT("otherInstrumentedMs")),
        First->GetNumberField(TEXT("instrumentedMs")));
    TestEqual(TEXT("frame partitions into instrumented plus uninstrumented"),
        First->GetNumberField(TEXT("instrumentedMs")) + First->GetNumberField(TEXT("uninstrumentedMs")),
        First->GetNumberField(TEXT("frameMs")));

    const TSharedPtr<FJsonObject> Average = Multi->GetObjectField(TEXT("averageAccounting"));
    TestFalse(TEXT("average accounting omits synthetic timestamps"), Average->HasField(TEXT("frameStartSeconds")));
    TestFalse(TEXT("average accounting omits synthetic end timestamps"), Average->HasField(TEXT("frameEndSeconds")));

    const TSharedPtr<FJsonObject> Thresholds = Multi->GetObjectField(TEXT("thresholdOmissionAccounting"));
    TestEqual(TEXT("timer reported plus omitted conserves total"),
        Thresholds->GetNumberField(TEXT("reportedTimerExclusiveMs")) +
            Thresholds->GetNumberField(TEXT("omittedTimerExclusiveMs")),
        Thresholds->GetNumberField(TEXT("totalExclusiveMs")));
    TestEqual(TEXT("category reported plus omitted conserves total"),
        Thresholds->GetNumberField(TEXT("reportedCategoryExclusiveMs")) +
            Thresholds->GetNumberField(TEXT("omittedCategoryExclusiveMs")),
        Thresholds->GetNumberField(TEXT("totalExclusiveMs")));

    const TArray<TSharedPtr<FJsonValue>>& Timers = Multi->GetArrayField(TEXT("timerAverages"));
    TestEqual(TEXT("one timer average is exported"), Timers.Num(), 1);
    if (Timers.Num() != 1)
    { return false; }

    const TSharedPtr<FJsonObject> Timer = Timers[0]->AsObject();
    TestEqual(TEXT("outer inclusive is independently exported"),
        Timer->GetNumberField(TEXT("avgOuterInclusiveMs")), 8.0);
    TestEqual(TEXT("nested inclusive remains independently exported"),
        Timer->GetNumberField(TEXT("avgInclusiveMs")), 12.0);

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_JsonReport_WaitAveragesPresenceMatchesComputedStatus,
    "Ck.CkInsightsAnalyzer.JsonReport.WaitAveragesPresenceMatchesComputedStatus",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_JsonReport_WaitAveragesPresenceMatchesComputedStatus::RunTest(const FString&)
{
    using namespace ck_json_report_tests;

    auto Stats = FCk_MultiFrameStats{};
    Stats.FrameCount = 1;
    Stats.WaitAveragesComputed = true;
    Stats.WaitAverages.Add(FCk_WaitThreadSummary{17, TEXT("GameThread"), true, 2.5, 10.0});

    const auto Json = FCk_JsonReport::GenerateMultiFrame(
        FCk_TraceSession{}, Stats, FCk_MultiFrameReportConfig{});
    const auto Root = Parse(Json, *this);
    if (NOT Root.IsValid())
    { return false; }

    const TSharedPtr<FJsonObject> Multi = Root->GetObjectField(TEXT("multiFrame"));
    TestTrue(TEXT("computed wait averages are marked computed"), Multi->GetBoolField(TEXT("waitAveragesComputed")));
    TestTrue(TEXT("computed wait averages are present"), Multi->HasField(TEXT("waitAverages")));
    const TArray<TSharedPtr<FJsonValue>>& Waits = Multi->GetArrayField(TEXT("waitAverages"));
    TestEqual(TEXT("computed wait row is exported"), Waits.Num(), 1);
    if (Waits.Num() == 1)
    {
        TestEqual(TEXT("wait row retains wall time"), Waits[0]->AsObject()->GetNumberField(TEXT("wallMs")), 10.0);
    }

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

#endif // WITH_DEV_AUTOMATION_TESTS
