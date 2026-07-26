#include "CkInsightsAnalyzer/Commandlet/CkInsightsAnalyzerCommandlet.h"

#include "CkInsightsAnalyzer/Core/CkTraceSession.h"
#include "CkInsightsAnalyzer/Core/CkFrameAnalyzer.h"
#include "CkInsightsAnalyzer/Report/CkFrameReport.h"
#include "CkInsightsAnalyzer/Report/CkJsonReport.h"
#include "CkInsightsAnalyzer/Report/CkMultiFrameReport.h"
#include "CkInsightsAnalyzer_Log.h"

#include "CkCore/Macros/CkMacros.h"

#include "HAL/PlatformApplicationMisc.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

// --------------------------------------------------------------------------------------------------------------------

UCkInsightsAnalyzerCommandlet::UCkInsightsAnalyzerCommandlet()
{
    LogToConsole = true;
    IsEditor = true;
    IsClient = false;
    IsServer = false;

    HelpDescription = TEXT("Analyzes Unreal Insights .utrace files and generates Slack-friendly performance reports.");
    HelpUsage = TEXT("CkInsightsAnalyzer -trace=<path> [-frame=N | -frames=N-M | -worst=N | -all] [-output=path] [-clipboard]");

    HelpParamNames.Add(TEXT("trace"));
    HelpParamDescriptions.Add(TEXT("Path to .utrace file (required)"));

    HelpParamNames.Add(TEXT("frame"));
    HelpParamDescriptions.Add(TEXT("Analyze a single frame by index"));

    HelpParamNames.Add(TEXT("worst"));
    HelpParamDescriptions.Add(TEXT("Find and analyze the N worst frames (default: 10)"));

    HelpParamNames.Add(TEXT("all"));
    HelpParamDescriptions.Add(TEXT("Analyze all frames (multi-frame report)"));

    HelpParamNames.Add(TEXT("json"));
    HelpParamDescriptions.Add(TEXT("Emit machine-readable JSON instead of Slack markdown"));

    HelpParamNames.Add(TEXT("output"));
    HelpParamDescriptions.Add(TEXT("Write report to file"));

    HelpParamNames.Add(TEXT("clipboard"));
    HelpParamDescriptions.Add(TEXT("Copy report to Windows clipboard"));
}

// --------------------------------------------------------------------------------------------------------------------

int32
    UCkInsightsAnalyzerCommandlet::
    Main(const FString& Params)
{
    TArray<FString> Tokens;
    TArray<FString> Switches;
    TMap<FString, FString> ParsedParams;
    UCommandlet::ParseCommandLine(*Params, Tokens, Switches, ParsedParams);

    // ---- Validate required params ----

    const auto TracePath = ParsedParams.Find(TEXT("trace"));
    if (NOT TracePath || TracePath->IsEmpty())
    {
        ck::insights_analyzer::Error(TEXT("Missing required parameter: -trace=<path>"));
        PrintUsage();
        return 1;
    }

    // ---- Parse optional params ----

    const bool Clipboard = Switches.Contains(TEXT("clipboard"));
    const bool AllFrames = Switches.Contains(TEXT("all"));
    const bool Raw = Switches.Contains(TEXT("raw"));
    const bool Json = Switches.Contains(TEXT("json"));
    const bool ShowAll = Switches.Contains(TEXT("showall"));

    const auto OutputPath = ParsedParams.Find(TEXT("output"));

    double Budget = 16.67;
    if (const auto BudgetStr = ParsedParams.Find(TEXT("budget")))
    {
        Budget = FCString::Atod(**BudgetStr);
        if (Budget <= 0.0) Budget = 16.67;
    }

    int32 RawTop = 50;
    if (const auto TopStr = ParsedParams.Find(TEXT("top")))
    {
        RawTop = FCString::Atoi(**TopStr);
        if (RawTop <= 0) RawTop = 50;
    }

    // ---- Open trace ----

    ck::insights_analyzer::Display(TEXT("Opening trace: {}"), *TracePath);

    FCk_TraceSession Session;
    if (NOT Session.Open(*TracePath))
    {
        ck::insights_analyzer::Error(TEXT("Failed to open trace file: {}"), *TracePath);
        return 1;
    }

    const auto TotalFrames = Session.GetFrameCount();
    ck::insights_analyzer::Display(
        TEXT("Trace loaded: {:.1f}s duration, {} game frames"),
        Session.GetDurationSeconds(), TotalFrames);

    if (TotalFrames == 0)
    {
        ck::insights_analyzer::Error(TEXT("No game frames found in trace"));
        return 1;
    }

    // ---- Determine analysis mode and generate report ----

    FString Report;

    if (const auto FrameStr = ParsedParams.Find(TEXT("frame")))
    {
        const auto FrameIndex = FCString::Strtoui64(**FrameStr, nullptr, 10);

        if (FrameIndex >= TotalFrames)
        {
            ck::insights_analyzer::Error(
                TEXT("Frame index {} out of range (0-{})"), FrameIndex, TotalFrames - 1);
            return 1;
        }

        ck::insights_analyzer::Display(TEXT("Analyzing frame {}..."), FrameIndex);

        FCk_FrameAnalysisResult Result = FCk_FrameAnalyzer::AnalyzeFrame(Session, FrameIndex);
        if (NOT Result.IsValid())
        {
            ck::insights_analyzer::Error(TEXT("Frame {} produced no analysis data"), FrameIndex);
            return 1;
        }

        FCk_FrameReportConfig ReportConfig;
        ReportConfig.TargetFrameMs = Budget;
        ReportConfig.ShowRawTimerList = Raw;
        ReportConfig.RawTimerCount = RawTop;
        ReportConfig.ShowAllChildren = ShowAll;

        if (Json)
        {
            Report = FCk_JsonReport::GenerateSingleFrame(Session, Result, ReportConfig);
        }
        else
        {
            FCk_FrameReport FrameReport(ReportConfig);
            Report = FrameReport.Generate(Session, Result);
        }
    }
    else if (const auto FramesStr = ParsedParams.Find(TEXT("frames")))
    {
        FString StartStr, EndStr;
        if (FramesStr->Split(TEXT("-"), &StartStr, &EndStr))
        {
            const auto StartFrame = FCString::Strtoui64(*StartStr, nullptr, 10);
            const auto EndFrameInclusive = FCString::Strtoui64(*EndStr, nullptr, 10);
            const auto EndFrame = FMath::Min(EndFrameInclusive + 1, TotalFrames);

            if (StartFrame >= EndFrame)
            {
                ck::insights_analyzer::Error(
                    TEXT("Invalid frame range: {}-{}"), StartFrame, EndFrame - 1);
                return 1;
            }

            ck::insights_analyzer::Display(
                TEXT("Analyzing frames {}-{} ({} frames)..."),
                StartFrame, EndFrame - 1, EndFrame - StartFrame);

            FCk_MultiFrameReportConfig MultiConfig;
            MultiConfig.TargetFrameMs = Budget;

            FCk_MultiFrameReport MultiReport(MultiConfig);
            Report = MultiReport.AnalyzeAndGenerate(Session, StartFrame, EndFrame);

            if (Json)
            {
                Report = MultiReport.GetStats().FrameCount > 0
                    ? FCk_JsonReport::GenerateMultiFrame(Session, MultiReport.GetStats(), MultiReport.GetConfig())
                    : FString{};
            }
        }
        else
        {
            ck::insights_analyzer::Error(
                TEXT("Invalid -frames format. Use: -frames=100-200"));
            return 1;
        }
    }
    else if (const auto WorstStr = ParsedParams.Find(TEXT("worst")))
    {
        int32 WorstCount = FCString::Atoi(**WorstStr);
        if (WorstCount <= 0) WorstCount = 10;

        ck::insights_analyzer::Display(
            TEXT("Finding {} worst frames across {} total frames..."), WorstCount, TotalFrames);

        FCk_MultiFrameReportConfig MultiConfig;
        MultiConfig.TargetFrameMs = Budget;
        MultiConfig.WorstFrameCount = WorstCount;

        FCk_MultiFrameReport MultiReport(MultiConfig);
        Report = MultiReport.AnalyzeWorstFrames(Session, WorstCount);

        if (Json)
        {
            Report = MultiReport.GetStats().FrameCount > 0
                ? FCk_JsonReport::GenerateMultiFrame(Session, MultiReport.GetStats(), MultiReport.GetConfig())
                : FString{};
        }
    }
    else if (AllFrames)
    {
        ck::insights_analyzer::Display(
            TEXT("Analyzing all {} frames..."), TotalFrames);

        FCk_MultiFrameReportConfig MultiConfig;
        MultiConfig.TargetFrameMs = Budget;

        FCk_MultiFrameReport MultiReport(MultiConfig);
        Report = MultiReport.AnalyzeAndGenerate(Session, 0, 0);

        if (Json)
        {
            Report = MultiReport.GetStats().FrameCount > 0
                ? FCk_JsonReport::GenerateMultiFrame(Session, MultiReport.GetStats(), MultiReport.GetConfig())
                : FString{};
        }
    }
    else
    {
        ck::insights_analyzer::Display(
            TEXT("No mode specified, defaulting to -worst=10..."));

        FCk_MultiFrameReportConfig MultiConfig;
        MultiConfig.TargetFrameMs = Budget;

        FCk_MultiFrameReport MultiReport(MultiConfig);
        Report = MultiReport.AnalyzeWorstFrames(Session, 10);

        if (Json)
        {
            Report = MultiReport.GetStats().FrameCount > 0
                ? FCk_JsonReport::GenerateMultiFrame(Session, MultiReport.GetStats(), MultiReport.GetConfig())
                : FString{};
        }
    }

    // ---- Output ----

    if (Report.IsEmpty())
    {
        ck::insights_analyzer::Error(TEXT("Report generation produced no output"));
        return 1;
    }

    // Split by line to avoid log truncation
    TArray<FString> ReportLines;
    Report.ParseIntoArrayLines(ReportLines);
    for (const FString& Line : ReportLines)
    {
        ck::insights_analyzer::Display(TEXT("{}"), Line);
    }

    // A commandlet's CWD is Binaries/Win64, so a raw relative output path would silently land
    // there — resolve relative paths against the project dir instead.
    if (OutputPath && NOT OutputPath->IsEmpty())
    {
        const auto ResolvedOutputPath = FPaths::IsRelative(**OutputPath)
            ? FPaths::Combine(FPaths::ProjectDir(), **OutputPath)
            : *OutputPath;

        if (FFileHelper::SaveStringToFile(Report, *ResolvedOutputPath, FFileHelper::EEncodingOptions::ForceUTF8))
        {
            ck::insights_analyzer::Display(TEXT("Report written to: {}"), ResolvedOutputPath);
        }
        else
        {
            ck::insights_analyzer::Error(TEXT("Failed to write report to: {}"), ResolvedOutputPath);
        }
    }

    if (Clipboard)
    {
        if (CopyToClipboard(Report))
        {
            ck::insights_analyzer::Display(TEXT("Report copied to clipboard."));
        }
        else
        {
            ck::insights_analyzer::Warning(TEXT("Failed to copy to clipboard."));
        }
    }

    Session.Close();
    return 0;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkInsightsAnalyzerCommandlet::
    PrintUsage() const
    -> void
{
    ck::insights_analyzer::Display(TEXT(""));
    ck::insights_analyzer::Display(TEXT("CkInsightsAnalyzer - Unreal Insights Frame Analyzer"));
    ck::insights_analyzer::Display(TEXT(""));
    ck::insights_analyzer::Display(TEXT("Usage:"));
    ck::insights_analyzer::Display(TEXT("  UnrealEditor-Cmd.exe <Project.uproject> -run=CkInsightsAnalyzer -trace=<path> [options]"));
    ck::insights_analyzer::Display(TEXT(""));
    ck::insights_analyzer::Display(TEXT("Required:"));
    ck::insights_analyzer::Display(TEXT("  -trace=<path>       Path to .utrace file"));
    ck::insights_analyzer::Display(TEXT(""));
    ck::insights_analyzer::Display(TEXT("Analysis modes (pick one):"));
    ck::insights_analyzer::Display(TEXT("  -frame=<N>          Analyze a single frame by index"));
    ck::insights_analyzer::Display(TEXT("  -frames=<N-M>       Analyze a range of frames (e.g., 100-200)"));
    ck::insights_analyzer::Display(TEXT("  -worst=<N>          Find and report the N worst frames (default)"));
    ck::insights_analyzer::Display(TEXT("  -all                Analyze all frames"));
    ck::insights_analyzer::Display(TEXT(""));
    ck::insights_analyzer::Display(TEXT("Options:"));
    ck::insights_analyzer::Display(TEXT("  -budget=<ms>        Target frame budget in ms (default: 16.67)"));
    ck::insights_analyzer::Display(TEXT("  -raw                Include raw ranked timer list"));
    ck::insights_analyzer::Display(TEXT("  -top=<N>            Number of raw timers (default: 50)"));
    ck::insights_analyzer::Display(TEXT("  -json               Emit machine-readable JSON instead of Slack markdown"));
    ck::insights_analyzer::Display(TEXT("  -output=<path>      Write report to file"));
    ck::insights_analyzer::Display(TEXT("  -clipboard          Copy report to Windows clipboard"));
    ck::insights_analyzer::Display(TEXT(""));
    ck::insights_analyzer::Display(TEXT("Examples:"));
    ck::insights_analyzer::Display(TEXT("  -run=CkInsightsAnalyzer -trace=C:/traces/session.utrace -worst=5"));
    ck::insights_analyzer::Display(TEXT("  -run=CkInsightsAnalyzer -trace=C:/traces/session.utrace -frame=347 -clipboard"));
    ck::insights_analyzer::Display(TEXT("  -run=CkInsightsAnalyzer -trace=C:/traces/session.utrace -all -output=report.md"));
    ck::insights_analyzer::Display(TEXT("  -run=CkInsightsAnalyzer -trace=C:/traces/session.utrace -all -json -output=report.json"));
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkInsightsAnalyzerCommandlet::
    CopyToClipboard(const FString& Text) const
    -> bool
{
    FPlatformApplicationMisc::ClipboardCopy(*Text);
    return true;
}

// --------------------------------------------------------------------------------------------------------------------
