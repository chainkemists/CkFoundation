#include "CkCore/Debug/CkDebugDraw_Utils.h"

#include <HAL/IConsoleManager.h>
#include <Misc/AutomationTest.h>
#include <Misc/CommandLine.h>
#include <Misc/Parse.h>
#include <Misc/ScopeExit.h>

#if WITH_DEV_AUTOMATION_TESTS

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_DebugDraw_StreamerMode,
    "Ck.CkCore.DebugDraw.StreamerMode.CVar",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool
    FCkTest_DebugDraw_StreamerMode::
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
    TestTrue(TEXT("CVar enables diagnostic draw suppression"), ck::debug_draw::Is_SuppressedForStreamerMode());

    const auto HasLaunchOverride = FParse::Param(FCommandLine::Get(), TEXT("CkStreamerMode"));
    if (NOT HasLaunchOverride)
    {
        CVar->Set(0, ECVF_SetByCode);
        TestFalse(TEXT("CVar disable restores diagnostic draw when no launch override is present"),
                  ck::debug_draw::Is_SuppressedForStreamerMode());
    }

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

#endif // WITH_DEV_AUTOMATION_TESTS
