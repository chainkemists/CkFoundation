#include "CkAssetExporter/Dispatch/CkAssetExporter_Dispatch.h"
#include "CkAssetExporter/ExportMeta/CkAssetExporter_ExportMeta.h"

#include <Dom/JsonObject.h>
#include <Engine/DataAsset.h>
#include <HAL/FileManager.h>
#include <Misc/AutomationTest.h>
#include <Misc/FileHelper.h>
#include <Misc/Paths.h>
#include <Serialization/JsonSerializer.h>
#include <Serialization/JsonWriter.h>

#if WITH_DEV_AUTOMATION_TESTS

// --------------------------------------------------------------------------------------------------------------------
// Pure-logic coverage for the dispatch layer + export-meta round-trip. No content dependencies and no env gating — the
// three cases exercise string/JSON logic only (path resolution, the friendly-class map, and sibling-meta read/write).
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

    auto ErrorUnknown = FString{};
    auto* ResolvedUnknown = FCk_AssetExporter_Dispatch::TryResolve_FriendlyClassName(TEXT("Bogus"), ErrorUnknown);
    TestNull(TEXT("unknown name returns null"), ResolvedUnknown);
    TestTrue(TEXT("error names the unknown input"), ErrorUnknown.Contains(TEXT("Bogus")));
    TestTrue(TEXT("error lists a valid name (DataAsset)"), ErrorUnknown.Contains(TEXT("DataAsset")));
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

    // A json carrying a "_meta" object.
    {
        auto Root = MakeShared<FJsonObject>();
        auto Meta = MakeShared<FJsonObject>();
        Meta->SetStringField(TEXT("sourceHash"), TEXT("abc123"));
        Meta->SetNumberField(TEXT("exporterVersion"), 7);
        Root->SetObjectField(TEXT("_meta"), Meta);
        Root->SetStringField(TEXT("assetName"), TEXT("Dummy"));
        SerializeToFile(Root, GoodPath);
    }

    // A json with NO "_meta".
    {
        auto Root = MakeShared<FJsonObject>();
        Root->SetStringField(TEXT("assetName"), TEXT("NoMeta"));
        SerializeToFile(Root, NoMetaPath);
    }

    // Present + well-formed -> reads its stored values.
    {
        auto Hash = FString{};
        auto Version = int32{0};
        const auto Read = FCk_AssetExportMeta::TryRead_SiblingMeta(GoodPath, Hash, Version);
        TestTrue(TEXT("round-trip read succeeds"), Read);
        TestEqual(TEXT("sourceHash round-trips"), Hash, TEXT("abc123"));
        TestEqual(TEXT("exporterVersion round-trips"), Version, 7);
    }

    // Missing file -> false.
    {
        auto Hash = FString{};
        auto Version = int32{0};
        TestFalse(TEXT("missing file returns false"), FCk_AssetExportMeta::TryRead_SiblingMeta(MissingPath, Hash, Version));
    }

    // Meta-less json -> false.
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

#endif
