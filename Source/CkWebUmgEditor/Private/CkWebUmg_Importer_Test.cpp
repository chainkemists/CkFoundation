#include "CkWebUmgEditor/CkWebUmg_Importer.h"
#include "CkWebUmgEditor/CkWebUmg_PageAssetFactory.h"

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

// --------------------------------------------------------------------------------------------------------------------

// The full HTML -> extractor -> bundle -> asset chain, live: extract the smoke page with the real
// Node/Chrome toolchain and import it. Gates on success + node-count agreement with the committed
// golden import; byte-identity vs the golden is reported (per-browser-version determinism means it
// can legitimately drift when Chrome updates - informational, never a gate).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCkWebUmg_HtmlImport_Test,
    "CkTests.UnitTests.CkWebUmg.HtmlImport",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool
    FCkWebUmg_HtmlImport_Test::
    RunTest(
        const FString&)
{
    const auto Plugin = IPluginManager::Get().FindPlugin(TEXT("CkFoundation"));
    if (NOT TestTrue(TEXT("plugin found"), Plugin != nullptr))
    { return false; }
    const auto CorpusDir = FPaths::Combine(Plugin->GetBaseDir(),
        TEXT("Tools"), TEXT("ckwebumg-extract"), TEXT("corpus"));

    constexpr auto SaveToDisk = false;
    auto* FromHtml = ck::webumg::editor::ImportPageAssetFromHtml(
        FPaths::Combine(CorpusDir, TEXT("smoke.html")), TEXT("/Temp/CkWebUmgHtmlTest"), SaveToDisk);
    if (NOT TestTrue(TEXT("html imports end-to-end (node + Chrome toolchain)"), FromHtml != nullptr))
    { return false; }

    auto* FromGolden = ck::webumg::editor::ImportPageAsset(
        FPaths::Combine(CorpusDir, TEXT("golden"), TEXT("smoke.ckui.json")),
        TEXT("/Temp/CkWebUmgGoldenTest"), SaveToDisk);
    if (NOT TestTrue(TEXT("golden imports"), FromGolden != nullptr))
    { return false; }

    TestTrue(TEXT("live extraction and committed golden agree on node count"),
        FromHtml->Get_Nodes().Num() == FromGolden->Get_Nodes().Num());
    AddInfo(FString::Printf(TEXT("live-vs-golden source hash: %s (equal=%d)"),
        *FromHtml->Get_SourceHash(),
        FromHtml->Get_SourceHash() == FromGolden->Get_SourceHash() ? 1 : 0));
    return true;
}

// --------------------------------------------------------------------------------------------------------------------

// The Content-Browser face: the factory imports a dropped .html through the full toolchain, the
// asset stamps its source, and the reimport handler re-runs from that stamp (unchanged = no-op).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCkWebUmg_FactoryReimport_Test,
    "CkTests.UnitTests.CkWebUmg.FactoryReimport",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool
    FCkWebUmg_FactoryReimport_Test::
    RunTest(
        const FString&)
{
    const auto Plugin = IPluginManager::Get().FindPlugin(TEXT("CkFoundation"));
    if (NOT TestTrue(TEXT("plugin found"), Plugin != nullptr))
    { return false; }
    const auto HtmlPath = FPaths::Combine(Plugin->GetBaseDir(),
        TEXT("Tools"), TEXT("ckwebumg-extract"), TEXT("corpus"), TEXT("smoke.html"));

    auto* Factory = NewObject<UCk_WebUmg_PageAssetFactory>();
    auto* Package = CreatePackage(TEXT("/Temp/CkWebUmgFactoryTest/smoke"));
    auto Cancelled = false;
    auto* Created = Factory->FactoryCreateFile(
        UCk_WebUmg_PageAsset_UE::StaticClass(), Package, TEXT("smoke"),
        RF_Public | RF_Standalone, HtmlPath, nullptr, GWarn, Cancelled);

    auto* Asset = Cast<UCk_WebUmg_PageAsset_UE>(Created);
    if (NOT TestTrue(TEXT("factory imports the dropped html"), Asset != nullptr))
    { return false; }
    TestTrue(TEXT("html source stamped for reimport"),
        Asset->Get_SourceHtmlPath().EndsWith(TEXT("smoke.html")));

    auto ReimportSources = TArray<FString>{};
    TestTrue(TEXT("reimport handler recognizes the asset"),
        Factory->CanReimport(Asset, ReimportSources)
        && ReimportSources.Num() == 1 && ReimportSources[0].EndsWith(TEXT("smoke.html")));

    TestTrue(TEXT("reimport from the stamped source succeeds (unchanged = hash no-op)"),
        Factory->Reimport(Asset) == EReimportResult::Succeeded);
    return true;
}

#endif // WITH_AUTOMATION_TESTS
