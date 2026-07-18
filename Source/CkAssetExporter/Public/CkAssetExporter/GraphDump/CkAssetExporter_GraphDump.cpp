#include "CkAssetExporter_GraphDump.h"

#include "CkAssetExporter_Log.h"
#include "CkAssetExporter/Dispatch/CkAssetExporter_DispatchInternal.h"

#include "CkCore/Macros/CkMacros.h"

#include "AssetRegistry/ARFilter.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"

#include <Misc/FileHelper.h>
#include <Misc/Paths.h>
#include <Particles/ParticleSystem.h>
#include <UObject/ObjectRedirector.h>

#include <Dom/JsonObject.h>
#include <Dom/JsonValue.h>
#include <Serialization/JsonSerializer.h>
#include <Serialization/JsonWriter.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_asset_exporter_graphdump
{
    // Queries one dependency direction (hard vs soft) as PACKAGE names, sorted lexically (FName::LexicalLess — the
    // same idiom the sweep uses), and emits them as a JSON string array. Deps outside the root are included on
    // purpose: they ARE the migration-closure frontier.
    static auto
    Get_SortedDepStrings(
        const IAssetRegistry& InRegistry,
        FName InPackageName,
        UE::AssetRegistry::EDependencyQuery InQuery)
        -> TArray<TSharedPtr<FJsonValue>>
    {
        auto Deps = TArray<FName>{};
        InRegistry.GetDependencies(InPackageName, Deps, UE::AssetRegistry::EDependencyCategory::Package, InQuery);
        Deps.Sort([](const FName& A, const FName& B) { return A.LexicalLess(B); });

        auto Values = TArray<TSharedPtr<FJsonValue>>{};
        Values.Reserve(Deps.Num());
        for (const auto& Dep : Deps)
        { Values.Add(MakeShared<FJsonValueString>(Dep.ToString())); }
        return Values;
    }

    // "cascade" for UParticleSystem-derived classes, "redirector" for UObjectRedirector — both resolved from the
    // registry-reported class WITHOUT loading the asset (EResolveClass::No via FAssetData::IsInstanceOf).
    static auto
    Get_Flags(
        const FAssetData& InAssetData)
        -> TArray<TSharedPtr<FJsonValue>>
    {
        auto Flags = TArray<TSharedPtr<FJsonValue>>{};
        if (InAssetData.IsInstanceOf(UParticleSystem::StaticClass()))
        { Flags.Add(MakeShared<FJsonValueString>(TEXT("cascade"))); }
        if (InAssetData.IsInstanceOf(UObjectRedirector::StaticClass()))
        { Flags.Add(MakeShared<FJsonValueString>(TEXT("redirector"))); }
        return Flags;
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_AssetExporter_GraphDump::
    DumpGraph(
        const FString& InPackageDir)
    -> TSharedPtr<FJsonObject>
{
    auto& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
    AssetRegistry.WaitForCompletion();

    constexpr auto ForceRescan = true;
    AssetRegistry.ScanPathsSynchronous({ InPackageDir }, ForceRescan);

    // NO class filter — the graph serves migration closure planning, so every asset under the root (art included)
    // must appear.
    auto Filter = FARFilter{};
    Filter.bRecursivePaths = true;
    Filter.PackagePaths.Add(FName{*InPackageDir});

    auto Assets = TArray<FAssetData>{};
    const auto QueryOk = AssetRegistry.GetAssets(Filter, Assets);
    if (NOT QueryOk)
    {
        ck::asset_exporter::Error(TEXT("[GraphDump] Asset enumeration failed for [{}]"), InPackageDir);
        return {};
    }

    Assets.Sort([](const FAssetData& A, const FAssetData& B)
    {
        return A.GetObjectPathString().Compare(B.GetObjectPathString(), ESearchCase::CaseSensitive) < 0;
    });

    ck::asset_exporter::Display(
        TEXT("[GraphDump] Enumerated {} asset(s) under [{}]"), Assets.Num(), InPackageDir);

    auto Rows = TArray<TSharedPtr<FJsonValue>>{};
    Rows.Reserve(Assets.Num());

    for (const auto& AssetData : Assets)
    {
        auto Row = MakeShared<FJsonObject>();
        Row->SetStringField(TEXT("assetPath"), AssetData.GetObjectPathString());
        Row->SetStringField(TEXT("class"), AssetData.AssetClassPath.GetAssetName().ToString());

        const auto DiskPath = ck_asset_exporter_dispatch::Get_PackageDiskPath(AssetData.PackageName.ToString());
        Row->SetStringField(TEXT("diskPath"), DiskPath.IsEmpty() ? DiskPath : FPaths::ConvertRelativePathToFull(DiskPath));

        Row->SetArrayField(TEXT("hardDeps"),
            ck_asset_exporter_graphdump::Get_SortedDepStrings(AssetRegistry, AssetData.PackageName, UE::AssetRegistry::EDependencyQuery::Hard));
        Row->SetArrayField(TEXT("softDeps"),
            ck_asset_exporter_graphdump::Get_SortedDepStrings(AssetRegistry, AssetData.PackageName, UE::AssetRegistry::EDependencyQuery::Soft));

        // Omit the "flags" field entirely when empty (the common case).
        const auto Flags = ck_asset_exporter_graphdump::Get_Flags(AssetData);
        if (Flags.Num() > 0)
        { Row->SetArrayField(TEXT("flags"), Flags); }

        Rows.Add(MakeShared<FJsonValueObject>(Row));
    }

    auto Root = MakeShared<FJsonObject>();
    Root->SetStringField(TEXT("root"), InPackageDir);
    Root->SetNumberField(TEXT("count"), Rows.Num());
    Root->SetArrayField(TEXT("assets"), Rows);
    return Root;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_AssetExporter_GraphDump::
    WriteGraph(
        const TSharedPtr<FJsonObject>& InGraph,
        const FString& InProjectSavedSubdir)
    -> FString
{
    if (NOT InGraph.IsValid())
    { return {}; }

    auto JsonString = FString{};
    const auto Writer = TJsonWriterFactory<>::Create(&JsonString);
    FJsonSerializer::Serialize(InGraph.ToSharedRef(), Writer);

    // Absolute on purpose: external consumers (migration planners, agents) run with their own cwd, where the
    // editor's relative "../../Saved/..." form would not resolve.
    const auto OutPath = FPaths::ConvertRelativePathToFull(
        FPaths::Combine(FPaths::ProjectSavedDir(), InProjectSavedSubdir, TEXT("graph.json")));

    if (NOT FFileHelper::SaveStringToFile(JsonString, *OutPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
    { ck::asset_exporter::Error(TEXT("[GraphDump] Failed to write graph [{}]"), OutPath); }

    return OutPath;
}

// --------------------------------------------------------------------------------------------------------------------
