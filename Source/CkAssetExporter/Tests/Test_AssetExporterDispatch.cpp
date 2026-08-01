#include "CkAssetExporter/Dispatch/CkAssetExporter_Dispatch.h"
#include "CkAssetExporter/ExportMeta/CkAssetExporter_ExportMeta.h"
#include "CkAssetExporter/GraphDump/CkAssetExporter_GraphDump.h"
#include "CkAssetExporter/Validation/CkAssetExporter_AutoReimportGuard.h"

#include "CkCore/Macros/CkMacros.h"

#include <Dom/JsonObject.h>
#include <Dom/JsonValue.h>
#include <Engine/DataAsset.h>
#include <Engine/DataTable.h>
#include <HAL/FileManager.h>
#include <Materials/MaterialInterface.h>
#include <Misc/AutomationTest.h>
#include <Misc/FileHelper.h>
#include <Misc/Paths.h>
#include <Serialization/JsonSerializer.h>
#include <Serialization/JsonWriter.h>

#if WITH_DEV_AUTOMATION_TESTS

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCk_AssetExporter_Dispatch_ResolveInput_Test,
    "Ck.AssetExporter.Dispatch.ResolveInput",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCk_AssetExporter_Dispatch_ResolveInput_Test::RunTest(const FString& InParameters)
{
    const auto FromPackage = FCk_AssetExporter_Dispatch::Resolve_InputToObjectPaths(TEXT("/Game/Foo/Bar"));
    TestEqual(TEXT("package path resolves to one object path"), FromPackage.Num(), 1);
    if (FromPackage.Num() == 1)
    { TestEqual(TEXT("package path gains .Name suffix"), FromPackage[0], TEXT("/Game/Foo/Bar.Bar")); }

    const auto FromObject = FCk_AssetExporter_Dispatch::Resolve_InputToObjectPaths(TEXT("/Game/Foo/Bar.Bar"));
    TestEqual(TEXT("object path resolves to one object path"), FromObject.Num(), 1);
    if (FromObject.Num() == 1)
    { TestEqual(TEXT("object path passes through unchanged"), FromObject[0], TEXT("/Game/Foo/Bar.Bar")); }

    const auto FromGarbage = FCk_AssetExporter_Dispatch::Resolve_InputToObjectPaths(TEXT("not a path"));
    TestEqual(TEXT("garbage input returns empty"), FromGarbage.Num(), 0);

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCk_AssetExporter_Dispatch_FriendlyClassMap_Test,
    "Ck.AssetExporter.Dispatch.FriendlyClassMap",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCk_AssetExporter_Dispatch_FriendlyClassMap_Test::RunTest(const FString& InParameters)
{
    auto ErrorLower = FString{};
    auto* ResolvedLower = FCk_AssetExporter_Dispatch::TryResolve_FriendlyClassName(TEXT("dataasset"), ErrorLower);
    TestNotNull(TEXT("lowercase 'dataasset' resolves case-insensitively"), ResolvedLower);
    TestTrue(TEXT("'dataasset' resolves to UDataAsset"), ResolvedLower == UDataAsset::StaticClass());

    auto ErrorDataTable = FString{};
    auto* ResolvedDataTable = FCk_AssetExporter_Dispatch::TryResolve_FriendlyClassName(TEXT("datatable"), ErrorDataTable);
    TestNotNull(TEXT("lowercase 'datatable' resolves case-insensitively"), ResolvedDataTable);
    TestTrue(TEXT("'datatable' resolves to UDataTable"), ResolvedDataTable == UDataTable::StaticClass());

    auto ErrorMaterial = FString{};
    auto* ResolvedMaterial = FCk_AssetExporter_Dispatch::TryResolve_FriendlyClassName(TEXT("material"), ErrorMaterial);
    TestNotNull(TEXT("lowercase 'material' resolves case-insensitively"), ResolvedMaterial);
    TestTrue(TEXT("'material' resolves to UMaterialInterface"), ResolvedMaterial == UMaterialInterface::StaticClass());

    auto ErrorUnknown = FString{};
    auto* ResolvedUnknown = FCk_AssetExporter_Dispatch::TryResolve_FriendlyClassName(TEXT("Bogus"), ErrorUnknown);
    TestNull(TEXT("unknown name returns null"), ResolvedUnknown);
    TestTrue(TEXT("error names the unknown input"), ErrorUnknown.Contains(TEXT("Bogus")));
    TestTrue(TEXT("error lists a valid name (DataAsset)"), ErrorUnknown.Contains(TEXT("DataAsset")));
    TestTrue(TEXT("error lists a valid name (DataTable)"), ErrorUnknown.Contains(TEXT("DataTable")));
    TestTrue(TEXT("error lists a valid name (Niagara)"), ErrorUnknown.Contains(TEXT("Niagara")));

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCk_AssetExporter_Dispatch_ExportMetaRoundTrip_Test,
    "Ck.AssetExporter.Dispatch.ExportMetaRoundTrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCk_AssetExporter_Dispatch_ExportMetaRoundTrip_Test::RunTest(const FString& InParameters)
{
    const auto TestDir = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("CkAssetExporter"), TEXT("Test"));
    constexpr auto MakeTree = true;
    IFileManager::Get().MakeDirectory(*TestDir, MakeTree);

    const auto GoodPath    = FPaths::Combine(TestDir, TEXT("meta_roundtrip.json"));
    const auto NoMetaPath  = FPaths::Combine(TestDir, TEXT("meta_absent.json"));
    const auto MissingPath = FPaths::Combine(TestDir, TEXT("meta_missing.json"));

    const auto SerializeToFile = [](const TSharedPtr<FJsonObject>& InRoot, const FString& InPath) -> void
    {
        auto JsonString = FString{};
        const auto Writer = TJsonWriterFactory<>::Create(&JsonString);
        FJsonSerializer::Serialize(InRoot.ToSharedRef(), Writer);
        FFileHelper::SaveStringToFile(JsonString, *InPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    };

    {
        auto Root = MakeShared<FJsonObject>();
        auto Meta = MakeShared<FJsonObject>();
        Meta->SetStringField(TEXT("sourceHash"), TEXT("abc123"));
        Meta->SetNumberField(TEXT("exporterVersion"), 7);
        Root->SetObjectField(TEXT("_meta"), Meta);
        Root->SetStringField(TEXT("assetName"), TEXT("Dummy"));
        SerializeToFile(Root, GoodPath);
    }

    {
        auto Root = MakeShared<FJsonObject>();
        Root->SetStringField(TEXT("assetName"), TEXT("NoMeta"));
        SerializeToFile(Root, NoMetaPath);
    }

    {
        auto Hash = FString{};
        auto Version = int32{0};
        const auto Read = FCk_AssetExportMeta::TryRead_SiblingMeta(GoodPath, Hash, Version);
        TestTrue(TEXT("round-trip read succeeds"), Read);
        TestEqual(TEXT("sourceHash round-trips"), Hash, TEXT("abc123"));
        TestEqual(TEXT("exporterVersion round-trips"), Version, 7);
    }

    {
        auto Hash = FString{};
        auto Version = int32{0};
        TestFalse(TEXT("missing file returns false"), FCk_AssetExportMeta::TryRead_SiblingMeta(MissingPath, Hash, Version));
    }

    {
        auto Hash = FString{};
        auto Version = int32{0};
        TestFalse(TEXT("meta-less json returns false"), FCk_AssetExportMeta::TryRead_SiblingMeta(NoMetaPath, Hash, Version));
    }

    constexpr auto RequireExists = false;
    IFileManager::Get().Delete(*GoodPath, RequireExists);
    IFileManager::Get().Delete(*NoMetaPath, RequireExists);

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCk_AssetExporter_Dispatch_SummaryTextBanner_Test,
    "Ck.AssetExporter.Dispatch.SummaryTextBanner",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCk_AssetExporter_Dispatch_SummaryTextBanner_Test::RunTest(const FString& InParameters)
{
    const auto Banner = FCk_AssetExportMeta::Get_SummaryTextBanner(TEXT("MyAsset"));

    TestTrue(TEXT("banner opens with the module tag"), Banner.StartsWith(TEXT("// [CkAssetExporter]")));
    TestTrue(TEXT("banner names the sibling sidecar by its real filename"), Banner.Contains(TEXT("MyAsset.ckexport")));
    TestTrue(TEXT("banner points at _meta"), Banner.Contains(TEXT("\"_meta\"")));
    TestTrue(TEXT("banner ends with a blank separator line"), Banner.EndsWith(TEXT("\n\n")));

    TestEqual(TEXT("banner is deterministic"), Banner, FCk_AssetExportMeta::Get_SummaryTextBanner(TEXT("MyAsset")));

    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// The load-bearing invariant of the whole sidecar-extension design: an extension no import factory claims is rejected
// by FMatchRules::IsFileApplicable BEFORE any wildcard rule, which is what makes the sidecar corpus invisible to the
// auto-reimport monitor in every consuming project with zero configuration. If someone ever changes the extension to
// something a factory does claim, the editor silently drops to ~18 fps on a large corpus and no UE-side profiler can
// attribute it. This test is the only thing standing between that change and that outcome.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCk_AssetExporter_Dispatch_SidecarExtensionUnclaimed_Test,
    "Ck.AssetExporter.Dispatch.SidecarExtensionUnclaimed",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCk_AssetExporter_Dispatch_SidecarExtensionUnclaimed_Test::RunTest(const FString& InParameters)
{
    const auto FactoryExtensions = FCk_AssetExporter_AutoReimportGuard::Get_AllFactoryExtensions();

    TestTrue(TEXT("factory extension enumeration returned something"), NOT FactoryExtensions.IsEmpty());

    // Control assertion: json IS a claimed import format (UReimportDataTableFactory) — that is precisely why the
    // sidecar had to stop being a .json. Without this, a broken enumeration would let the real assertion below pass
    // vacuously and the test would guard nothing.
    TestTrue(TEXT("control: 'json' is claimed by an import factory"),
        FactoryExtensions.Contains(TEXT("json;"), ESearchCase::IgnoreCase));

    const auto SidecarWithoutDot = ck::asset_exporter::extension::Sidecar.RightChop(1);
    TestTrue(TEXT("sidecar extension is a terminal extension starting with a dot"),
        ck::asset_exporter::extension::Sidecar.StartsWith(TEXT(".")) && NOT SidecarWithoutDot.Contains(TEXT(".")));

    TestFalse(TEXT("NO import factory claims the sidecar extension"),
        FactoryExtensions.Contains(SidecarWithoutDot + TEXT(";"), ESearchCase::IgnoreCase));

    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// Needs no env gating: /Game/BusterBlock/MembershipBoard is a tiny content dir that is always present.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCk_AssetExporter_Dispatch_GraphDumpShape_Test,
    "Ck.AssetExporter.Dispatch.GraphDumpShape",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCk_AssetExporter_Dispatch_GraphDumpShape_Test::RunTest(const FString& InParameters)
{
    const auto Graph = FCk_AssetExporter_GraphDump::DumpGraph(TEXT("/Game/BusterBlock/MembershipBoard"));
    TestTrue(TEXT("graph object is valid"), Graph.IsValid());
    if (NOT Graph.IsValid())
    { return false; }

    const TArray<TSharedPtr<FJsonValue>>* Assets = nullptr;
    const auto HasAssets = Graph->TryGetArrayField(TEXT("assets"), Assets) && Assets != nullptr;
    TestTrue(TEXT("graph has an 'assets' array"), HasAssets);
    if (NOT HasAssets)
    { return false; }

    TestTrue(TEXT("assets array has at least one row"), Assets->Num() >= 1);

    for (const auto& AssetValue : *Assets)
    {
        const auto Row = AssetValue.IsValid() ? AssetValue->AsObject() : nullptr;
        TestTrue(TEXT("asset row is an object"), Row.IsValid());
        if (NOT Row.IsValid())
        { continue; }

        auto AssetPath = FString{};
        TestTrue(TEXT("row has non-empty assetPath"),
            Row->TryGetStringField(TEXT("assetPath"), AssetPath) && NOT AssetPath.IsEmpty());

        auto ClassName = FString{};
        TestTrue(TEXT("row has non-empty class"),
            Row->TryGetStringField(TEXT("class"), ClassName) && NOT ClassName.IsEmpty());

        auto DiskPath = FString{};
        const auto HasDisk = Row->TryGetStringField(TEXT("diskPath"), DiskPath);
        TestTrue(TEXT("row has diskPath"), HasDisk);

        const auto HasDriveLetterPrefix = DiskPath.Len() >= 2 && FChar::IsAlpha(DiskPath[0]) && DiskPath[1] == TEXT(':');
        TestTrue(TEXT("diskPath is absolute (drive-letter prefixed)"), HasDriveLetterPrefix);

        const TArray<TSharedPtr<FJsonValue>>* HardDeps = nullptr;
        if (Row->TryGetArrayField(TEXT("hardDeps"), HardDeps) && HardDeps != nullptr)
        {
            auto AsStrings = TArray<FString>{};
            for (const auto& Dep : *HardDeps)
            {
                auto DepString = FString{};
                if (Dep.IsValid() && Dep->TryGetString(DepString))
                { AsStrings.Add(DepString); }
            }

            auto Sorted = AsStrings;
            Sorted.Sort([](const FString& A, const FString& B) { return FName(*A).LexicalLess(FName(*B)); });
            TestTrue(TEXT("hardDeps array is lexically sorted"), AsStrings == Sorted);
        }
    }

    return true;
}

#endif
