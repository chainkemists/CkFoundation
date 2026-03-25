#include "CkInsightsAnalyzer/Commandlet/CkInsightsAnalyzerCommandlet.h"

#include "CkInsightsAnalyzer/Core/CkTraceSession.h"
#include "CkInsightsAnalyzer/Core/CkFrameAnalyzer.h"
#include "CkInsightsAnalyzer/Report/CkFrameReport.h"
#include "CkInsightsAnalyzer/Report/CkMultiFrameReport.h"
#include "CkInsightsAnalyzer_Log.h"

#include "HAL/PlatformApplicationMisc.h"
#include "Misc/FileHelper.h"

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
    // Parse command line
    TArray<FString> Tokens;
    TArray<FString> Switches;
    TMap<FString, FString> ParsedParams;
    UCommandlet::ParseCommandLine(*Params, Tokens, Switches, ParsedParams);

    // ---- Validate required params ----

    const FString* TracePath = ParsedParams.Find(TEXT("trace"));
    if (!TracePath || TracePath->IsEmpty())
    {
        UE_LOG(CkInsightsAnalyzer, Error, TEXT("Missing required parameter: -trace=<path>"));
        PrintUsage();
        return 1;
    }

    // ---- Parse optional params ----

    const bool bClipboard = Switches.Contains(TEXT("clipboard"));
    const bool bAll = Switches.Contains(TEXT("all"));
    const bool bRaw = Switches.Contains(TEXT("raw"));

    const FString* OutputPath = ParsedParams.Find(TEXT("output"));

    double Budget = 16.67;
    if (const FString* BudgetStr = ParsedParams.Find(TEXT("budget")))
    {
        Budget = FCString::Atod(**BudgetStr);
        if (Budget <= 0.0) Budget = 16.67;
    }

    int32 RawTop = 50;
    if (const FString* TopStr = ParsedParams.Find(TEXT("top")))
    {
        RawTop = FCString::Atoi(**TopStr);
        if (RawTop <= 0) RawTop = 50;
    }

    // ---- Open trace ----

    UE_LOG(CkInsightsAnalyzer, Display, TEXT("Opening trace: %s"), **TracePath);

    FCk_TraceSession Session;
    if (!Session.Open(*TracePath))
    {
        UE_LOG(CkInsightsAnalyzer, Error, TEXT("Failed to open trace file: %s"), **TracePath);
        return 1;
    }

    const uint64 TotalFrames = Session.GetFrameCount();
    UE_LOG(CkInsightsAnalyzer, Display,
        TEXT("Trace loaded: %.1fs duration, %llu game frames"),
        Session.GetDurationSeconds(), TotalFrames);

    if (TotalFrames == 0)
    {
        UE_LOG(CkInsightsAnalyzer, Error, TEXT("No game frames found in trace"));
        return 1;
    }

    // ---- Determine analysis mode and generate report ----

    FString Report;

    if (const FString* FrameStr = ParsedParams.Find(TEXT("frame")))
    {
        // Single frame mode
        const uint64 FrameIndex = FCString::Strtoui64(**FrameStr, nullptr, 10);

        if (FrameIndex >= TotalFrames)
        {
            UE_LOG(CkInsightsAnalyzer, Error,
                TEXT("Frame index %llu out of range (0-%llu)"), FrameIndex, TotalFrames - 1);
            return 1;
        }

        UE_LOG(CkInsightsAnalyzer, Display, TEXT("Analyzing frame %llu..."), FrameIndex);

        FCk_FrameAnalysisResult Result = FCk_FrameAnalyzer::AnalyzeFrame(Session, FrameIndex);
        if (!Result.IsValid())
        {
            UE_LOG(CkInsightsAnalyzer, Error, TEXT("Frame %llu produced no analysis data"), FrameIndex);
            return 1;
        }

        FCk_FrameReportConfig ReportConfig;
        ReportConfig.TargetFrameMs = Budget;
        ReportConfig.bShowRawTimerList = bRaw;
        ReportConfig.RawTimerCount = RawTop;

        FCk_FrameReport FrameReport(ReportConfig);
        Report = FrameReport.Generate(Session, Result);
    }
    else if (const FString* FramesStr = ParsedParams.Find(TEXT("frames")))
    {
        // Frame range mode (e.g., "100-200")
        FString StartStr, EndStr;
        if (FramesStr->Split(TEXT("-"), &StartStr, &EndStr))
        {
            const uint64 StartFrame = FCString::Strtoui64(*StartStr, nullptr, 10);
            uint64 EndFrame = FCString::Strtoui64(*EndStr, nullptr, 10);

            // EndFrame is inclusive in user input, exclusive in API
            EndFrame = FMath::Min(EndFrame + 1, TotalFrames);

            if (StartFrame >= EndFrame)
            {
                UE_LOG(CkInsightsAnalyzer, Error,
                    TEXT("Invalid frame range: %llu-%llu"), StartFrame, EndFrame - 1);
                return 1;
            }

            UE_LOG(CkInsightsAnalyzer, Display,
                TEXT("Analyzing frames %llu-%llu (%llu frames)..."),
                StartFrame, EndFrame - 1, EndFrame - StartFrame);

            FCk_MultiFrameReportConfig MultiConfig;
            MultiConfig.TargetFrameMs = Budget;

            FCk_MultiFrameReport MultiReport(MultiConfig);
            Report = MultiReport.AnalyzeAndGenerate(Session, StartFrame, EndFrame);
        }
        else
        {
            UE_LOG(CkInsightsAnalyzer, Error,
                TEXT("Invalid -frames format. Use: -frames=100-200"));
            return 1;
        }
    }
    else if (const FString* WorstStr = ParsedParams.Find(TEXT("worst")))
    {
        // Worst N frames mode
        int32 WorstCount = FCString::Atoi(**WorstStr);
        if (WorstCount <= 0) WorstCount = 10;

        UE_LOG(CkInsightsAnalyzer, Display,
            TEXT("Finding %d worst frames across %llu total frames..."), WorstCount, TotalFrames);

        FCk_MultiFrameReportConfig MultiConfig;
        MultiConfig.TargetFrameMs = Budget;
        MultiConfig.WorstFrameCount = WorstCount;

        FCk_MultiFrameReport MultiReport(MultiConfig);
        Report = MultiReport.AnalyzeWorstFrames(Session, WorstCount);
    }
    else if (bAll)
    {
        // All frames mode
        UE_LOG(CkInsightsAnalyzer, Display,
            TEXT("Analyzing all %llu frames..."), TotalFrames);

        FCk_MultiFrameReportConfig MultiConfig;
        MultiConfig.TargetFrameMs = Budget;

        FCk_MultiFrameReport MultiReport(MultiConfig);
        Report = MultiReport.AnalyzeAndGenerate(Session, 0, 0);
    }
    else
    {
        // Default: worst 10 frames
        UE_LOG(CkInsightsAnalyzer, Display,
            TEXT("No mode specified, defaulting to -worst=10..."));

        FCk_MultiFrameReportConfig MultiConfig;
        MultiConfig.TargetFrameMs = Budget;

        FCk_MultiFrameReport MultiReport(MultiConfig);
        Report = MultiReport.AnalyzeWorstFrames(Session, 10);
    }

    // ---- Output ----

    if (Report.IsEmpty())
    {
        UE_LOG(CkInsightsAnalyzer, Error, TEXT("Report generation produced no output"));
        return 1;
    }

    // Print to stdout (via UE_LOG Display)
    // Split by line to avoid log truncation
    TArray<FString> ReportLines;
    Report.ParseIntoArrayLines(ReportLines);
    for (const FString& Line : ReportLines)
    {
        UE_LOG(CkInsightsAnalyzer, Display, TEXT("%s"), *Line);
    }

    // Write to file if requested
    if (OutputPath && !OutputPath->IsEmpty())
    {
        if (FFileHelper::SaveStringToFile(Report, **OutputPath, FFileHelper::EEncodingOptions::ForceUTF8))
        {
            UE_LOG(CkInsightsAnalyzer, Display, TEXT("Report written to: %s"), **OutputPath);
        }
        else
        {
            UE_LOG(CkInsightsAnalyzer, Error, TEXT("Failed to write report to: %s"), **OutputPath);
        }
    }

    // Copy to clipboard if requested
    if (bClipboard)
    {
        if (CopyToClipboard(Report))
        {
            UE_LOG(CkInsightsAnalyzer, Display, TEXT("Report copied to clipboard."));
        }
        else
        {
            UE_LOG(CkInsightsAnalyzer, Warning, TEXT("Failed to copy to clipboard."));
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
    UE_LOG(CkInsightsAnalyzer, Display, TEXT(""));
    UE_LOG(CkInsightsAnalyzer, Display, TEXT("CkInsightsAnalyzer - Unreal Insights Frame Analyzer"));
    UE_LOG(CkInsightsAnalyzer, Display, TEXT(""));
    UE_LOG(CkInsightsAnalyzer, Display, TEXT("Usage:"));
    UE_LOG(CkInsightsAnalyzer, Display, TEXT("  UnrealEditor-Cmd.exe <Project.uproject> -run=CkInsightsAnalyzer -trace=<path> [options]"));
    UE_LOG(CkInsightsAnalyzer, Display, TEXT(""));
    UE_LOG(CkInsightsAnalyzer, Display, TEXT("Required:"));
    UE_LOG(CkInsightsAnalyzer, Display, TEXT("  -trace=<path>       Path to .utrace file"));
    UE_LOG(CkInsightsAnalyzer, Display, TEXT(""));
    UE_LOG(CkInsightsAnalyzer, Display, TEXT("Analysis modes (pick one):"));
    UE_LOG(CkInsightsAnalyzer, Display, TEXT("  -frame=<N>          Analyze a single frame by index"));
    UE_LOG(CkInsightsAnalyzer, Display, TEXT("  -frames=<N-M>       Analyze a range of frames (e.g., 100-200)"));
    UE_LOG(CkInsightsAnalyzer, Display, TEXT("  -worst=<N>          Find and report the N worst frames (default)"));
    UE_LOG(CkInsightsAnalyzer, Display, TEXT("  -all                Analyze all frames"));
    UE_LOG(CkInsightsAnalyzer, Display, TEXT(""));
    UE_LOG(CkInsightsAnalyzer, Display, TEXT("Options:"));
    UE_LOG(CkInsightsAnalyzer, Display, TEXT("  -budget=<ms>        Target frame budget in ms (default: 16.67)"));
    UE_LOG(CkInsightsAnalyzer, Display, TEXT("  -raw                Include raw ranked timer list"));
    UE_LOG(CkInsightsAnalyzer, Display, TEXT("  -top=<N>            Number of raw timers (default: 50)"));
    UE_LOG(CkInsightsAnalyzer, Display, TEXT("  -output=<path>      Write report to file"));
    UE_LOG(CkInsightsAnalyzer, Display, TEXT("  -clipboard          Copy report to Windows clipboard"));
    UE_LOG(CkInsightsAnalyzer, Display, TEXT(""));
    UE_LOG(CkInsightsAnalyzer, Display, TEXT("Examples:"));
    UE_LOG(CkInsightsAnalyzer, Display, TEXT("  -run=CkInsightsAnalyzer -trace=C:/traces/session.utrace -worst=5"));
    UE_LOG(CkInsightsAnalyzer, Display, TEXT("  -run=CkInsightsAnalyzer -trace=C:/traces/session.utrace -frame=347 -clipboard"));
    UE_LOG(CkInsightsAnalyzer, Display, TEXT("  -run=CkInsightsAnalyzer -trace=C:/traces/session.utrace -all -output=report.md"));
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
