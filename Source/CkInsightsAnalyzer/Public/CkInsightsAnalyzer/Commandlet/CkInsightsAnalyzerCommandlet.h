#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "CkInsightsAnalyzerCommandlet.generated.h"

// --------------------------------------------------------------------------------------------------------------------

/**
 * Headless commandlet for analyzing Unreal Insights .utrace files.
 *
 * Invocation:
 *   UnrealEditor-Cmd.exe <Project.uproject> -run=CkInsightsAnalyzer -trace=path/to/file.utrace [options]
 *
 * Options:
 *   -trace=<path>       Path to .utrace file (required)
 *   -frame=<N>          Analyze a single frame by index
 *   -frames=<N-M>       Analyze a range of frames (e.g., 100-200)
 *   -worst=<N>          Find and analyze the N worst frames (default: 10)
 *   -all                Analyze all frames (generates multi-frame report)
 *   -raw                Include raw ranked timer list
 *   -top=<N>            Number of raw timers to show (default: 50)
 *   -budget=<ms>        Target frame budget in ms (default: 16.67 for 60fps)
 *   -output=<path>      Write report to file (in addition to stdout)
 *   -clipboard          Copy report to Windows clipboard
 *
 * Examples:
 *   -run=CkInsightsAnalyzer -trace=C:/traces/session.utrace -worst=5
 *   -run=CkInsightsAnalyzer -trace=C:/traces/session.utrace -frame=347
 *   -run=CkInsightsAnalyzer -trace=C:/traces/session.utrace -frames=100-200
 *   -run=CkInsightsAnalyzer -trace=C:/traces/session.utrace -all -output=report.md
 */
UCLASS()
class UCkInsightsAnalyzerCommandlet : public UCommandlet
{
    GENERATED_BODY()

public:
    UCkInsightsAnalyzerCommandlet();

    //~ Begin UCommandlet Interface
    virtual int32 Main(const FString& Params) override;
    //~ End UCommandlet Interface

private:
    auto PrintUsage() const -> void;
    auto CopyToClipboard(const FString& Text) const -> bool;
};

// --------------------------------------------------------------------------------------------------------------------
