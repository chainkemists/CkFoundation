#include "CkAssetExporter_Dispatch.h"

#include "CkAssetExporter_Log.h"
#include "CkAssetExporter/BehaviorTreeExporter/CkBehaviorTreeExporter.h"
#include "CkAssetExporter/BlueprintExporter/CkBlueprintExporter.h"
#include "CkAssetExporter/CascadeExporter/CkCascadeExporter.h"
#include "CkAssetExporter/DataAssetExporter/CkDataAssetExporter.h"
#include "CkAssetExporter/DataTableExporter/CkDataTableExporter.h"
#include "CkAssetExporter/Dispatch/CkAssetExporter_DispatchInternal.h"
#include "CkAssetExporter/EQSExporter/CkEQSExporter.h"
#include "CkAssetExporter/ExportMeta/CkAssetExporter_ExportMeta.h"
#include "CkAssetExporter/NiagaraExporter/CkNiagaraExporter.h"
#include "CkAssetExporter/StateTreeExporter/CkStateTreeExporter.h"
#include "CkAssetExporter/UserDefinedEnumExporter/CkUserDefinedEnumExporter.h"
#include "CkAssetExporter/UserDefinedStructExporter/CkUserDefinedStructExporter.h"

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Validation/CkIsValid.h"

#include "AssetRegistry/ARFilter.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"

#include <BehaviorTree/BehaviorTree.h>
#include <Engine/Blueprint.h>
#include <Engine/DataAsset.h>
#include <Engine/DataTable.h>
#include <Engine/UserDefinedEnum.h>
#include <EnvironmentQuery/EnvQuery.h>
#include <HAL/FileManager.h>
#include <Misc/FileHelper.h>
#include <Misc/PackageName.h>
#include <Misc/Paths.h>
#include <NiagaraSystem.h>
#include <Particles/ParticleSystem.h>
#include <StateTree.h>
#include <StructUtils/UserDefinedStruct.h>
#include <UObject/Object.h>
#include <UObject/Package.h>
#include <UObject/UObjectGlobals.h>

#include <Dom/JsonObject.h>
#include <Dom/JsonValue.h>
#include <Serialization/JsonSerializer.h>
#include <Serialization/JsonWriter.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_asset_exporter_dispatch
{
    // Loading a directory sweep's worth of assets in one editor/commandlet session — collect periodically so the
    // working set stays bounded (VfxCorpus precedent).
    constexpr auto GcInterval = int32{100};

    // Ordered friendly-name -> class table. Order is display-only; per-asset routing (Route) enforces most-derived
    // dispatch independently.
    static auto
    Get_FriendlyClassTable()
        -> TArray<TPair<FString, UClass*>>
    {
        return
        {
            { TEXT("DataAsset"),    UDataAsset::StaticClass() },
            { TEXT("DataTable"),    UDataTable::StaticClass() },
            { TEXT("Blueprint"),    UBlueprint::StaticClass() },
            { TEXT("BehaviorTree"), UBehaviorTree::StaticClass() },
            { TEXT("EQS"),          UEnvQuery::StaticClass() },
            { TEXT("StateTree"),    UStateTree::StaticClass() },
            { TEXT("Enum"),         UUserDefinedEnum::StaticClass() },
            { TEXT("Struct"),       UUserDefinedStruct::StaticClass() },
            { TEXT("Niagara"),      UNiagaraSystem::StaticClass() },
            { TEXT("Cascade"),      UParticleSystem::StaticClass() },
        };
    }

    // Copies the common result fields (every per-type export result shares this shape) into a dispatch entry row.
    template <typename TResult>
    static auto
    Fill_Entry(
        FCk_AssetExportDispatchEntryResult& InOutEntry,
        const TResult& InResult)
        -> void
    {
        InOutEntry.Succeeded = InResult.Succeeded;
        InOutEntry.JsonFilePath = InResult.JsonFilePath;
        InOutEntry.TextFilePath = InResult.TextFilePath;
        InOutEntry.ErrorMessage = InResult.ErrorMessage;
    }

    // DataTable result carries a CsvFilePath instead of a TextFilePath — a non-template overload (preferred over the
    // template on exact match) maps the csv sibling into the entry's TextFilePath slot: the manifest 'txt' column
    // carries the csv path for DataTables.
    static auto
    Fill_Entry(
        FCk_AssetExportDispatchEntryResult& InOutEntry,
        const FCk_DataTableExportResult& InResult)
        -> void
    {
        InOutEntry.Succeeded = InResult.Succeeded;
        InOutEntry.JsonFilePath = InResult.JsonFilePath;
        InOutEntry.TextFilePath = InResult.CsvFilePath;
        InOutEntry.ErrorMessage = InResult.ErrorMessage;
    }

    // True (and stamps the entry as a skip) when skip-fresh is on and the sibling json is up to date for InVersion.
    static auto
    Is_FreshAndSkip(
        const UObject* InAsset,
        bool InSkipFresh,
        int32 InVersion,
        FCk_AssetExportDispatchEntryResult& InOutEntry)
        -> bool
    {
        if (NOT InSkipFresh)
        { return false; }

        const auto SiblingJson = FCk_AssetExporter_Dispatch::Get_SiblingJsonPathForAsset(InAsset);
        if (SiblingJson.IsEmpty())
        { return false; }

        if (NOT FCk_AssetExportMeta::Get_IsExportFresh(InAsset, SiblingJson, InVersion))
        { return false; }

        InOutEntry.Succeeded = true;
        InOutEntry.Skipped = true;
        InOutEntry.JsonFilePath = SiblingJson;
        return true;
    }

    // Classify InAsset most-derived-first and export via its dedicated exporter. Mirrors the exclusion order in
    // CkDataAssetExporter_AssetAction::DoHasDedicatedExporter — the generic UDataAsset exporter is LAST so the
    // DataAsset-derived EQS/StateTree types reach their structure-aware exporters first.
    static auto
    Route(
        UObject* InAsset,
        bool InSkipFresh,
        FCk_AssetExportDispatchEntryResult& InOutEntry)
        -> void
    {
        InOutEntry.AssetClass = InAsset->GetClass()->GetName();

        if (auto* BehaviorTree = Cast<UBehaviorTree>(InAsset))
        {
            if (Is_FreshAndSkip(InAsset, InSkipFresh, ck::asset_exporter::version::BehaviorTree, InOutEntry))
            { return; }
            Fill_Entry(InOutEntry, FCk_BehaviorTreeExporter::ExportBehaviorTree(BehaviorTree));
            return;
        }
        if (auto* Query = Cast<UEnvQuery>(InAsset))
        {
            if (Is_FreshAndSkip(InAsset, InSkipFresh, ck::asset_exporter::version::EQS, InOutEntry))
            { return; }
            Fill_Entry(InOutEntry, FCk_EQSExporter::ExportEQS(Query));
            return;
        }
        if (auto* StateTree = Cast<UStateTree>(InAsset))
        {
            if (Is_FreshAndSkip(InAsset, InSkipFresh, ck::asset_exporter::version::StateTree, InOutEntry))
            { return; }
            Fill_Entry(InOutEntry, FCk_StateTreeExporter::ExportStateTree(StateTree));
            return;
        }
        if (auto* Enum = Cast<UUserDefinedEnum>(InAsset))
        {
            if (Is_FreshAndSkip(InAsset, InSkipFresh, ck::asset_exporter::version::UserDefinedEnum, InOutEntry))
            { return; }
            Fill_Entry(InOutEntry, FCk_UserDefinedEnumExporter::ExportUserDefinedEnum(Enum));
            return;
        }
        if (auto* Struct = Cast<UUserDefinedStruct>(InAsset))
        {
            if (Is_FreshAndSkip(InAsset, InSkipFresh, ck::asset_exporter::version::UserDefinedStruct, InOutEntry))
            { return; }
            Fill_Entry(InOutEntry, FCk_UserDefinedStructExporter::ExportUserDefinedStruct(Struct));
            return;
        }
        if (auto* Blueprint = Cast<UBlueprint>(InAsset)) // covers UWidgetBlueprint
        {
            if (Is_FreshAndSkip(InAsset, InSkipFresh, ck::asset_exporter::version::Blueprint, InOutEntry))
            { return; }
            Fill_Entry(InOutEntry, FCk_BlueprintExporter::ExportBlueprint(Blueprint));
            return;
        }
        if (auto* NiagaraSystem = Cast<UNiagaraSystem>(InAsset)) // no versioned meta — never skips
        {
            Fill_Entry(InOutEntry, FCk_NiagaraExporter::ExportNiagaraSystem(NiagaraSystem));
            return;
        }
        if (auto* CascadeSystem = Cast<UParticleSystem>(InAsset)) // no versioned meta — never skips
        {
            Fill_Entry(InOutEntry, FCk_CascadeExporter::ExportParticleSystem(CascadeSystem));
            return;
        }
        if (auto* DataTable = Cast<UDataTable>(InAsset)) // dedicated type — NOT a UDataAsset, but keep dedicated-first order
        {
            if (Is_FreshAndSkip(InAsset, InSkipFresh, ck::asset_exporter::version::DataTable, InOutEntry))
            { return; }
            Fill_Entry(InOutEntry, FCk_DataTableExporter::ExportDataTable(DataTable));
            return;
        }
        if (auto* DataAsset = Cast<UDataAsset>(InAsset)) // generic, LAST among the DataAsset family
        {
            if (Is_FreshAndSkip(InAsset, InSkipFresh, ck::asset_exporter::version::DataAsset, InOutEntry))
            { return; }
            Fill_Entry(InOutEntry, FCk_DataAssetExporter::ExportDataAsset(DataAsset));
            return;
        }

        InOutEntry.Succeeded = false;
        InOutEntry.ErrorMessage = ck::Format_UE(TEXT("No exporter for class [{}]"), InAsset->GetClass()->GetName());
    }

    static auto
    Tally_Entry(
        FCk_AssetExportDispatchSummary& InOutSummary,
        FCk_AssetExportDispatchEntryResult InEntry)
        -> void
    {
        if (InEntry.Skipped)
        { ++InOutSummary.Skipped; }
        else if (InEntry.Succeeded)
        { ++InOutSummary.Exported; }
        else
        { ++InOutSummary.Failed; }

        InOutSummary.Entries.Add(MoveTemp(InEntry));
    }

    // Definition of the shared helper declared in CkAssetExporter_DispatchInternal.h (external linkage — GraphDump
    // reuses this one definition). Best-effort on-disk file path for a package name (list/graph display only — never
    // loads). Prefers the .uasset, then .umap; falls back to the .uasset guess when neither exists.
    auto
    Get_PackageDiskPath(
        const FString& InPackageName)
        -> FString
    {
        auto Base = FString{};
        if (NOT FPackageName::TryConvertLongPackageNameToFilename(InPackageName, Base))
        { return {}; }

        const auto AssetFile = Base + FPackageName::GetAssetPackageExtension();
        if (IFileManager::Get().FileExists(*AssetFile))
        { return AssetFile; }

        const auto MapFile = Base + FPackageName::GetMapPackageExtension();
        if (IFileManager::Get().FileExists(*MapFile))
        { return MapFile; }

        return AssetFile;
    }

    // Resolves a friendly-filter list to concrete classes (empty list = every supported type). Returns false + sets
    // OutError when a name is unknown.
    static auto
    Resolve_ClassFilters(
        const TArray<FString>& InClassFilters,
        TArray<UClass*>& OutClasses,
        FString& OutError)
        -> bool
    {
        if (InClassFilters.Num() == 0)
        {
            for (const auto& Pair : Get_FriendlyClassTable())
            { OutClasses.Add(Pair.Value); }
            return true;
        }

        for (const auto& FilterName : InClassFilters)
        {
            auto* Class = FCk_AssetExporter_Dispatch::TryResolve_FriendlyClassName(FilterName, OutError);
            if (Class == nullptr)
            { return false; }
            OutClasses.Add(Class);
        }
        return true;
    }

    // Builds the recursive ARFilter and syncs the registry so a long-lived server sees newly written assets.
    static auto
    Query_Assets(
        const FString& InPackageDir,
        const TArray<UClass*>& InClasses,
        TArray<FAssetData>& OutAssets)
        -> void
    {
        auto& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
        AssetRegistry.WaitForCompletion();

        constexpr auto ForceRescan = true;
        AssetRegistry.ScanPathsSynchronous({ InPackageDir }, ForceRescan);

        auto Filter = FARFilter{};
        Filter.bRecursivePaths = true;
        Filter.bRecursiveClasses = true;
        Filter.PackagePaths.Add(FName{*InPackageDir});
        for (const auto* Class : InClasses)
        { Filter.ClassPaths.Add(Class->GetClassPathName()); }

        AssetRegistry.GetAssets(Filter, OutAssets);
        OutAssets.Sort([](const FAssetData& A, const FAssetData& B) { return A.PackageName.LexicalLess(B.PackageName); });
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_AssetExporter_Dispatch::
    TryResolve_FriendlyClassName(
        const FString& InName,
        FString& OutError)
    -> UClass*
{
    const auto Table = ck_asset_exporter_dispatch::Get_FriendlyClassTable();

    for (const auto& Entry : Table)
    {
        if (InName.Equals(Entry.Key, ESearchCase::IgnoreCase))
        { return Entry.Value; }
    }

    auto ValidNames = TArray<FString>{};
    for (const auto& Entry : Table)
    { ValidNames.Add(Entry.Key); }

    OutError = ck::Format_UE(TEXT("Unknown class filter [{}]. Valid names: {}"), InName, FString::Join(ValidNames, TEXT(", ")));
    return nullptr;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_AssetExporter_Dispatch::
    Get_SiblingJsonPathForAsset(
        const UObject* InAsset)
    -> FString
{
    if (ck::Is_NOT_Valid(InAsset))
    { return {}; }

    const auto PackageName = InAsset->GetOutermost()->GetName();

    auto DiskPath = FString{};
    if (NOT FPackageName::TryConvertLongPackageNameToFilename(PackageName, DiskPath))
    { return {}; }

    return DiskPath + TEXT(".json");
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_AssetExporter_Dispatch::
    Get_IsExportableClass(
        const UClass* InClass)
    -> bool
{
    if (InClass == nullptr)
    { return false; }

    for (const auto& Entry : ck_asset_exporter_dispatch::Get_FriendlyClassTable())
    {
        if (InClass->IsChildOf(Entry.Value))
        { return true; }
    }
    return false;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_AssetExporter_Dispatch::
    Resolve_InputToObjectPaths(
        const FString& InRaw)
    -> TArray<FString>
{
    auto Result = TArray<FString>{};

    auto Raw = InRaw;
    Raw.TrimStartAndEndInline();
    if (Raw.IsEmpty())
    { return Result; }

    const auto Normalized = Raw.Replace(TEXT("\\"), TEXT("/"));

    const auto PackagePathToObjectPath = [](const FString& InPackagePath) -> FString
    {
        auto AssetName = FString{};
        InPackagePath.Split(TEXT("/"), nullptr, &AssetName, ESearchCase::CaseSensitive, ESearchDir::FromEnd);
        if (AssetName.IsEmpty())
        { return InPackagePath; }
        return InPackagePath + TEXT(".") + AssetName;
    };

    // Disk path? (drive-letter prefix, or an explicit .uasset/.umap extension)
    const auto LooksLikeDiskPath =
        (Normalized.Len() >= 2 && FChar::IsAlpha(Normalized[0]) && Normalized[1] == TEXT(':')) ||
        Normalized.EndsWith(TEXT(".uasset")) ||
        Normalized.EndsWith(TEXT(".umap"));

    if (LooksLikeDiskPath)
    {
        auto PackageName = FString{};
        auto FailureReason = FString{};
        if (NOT FPackageName::TryConvertFilenameToLongPackageName(Normalized, PackageName, &FailureReason))
        {
            ck::asset_exporter::Warning(
                TEXT("[Dispatch] Could not convert disk path [{}] to a package name: {}"), InRaw, FailureReason);
            return Result;
        }
        Result.Add(PackagePathToObjectPath(PackageName));
        return Result;
    }

    if (NOT Raw.StartsWith(TEXT("/")))
    {
        ck::asset_exporter::Warning(
            TEXT("[Dispatch] Input [{}] is neither a /Game-style path nor a Content disk path — ignored"), InRaw);
        return Result;
    }

    // A UE object path carries a '.' in its final segment (Package.AssetName); a package path does not.
    auto LastSegment = FString{};
    Raw.Split(TEXT("/"), nullptr, &LastSegment, ESearchCase::CaseSensitive, ESearchDir::FromEnd);

    if (LastSegment.Contains(TEXT(".")))
    { Result.Add(Raw); }
    else
    { Result.Add(PackagePathToObjectPath(Raw)); }

    return Result;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_AssetExporter_Dispatch::
    Resolve_TokensToObjectPaths(
        const TArray<FString>& InTokens,
        FCk_AssetExportDispatchSummary& InOutSummary)
    -> TArray<FString>
{
    auto ObjectPaths = TArray<FString>{};

    for (const auto& Token : InTokens)
    {
        const auto Resolved = Resolve_InputToObjectPaths(Token);
        if (Resolved.IsEmpty())
        {
            auto Entry = FCk_AssetExportDispatchEntryResult{};
            Entry.AssetPath = Token;
            Entry.Succeeded = false;
            Entry.ErrorMessage = TEXT("Unresolvable input token (not an object path, package path, or Content disk path)");
            ++InOutSummary.Failed;
            InOutSummary.Entries.Add(MoveTemp(Entry));
            continue;
        }
        ObjectPaths.Append(Resolved);
    }

    return ObjectPaths;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_AssetExporter_Dispatch::
    ExportAssets(
        const TArray<FString>& InObjectPaths,
        bool InSkipFresh)
    -> FCk_AssetExportDispatchSummary
{
    auto Summary = FCk_AssetExportDispatchSummary{};
    Summary.Succeeded = true;

    auto LoadCounter = int32{0};

    for (const auto& ObjectPath : InObjectPaths)
    {
        auto Entry = FCk_AssetExportDispatchEntryResult{};
        Entry.AssetPath = ObjectPath;

        auto* Asset = LoadObject<UObject>(nullptr, *ObjectPath);
        if (ck::Is_NOT_Valid(Asset))
        {
            Entry.Succeeded = false;
            Entry.ErrorMessage = ck::Format_UE(TEXT("Failed to load object [{}]"), ObjectPath);
            ck::asset_exporter::Warning(TEXT("[Dispatch] {}"), Entry.ErrorMessage);
        }
        else
        {
            ck_asset_exporter_dispatch::Route(Asset, InSkipFresh, Entry);
        }

        ck_asset_exporter_dispatch::Tally_Entry(Summary, MoveTemp(Entry));

        if (++LoadCounter % ck_asset_exporter_dispatch::GcInterval == 0)
        { CollectGarbage(RF_NoFlags); }
    }

    return Summary;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_AssetExporter_Dispatch::
    SweepDirectory(
        const FString& InPackageDir,
        const TArray<FString>& InClassFilters,
        bool InSkipFresh)
    -> FCk_AssetExportDispatchSummary
{
    auto Summary = FCk_AssetExportDispatchSummary{};

    auto FilterClasses = TArray<UClass*>{};
    auto FilterError = FString{};
    if (NOT ck_asset_exporter_dispatch::Resolve_ClassFilters(InClassFilters, FilterClasses, FilterError))
    {
        Summary.Succeeded = false;
        Summary.ErrorMessage = FilterError;
        ck::asset_exporter::Error(TEXT("[Dispatch] {}"), FilterError);
        return Summary;
    }

    auto Assets = TArray<FAssetData>{};
    ck_asset_exporter_dispatch::Query_Assets(InPackageDir, FilterClasses, Assets);

    ck::asset_exporter::Display(
        TEXT("[Dispatch] Sweep of [{}] matched {} asset(s)"), InPackageDir, Assets.Num());

    Summary.Succeeded = true;

    auto LoadCounter = int32{0};

    for (const auto& AssetData : Assets)
    {
        auto Entry = FCk_AssetExportDispatchEntryResult{};
        Entry.AssetPath = AssetData.GetObjectPathString();

        auto* Asset = AssetData.GetAsset();
        if (ck::Is_NOT_Valid(Asset))
        {
            Entry.Succeeded = false;
            Entry.ErrorMessage = ck::Format_UE(TEXT("Failed to load object [{}]"), Entry.AssetPath);
            ck::asset_exporter::Warning(TEXT("[Dispatch] {}"), Entry.ErrorMessage);
        }
        else
        {
            ck_asset_exporter_dispatch::Route(Asset, InSkipFresh, Entry);
        }

        ck_asset_exporter_dispatch::Tally_Entry(Summary, MoveTemp(Entry));

        if (++LoadCounter % ck_asset_exporter_dispatch::GcInterval == 0)
        { CollectGarbage(RF_NoFlags); }
    }

    return Summary;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_AssetExporter_Dispatch::
    ListAssets(
        const FString& InPackageDir,
        const TArray<FString>& InClassFilters)
    -> TSharedPtr<FJsonObject>
{
    auto Root = MakeShared<FJsonObject>();

    auto FilterClasses = TArray<UClass*>{};
    auto FilterError = FString{};
    if (NOT ck_asset_exporter_dispatch::Resolve_ClassFilters(InClassFilters, FilterClasses, FilterError))
    {
        Root->SetBoolField(TEXT("ok"), false);
        Root->SetStringField(TEXT("error"), FilterError);
        ck::asset_exporter::Error(TEXT("[Dispatch] {}"), FilterError);
        return Root;
    }

    auto Assets = TArray<FAssetData>{};
    ck_asset_exporter_dispatch::Query_Assets(InPackageDir, FilterClasses, Assets);

    auto Rows = TArray<TSharedPtr<FJsonValue>>{};
    for (const auto& AssetData : Assets)
    {
        auto Row = MakeShared<FJsonObject>();
        Row->SetStringField(TEXT("assetPath"), AssetData.GetObjectPathString());
        Row->SetStringField(TEXT("class"), AssetData.AssetClassPath.GetAssetName().ToString());
        const auto DiskPath = ck_asset_exporter_dispatch::Get_PackageDiskPath(AssetData.PackageName.ToString());
        Row->SetStringField(TEXT("diskPath"), DiskPath.IsEmpty() ? DiskPath : FPaths::ConvertRelativePathToFull(DiskPath));
        Rows.Add(MakeShared<FJsonValueObject>(Row));
    }

    Root->SetBoolField(TEXT("ok"), true);
    Root->SetNumberField(TEXT("count"), Rows.Num());
    Root->SetArrayField(TEXT("assets"), Rows);
    return Root;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_AssetExporter_Dispatch::
    WriteManifest(
        const FCk_AssetExportDispatchSummary& InSummary,
        const FString& InProjectSavedSubdir)
    -> FString
{
    auto Root = MakeShared<FJsonObject>();
    Root->SetBoolField(TEXT("ok"), InSummary.Succeeded);
    Root->SetNumberField(TEXT("exported"), InSummary.Exported);
    Root->SetNumberField(TEXT("skipped"), InSummary.Skipped);
    Root->SetNumberField(TEXT("failed"), InSummary.Failed);

    // Written paths are absolute: manifest consumers (agents, wrappers) run with their own cwd, where the editor's
    // relative "../../Content/..." form would not resolve.
    const auto ToFull = [](const FString& InPath) -> FString
    {
        return InPath.IsEmpty() ? InPath : FPaths::ConvertRelativePathToFull(InPath);
    };

    auto Entries = TArray<TSharedPtr<FJsonValue>>{};
    for (const auto& Entry : InSummary.Entries)
    {
        auto Obj = MakeShared<FJsonObject>();
        Obj->SetStringField(TEXT("asset"), Entry.AssetPath);
        Obj->SetStringField(TEXT("class"), Entry.AssetClass);
        Obj->SetBoolField(TEXT("ok"), Entry.Succeeded);
        Obj->SetBoolField(TEXT("skipped"), Entry.Skipped);
        Obj->SetStringField(TEXT("json"), ToFull(Entry.JsonFilePath));
        Obj->SetStringField(TEXT("txt"), ToFull(Entry.TextFilePath));
        Obj->SetStringField(TEXT("error"), Entry.ErrorMessage);
        Entries.Add(MakeShared<FJsonValueObject>(Obj));
    }
    Root->SetArrayField(TEXT("entries"), Entries);

    auto JsonString = FString{};
    const auto Writer = TJsonWriterFactory<>::Create(&JsonString);
    FJsonSerializer::Serialize(Root, Writer);

    const auto OutPath = FPaths::ConvertRelativePathToFull(
        FPaths::Combine(FPaths::ProjectSavedDir(), InProjectSavedSubdir, TEXT("LastRun.json")));

    if (NOT FFileHelper::SaveStringToFile(JsonString, *OutPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
    { ck::asset_exporter::Error(TEXT("[Dispatch] Failed to write manifest [{}]"), OutPath); }

    return OutPath;
}

// --------------------------------------------------------------------------------------------------------------------
