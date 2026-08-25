#include "CkCore/Diagnostics/CkDiagnosticVisibility.h"

#include <HAL/IConsoleManager.h>
#include <Misc/AutomationTest.h>
#include <Misc/CommandLine.h>
#include <Misc/Parse.h>
#include <Misc/ScopeExit.h>

#if WITH_DEV_AUTOMATION_TESTS

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_DiagnosticVisibility,
    "Ck.CkCore.Diagnostics.Visibility.CVar",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool
    FCkTest_DiagnosticVisibility::
    RunTest(
        const FString& Parameters)
{
    auto* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("ck.Debug.StreamerMode"));
    TestNotNull(TEXT("Streamer-mode CVar is registered"), CVar);
    if (CVar == nullptr)
    { return false; }

    const auto PreviousValue = CVar->GetInt();
    ON_SCOPE_EXIT
    {
        CVar->Set(PreviousValue, ECVF_SetByCode);
    };

    CVar->Set(1, ECVF_SetByCode);
    TestEqual(TEXT("CVar selects hidden diagnostic visibility"),
              ck::diagnostic_visibility::Get_Mode(),
              ECk_DiagnosticVisibility_Mode::HiddenForStreamerMode);
    TestTrue(TEXT("CVar hides diagnostics"), ck::diagnostic_visibility::Is_HiddenForStreamerMode());

    CVar->Set(0, ECVF_SetByCode);
    const auto HasLaunchOverride = FParse::Param(FCommandLine::Get(), TEXT("CkStreamerMode"));
    const auto ExpectedMode = HasLaunchOverride
                                  ? ECk_DiagnosticVisibility_Mode::HiddenForStreamerMode
                                  : ECk_DiagnosticVisibility_Mode::Visible;
    TestEqual(TEXT("Launch override remains authoritative when the CVar is disabled"),
              ck::diagnostic_visibility::Get_Mode(),
              ExpectedMode);
    TestEqual(TEXT("Hidden predicate matches the resolved visibility mode"),
              ck::diagnostic_visibility::Is_HiddenForStreamerMode(),
              HasLaunchOverride);

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

#endif // WITH_DEV_AUTOMATION_TESTS
