#include "CkAngelscriptGenerator/Assets/CkAssetRegistrySubsystem.h"
#include "CkAngelscriptGenerator/SelfHeal/CkAngelscriptGenerator_AsSourceScanner.h"

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Reference/CkAssetReferenceProvider.h"

#include "Editor.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"

#if WITH_DEV_AUTOMATION_TESTS

using namespace ck::angelscriptgenerator::self_heal;

namespace ck_test_asset_reference_tracking
{
    // Referenced from Plugins/CkTests/Script/CkIskmRenderer/CkIskmRenderer_Assets.as as
    // `assets::load::SK_Mannequin()`. Nothing in the PROJECT's own Script/ tree mentions it, which is
    // exactly what makes it a probe for whether plugin script roots are scanned at all.
    const auto PluginReferencedAsset = FString{
        TEXT("/CkTests/Characters/Mannequins/Meshes/SK_Mannequin.SK_Mannequin")};

    auto Get_Subsystem() -> UCkAssetRegistrySubsystem*
    {
        return GEditor != nullptr ? GEditor->GetEditorSubsystem<UCkAssetRegistrySubsystem>() : nullptr;
    }
}

using namespace ck_test_asset_reference_tracking;

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_AssetReferenceTracking_ScanRootsCoverPlugins,
    "CkAngelscriptGenerator.UnitTests.AssetReferenceTracking.ScanRootsCoverPlugins",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_AssetReferenceTracking_ScanRootsCoverPlugins::RunTest(const FString&)
{
    // The usage scan used to enumerate only ProjectDir()/Script, which in a plugin-hosted layout is
    // a handful of files while the AngelScript that actually references assets lives under Plugins/.
    const auto Roots = FCkAsSourceScanner::Get_DefaultScanRoots();
    TestTrue(TEXT("at least one scan root exists"), Roots.Num() > 0);

    const auto HasPluginRoot = Roots.ContainsByPredicate([](const FString& InRoot)
    {
        return InRoot.Contains(TEXT("/Plugins/")) || InRoot.Contains(TEXT("\\Plugins\\"));
    });
    TestTrue(TEXT("scan roots include at least one plugin Script/ directory"), HasPluginRoot);

    const auto Files = FCkAsSourceScanner::Enumerate_AsSourceFiles(Roots);
    TestTrue(TEXT("enumeration finds .as files"), Files.Num() > 0);

    const auto PluginFileCount = Files.FilterByPredicate([](const FString& InPath)
    {
        return InPath.Contains(TEXT("/Plugins/")) || InPath.Contains(TEXT("\\Plugins\\"));
    }).Num();

    // The pre-fix scan saw 3 files in this project. Anything in that neighbourhood means the plugin
    // trees are still invisible.
    TestTrue(FString::Printf(
        TEXT("enumeration reaches plugin scripts (found %d of %d under Plugins/)"), PluginFileCount, Files.Num()),
        PluginFileCount > 0);

    const auto GeneratedLeaked = Files.ContainsByPredicate([](const FString& InPath)
    {
        auto Normalized = InPath;
        FPaths::NormalizeFilename(Normalized);
        return Normalized.Contains(TEXT("/Generated/"));
    });
    TestFalse(TEXT("generated accessor files are excluded from the usage scan"), GeneratedLeaked);

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_AssetReferenceTracking_PluginReferencedAssetIsReported,
    "CkAngelscriptGenerator.Integration.AssetReferenceTracking.PluginReferencedAssetIsReported",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_AssetReferenceTracking_PluginReferencedAssetIsReported::RunTest(const FString&)
{
    // The contract the whole reference-tracking feature exists to keep: an asset reached only through
    // a generated `assets::` accessor creates no package-dependency edge, so if this subsystem does
    // not report it, an auditor will offer a script-critical asset for deletion.
    auto* Subsystem = Get_Subsystem();
    TestNotNull(TEXT("asset registry subsystem is available"), Subsystem);
    if (Subsystem == nullptr)
    { return false; }

    TestTrue(TEXT("a script-reference provider is registered"),
        FCk_AssetReferenceProviderRegistry::Get().Get_HasAnyProvider());

    const auto Referencers = Subsystem->Get_ScriptReferencersOfAsset(FSoftObjectPath{PluginReferencedAsset});

    TestTrue(FString::Printf(
        TEXT("'%s' is reported as referenced from AngelScript (got %d referencer(s))"),
        *PluginReferencedAsset, Referencers.Num()),
        Referencers.Num() > 0);

    if (Referencers.IsEmpty())
    { return false; }

    // Get_ScriptReferencersOfAsset returns PROJECT-RELATIVE paths, so a plugin file reads as
    // `Plugins/CkTests/Script/...` with no leading separator.
    const auto FromPlugin = Referencers.ContainsByPredicate([](const FString& InPath)
    {
        auto Normalized = InPath;
        FPaths::NormalizeFilename(Normalized);
        return Normalized.StartsWith(TEXT("Plugins/")) || Normalized.Contains(TEXT("/Plugins/"));
    });
    TestTrue(FString::Printf(TEXT("and the referencing file is under a plugin's Script/ tree (first: '%s')"),
        *Referencers[0]), FromPlugin);

    // A path nothing references must still come back empty — the report has to discriminate, not
    // just always answer yes.
    const auto Absent = Subsystem->Get_ScriptReferencersOfAsset(
        FSoftObjectPath{TEXT("/Game/DoesNotExist/T_Absent.T_Absent")});
    TestEqual(TEXT("an unreferenced path reports no referencers"), Absent.Num(), 0);

    return true;
}

#endif

// --------------------------------------------------------------------------------------------------------------------
