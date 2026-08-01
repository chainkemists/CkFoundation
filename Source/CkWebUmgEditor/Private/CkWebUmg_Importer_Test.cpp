#include "CkWebUmgEditor/CkWebUmg_Importer.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "Interfaces/IPluginManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"

#if WITH_AUTOMATION_TESTS

// Import twice from an unchanged source: the second call must be a hash-match NO-OP returning the
// SAME object with identical state — the package-level face of DECISION 3's idempotence contract.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCkWebUmg_ImportIdempotence_Test,
    "CkTests.UnitTests.CkWebUmg.ImportIdempotence",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool
    FCkWebUmg_ImportIdempotence_Test::
    RunTest(
        const FString&)
{
    const auto Plugin = IPluginManager::Get().FindPlugin(TEXT("CkFoundation"));
    if (NOT TestTrue(TEXT("plugin found"), Plugin != nullptr))
    { return false; }

    const auto JsonPath = FPaths::Combine(Plugin->GetBaseDir(),
        TEXT("Tools"), TEXT("ckwebumg-extract"), TEXT("corpus"), TEXT("golden"), TEXT("P5_images.ckui.json"));
    constexpr auto SaveToDisk = false; // in-memory only — tests never write Content

    auto* First = ck::webumg::editor::ImportPageAsset(JsonPath, TEXT("/Temp/CkWebUmgTest"), SaveToDisk);
    if (NOT TestTrue(TEXT("first import succeeds"), First != nullptr))
    { return false; }
    TestTrue(TEXT("nodes imported"), First->Get_Nodes().Num() > 0);
    TestTrue(TEXT("bundle texture imported and outered to the asset"),
        First->Get_Textures().Num() == 1
        && First->Get_Textures().begin()->Value->GetOuter() == First);

    auto* Second = ck::webumg::editor::ImportPageAsset(JsonPath, TEXT("/Temp/CkWebUmgTest"), SaveToDisk);
    TestTrue(TEXT("unchanged re-import is a no-op returning the same asset"), Second == First);
    return true;
}

#endif // WITH_AUTOMATION_TESTS
