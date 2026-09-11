#include "CkCore/IO/CkDeferredAssetInit_AngelScript.h"

#include "CkCore/Format/CkFormat.h"

#include <Misc/AutomationTest.h>

#if WITH_ANGELSCRIPT_CK
#include <AngelscriptManager.h>
#endif

#if WITH_DEV_AUTOMATION_TESTS && WITH_ANGELSCRIPT_CK

// --------------------------------------------------------------------------------------------------------------------
// The editor is a SOURCE boot. Phase 2's cache-boot fallback must key on the module registry being empty, which only a
// precompiled-cache boot produces - not on surgical attribution having declared no literal, which a source boot
// reaches whenever a CDO default deferred a load and no literal body did. Keyed on the latter, that boot re-runs
// every captured literal init: the full heal the surgical mode exists to avoid.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_DeferredAssetInit_SourceBootSurgicalHealSkipsPreClearCapture,
    "Ck.CkCore.IO.DeferredAssetInit.SourceBootSurgicalHealSkipsPreClearCapture",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool
    FCkTest_DeferredAssetInit_SourceBootSurgicalHealSkipsPreClearCapture::
    RunTest(
        const FString& Parameters)
{
    const auto ActiveModuleCount = FAngelscriptManager::Get().GetActiveModules().Num();
    if (NOT TestTrue(TEXT("The AS module registry is alive, so this is a source boot"), ActiveModuleCount > 0))
    { return false; }

    // With nothing captured, a wrongly taken fallback would also re-run nothing and this test could not see it.
    const auto CapturedLiteralCount = UCk_DeferredAssetInit_UE::Get_PreClearCapturedLiteralCount_ForTests();
    if (NOT TestTrue(TEXT("The pre-clear capture holds literal inits for a wrongly taken fallback to re-run"),
                     CapturedLiteralCount > 0))
    { return false; }

    const auto ReRunCount = UCk_DeferredAssetInit_UE::Run_SurgicalLiteralHealWithNoAttribution_ForTests();
    TestEqual(
        ck::Format_UE(TEXT("Literal inits attempted by a source boot with no attributed literal (the capture holds [{}])"),
                      CapturedLiteralCount),
        ReRunCount, 0);

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

#endif
