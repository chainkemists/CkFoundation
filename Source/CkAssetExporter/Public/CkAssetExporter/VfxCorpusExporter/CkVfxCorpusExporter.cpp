#include "CkVfxCorpusExporter.h"

#include "CkAssetExporter_Log.h"

#include "CkAssetExporter/CascadeExporter/CkCascadeExporter.h"
#include "CkAssetExporter/MaterialExporter/CkMaterialExporter.h"
#include "CkAssetExporter/MeshExporter/CkMeshExporter.h"
#include "CkAssetExporter/NiagaraExporter/CkNiagaraExporter.h"
#include "CkAssetExporter/TextureExporter/CkTextureExporter.h"

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/ARFilter.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture.h"
#include "HAL/FileManager.h"
#include "Materials/MaterialInterface.h"
#include "Misc/Paths.h"
#include "NiagaraSystem.h"
#include "Particles/ParticleSystem.h"
#include "UObject/UObjectGlobals.h"

#include <Dom/JsonObject.h>
#include <Dom/JsonValue.h>
#include <Serialization/JsonSerializer.h>
#include <Serialization/JsonWriter.h>
#include <Misc/FileHelper.h>
#include <Misc/PackageName.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::asset_exporter::corpus
{
    // Loading ~1000 systems + their material/texture dependency fans in one editor session — collect
    // periodically so the working set stays bounded.
    constexpr auto GcInterval = int32{100};

    static auto MakeRelative(const FString& InPath, const FString& InRoot) -> FString
    {
        auto Relative = InPath;
        FPaths::MakePathRelativeTo(Relative, *(InRoot + TEXT("/")));
        return Relative;
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_VfxCorpusExporter::
    Get_DefaultCorpusRoot()
    -> FString
{
    return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("CkVfxCorpus"));
}

auto
    FCk_VfxCorpusExporter::
    DoGet_PackName(
        const FString& InPackagePath)
    -> FString
{
    auto Segments = TArray<FString>{};
    InPackagePath.ParseIntoArray(Segments, TEXT("/"));
    if (Segments.Num() == 0)
    { return TEXT("Unknown"); }
    if (Segments[0] == TEXT("Game"))
    { return Segments.Num() > 1 ? Segments[1] : TEXT("GameRoot"); }
    return Segments[0];
}

auto
    FCk_VfxCorpusExporter::
    DoGet_OutputSubDir(
        const FString& InPackageName)
    -> FString
{
    auto Folder = FPackageName::GetLongPackagePath(InPackageName);
    Folder.RemoveFromStart(TEXT("/"));
    if (Folder == TEXT("Game"))
    { return TEXT("GameRoot"); }
    Folder.RemoveFromStart(TEXT("Game/"));
    return Folder;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_VfxCorpusExporter::
    ExportCorpus(
        const TArray<FString>& InContentRoots,
        const FString& InCorpusRoot)
    -> FCk_VfxCorpusSummary
{
    auto Summary = FCk_VfxCorpusSummary{};
    Summary.CorpusRoot = InCorpusRoot;

    // ---- Discover by class (pack naming conventions are unreliable) ----
    auto& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
    if (AssetRegistry.IsLoadingAssets())
    { AssetRegistry.WaitForCompletion(); }

    auto Filter = FARFilter{};
    Filter.ClassPaths.Add(UNiagaraSystem::StaticClass()->GetClassPathName());
    Filter.ClassPaths.Add(UParticleSystem::StaticClass()->GetClassPathName());
    Filter.bRecursiveClasses = true;
    Filter.bRecursivePaths = true;
    for (const auto& Root : InContentRoots)
    { Filter.PackagePaths.Add(FName{*Root}); }

    auto Assets = TArray<FAssetData>{};
    AssetRegistry.GetAssets(Filter, Assets);
    Assets.Sort([](const FAssetData& A, const FAssetData& B){ return A.PackageName.LexicalLess(B.PackageName); });

    UE_LOG(CkAssetExporter, Display, TEXT("[VfxCorpus] Discovered %d particle systems under [%s]"),
        Assets.Num(), *FString::Join(InContentRoots, TEXT(", ")));

    if (Assets.Num() == 0)
    {
        Summary.ErrorMessage = TEXT("Asset registry returned no particle systems under the given roots");
        return Summary;
    }

    // ---- Fresh corpus root (regenerable output — stale mixed corpora are worse than a re-run) ----
    if (IFileManager::Get().DirectoryExists(*InCorpusRoot))
    {
        constexpr auto RequireExists = false;
        constexpr auto DeleteTree = true;
        IFileManager::Get().DeleteDirectory(*InCorpusRoot, RequireExists, DeleteTree);
    }

    // ---- Systems ----
    auto SystemsArr = TArray<TSharedPtr<FJsonValue>>{};
    auto AllMaterialPaths = TSet<FString>{};
    auto AllMeshPaths = TSet<FString>{};
    auto LoadCounter = int32{0};

    for (const auto& AssetData : Assets)
    {
        const auto PackagePath = AssetData.PackageName.ToString();
        const auto Pack = DoGet_PackName(PackagePath);
        const auto SystemsDir = FPaths::Combine(InCorpusRoot, TEXT("systems"), DoGet_OutputSubDir(PackagePath));

        auto Row = MakeShared<FJsonObject>();
        Row->SetStringField(TEXT("name"), AssetData.AssetName.ToString());
        Row->SetStringField(TEXT("packagePath"), PackagePath);
        Row->SetStringField(TEXT("pack"), Pack);

        ++Summary.Systems;
        if (++LoadCounter % ck::asset_exporter::corpus::GcInterval == 0)
        { CollectGarbage(RF_NoFlags); }

        auto* Asset = AssetData.GetAsset();
        if (auto* NiagaraSystem = Cast<UNiagaraSystem>(Asset))
        {
            ++Summary.Niagara;
            Row->SetStringField(TEXT("type"), TEXT("niagara"));
            Row->SetNumberField(TEXT("emitters"), NiagaraSystem->GetEmitterHandles().Num());

            const auto Result = FCk_NiagaraExporter::ExportNiagaraSystem(NiagaraSystem, SystemsDir);
            Row->SetStringField(TEXT("status"), Result.Succeeded ? TEXT("ok") : TEXT("failed"));
            if (Result.Succeeded)
            {
                Row->SetStringField(TEXT("json"), ck::asset_exporter::corpus::MakeRelative(Result.JsonFilePath, InCorpusRoot));
                Row->SetStringField(TEXT("txt"), ck::asset_exporter::corpus::MakeRelative(Result.TextFilePath, InCorpusRoot));
                auto Mats = TArray<TSharedPtr<FJsonValue>>{};
                for (const auto& Material : Result.ReferencedMaterials)
                {
                    Mats.Add(MakeShared<FJsonValueString>(Material));
                    AllMaterialPaths.Add(Material);
                }
                Row->SetArrayField(TEXT("materials"), Mats);
                for (const auto& MeshPath : Result.ReferencedMeshes)
                { AllMeshPaths.Add(MeshPath); }
            }
            else
            {
                ++Summary.FailedSystems;
                Row->SetStringField(TEXT("error"), Result.ErrorMessage);
            }
        }
        else if (auto* CascadeSystem = Cast<UParticleSystem>(Asset))
        {
            ++Summary.Cascade;
            Row->SetStringField(TEXT("type"), TEXT("cascade"));
            Row->SetNumberField(TEXT("emitters"), CascadeSystem->Emitters.Num());

            const auto Result = FCk_CascadeExporter::ExportParticleSystem(CascadeSystem, SystemsDir);
            Row->SetStringField(TEXT("status"), Result.Succeeded ? TEXT("ok") : TEXT("failed"));
            if (Result.Succeeded)
            {
                Row->SetStringField(TEXT("json"), ck::asset_exporter::corpus::MakeRelative(Result.JsonFilePath, InCorpusRoot));
                Row->SetStringField(TEXT("txt"), ck::asset_exporter::corpus::MakeRelative(Result.TextFilePath, InCorpusRoot));
                auto Mats = TArray<TSharedPtr<FJsonValue>>{};
                for (const auto& Material : Result.ReferencedMaterials)
                {
                    Mats.Add(MakeShared<FJsonValueString>(Material));
                    AllMaterialPaths.Add(Material);
                }
                Row->SetArrayField(TEXT("materials"), Mats);
            }
            else
            {
                ++Summary.FailedSystems;
                Row->SetStringField(TEXT("error"), Result.ErrorMessage);
            }
        }
        else
        {
            ++Summary.FailedSystems;
            Row->SetStringField(TEXT("type"), TEXT("unknown"));
            Row->SetStringField(TEXT("status"), TEXT("failed"));
            Row->SetStringField(TEXT("error"), TEXT("asset failed to load or is not a particle system"));
        }
        SystemsArr.Add(MakeShared<FJsonValueObject>(Row));

        if (Summary.Systems % 50 == 0)
        {
            UE_LOG(CkAssetExporter, Display, TEXT("[VfxCorpus] Systems %d/%d (failed so far: %d)"),
                Summary.Systems, Assets.Num(), Summary.FailedSystems);
        }
    }

    // ---- Materials (deduplicated across systems) ----
    auto MaterialsArr = TArray<TSharedPtr<FJsonValue>>{};
    auto AllTexturePaths = TSet<FString>{};
    auto MaterialPathsSorted = AllMaterialPaths.Array();
    MaterialPathsSorted.Sort();

    UE_LOG(CkAssetExporter, Display, TEXT("[VfxCorpus] Exporting %d unique materials"), MaterialPathsSorted.Num());

    for (const auto& MaterialPath : MaterialPathsSorted)
    {
        const auto Pack = DoGet_PackName(MaterialPath);
        auto Row = MakeShared<FJsonObject>();
        Row->SetStringField(TEXT("path"), MaterialPath);
        Row->SetStringField(TEXT("pack"), Pack);

        ++Summary.Materials;
        if (++LoadCounter % ck::asset_exporter::corpus::GcInterval == 0)
        { CollectGarbage(RF_NoFlags); }

        auto* Material = LoadObject<UMaterialInterface>(nullptr, *MaterialPath);
        if (Material == nullptr)
        {
            ++Summary.FailedMaterials;
            Row->SetStringField(TEXT("status"), TEXT("failed"));
            Row->SetStringField(TEXT("error"), TEXT("load failed"));
            MaterialsArr.Add(MakeShared<FJsonValueObject>(Row));
            continue;
        }

        const auto Result = FCk_MaterialExporter::ExportMaterial(Material,
            FPaths::Combine(InCorpusRoot, TEXT("materials"), DoGet_OutputSubDir(FPackageName::ObjectPathToPackageName(MaterialPath))));
        Row->SetStringField(TEXT("status"), Result.Succeeded ? TEXT("ok") : TEXT("failed"));
        if (Result.Succeeded)
        {
            Row->SetStringField(TEXT("json"), ck::asset_exporter::corpus::MakeRelative(Result.JsonFilePath, InCorpusRoot));
            for (const auto& Texture : Result.ReferencedTextures)
            { AllTexturePaths.Add(Texture); }
        }
        else
        {
            ++Summary.FailedMaterials;
            Row->SetStringField(TEXT("error"), Result.ErrorMessage);
        }
        MaterialsArr.Add(MakeShared<FJsonValueObject>(Row));
    }

    // ---- Textures (deduplicated across materials) ----
    auto TexturesArr = TArray<TSharedPtr<FJsonValue>>{};
    auto TexturePathsSorted = AllTexturePaths.Array();
    TexturePathsSorted.Sort();

    UE_LOG(CkAssetExporter, Display, TEXT("[VfxCorpus] Exporting %d unique textures"), TexturePathsSorted.Num());

    for (const auto& TexturePath : TexturePathsSorted)
    {
        const auto Pack = DoGet_PackName(TexturePath);
        auto Row = MakeShared<FJsonObject>();
        Row->SetStringField(TEXT("path"), TexturePath);
        Row->SetStringField(TEXT("pack"), Pack);

        ++Summary.Textures;
        if (++LoadCounter % ck::asset_exporter::corpus::GcInterval == 0)
        { CollectGarbage(RF_NoFlags); }

        auto* Texture = LoadObject<UTexture>(nullptr, *TexturePath);
        if (Texture == nullptr)
        {
            ++Summary.FailedTextures;
            Row->SetStringField(TEXT("status"), TEXT("failed"));
            Row->SetStringField(TEXT("error"), TEXT("load failed"));
            TexturesArr.Add(MakeShared<FJsonValueObject>(Row));
            continue;
        }

        const auto Result = FCk_TextureExporter::ExportTexture(Texture,
            FPaths::Combine(InCorpusRoot, TEXT("textures"), DoGet_OutputSubDir(FPackageName::ObjectPathToPackageName(TexturePath))));
        Row->SetStringField(TEXT("status"), Result.Succeeded ? TEXT("ok") : TEXT("failed"));
        if (Result.Succeeded)
        {
            Row->SetStringField(TEXT("json"), ck::asset_exporter::corpus::MakeRelative(Result.JsonFilePath, InCorpusRoot));
            if (NOT Result.PngFilePath.IsEmpty())
            { Row->SetStringField(TEXT("png"), ck::asset_exporter::corpus::MakeRelative(Result.PngFilePath, InCorpusRoot)); }
        }
        else
        {
            ++Summary.FailedTextures;
            Row->SetStringField(TEXT("error"), Result.ErrorMessage);
        }
        TexturesArr.Add(MakeShared<FJsonValueObject>(Row));
    }

    // ---- Meshes (deduplicated across mesh renderers — the VFX carrier geometry) ----
    auto MeshesArr = TArray<TSharedPtr<FJsonValue>>{};
    auto MeshPathsSorted = AllMeshPaths.Array();
    MeshPathsSorted.Sort();

    UE_LOG(CkAssetExporter, Display, TEXT("[VfxCorpus] Exporting %d unique carrier meshes"), MeshPathsSorted.Num());

    for (const auto& MeshPath : MeshPathsSorted)
    {
        const auto Pack = DoGet_PackName(MeshPath);
        auto Row = MakeShared<FJsonObject>();
        Row->SetStringField(TEXT("path"), MeshPath);
        Row->SetStringField(TEXT("pack"), Pack);

        ++Summary.Meshes;
        if (++LoadCounter % ck::asset_exporter::corpus::GcInterval == 0)
        { CollectGarbage(RF_NoFlags); }

        auto* Mesh = LoadObject<UStaticMesh>(nullptr, *MeshPath);
        if (Mesh == nullptr)
        {
            ++Summary.FailedMeshes;
            Row->SetStringField(TEXT("status"), TEXT("failed"));
            Row->SetStringField(TEXT("error"), TEXT("load failed"));
            MeshesArr.Add(MakeShared<FJsonValueObject>(Row));
            continue;
        }

        const auto Result = FCk_MeshExporter::ExportStaticMesh(Mesh,
            FPaths::Combine(InCorpusRoot, TEXT("meshes"), DoGet_OutputSubDir(FPackageName::ObjectPathToPackageName(MeshPath))));
        Row->SetStringField(TEXT("status"), Result.Succeeded ? TEXT("ok") : TEXT("failed"));
        if (Result.Succeeded)
        {
            Row->SetStringField(TEXT("json"), ck::asset_exporter::corpus::MakeRelative(Result.JsonFilePath, InCorpusRoot));
            if (NOT Result.ObjFilePath.IsEmpty())
            { Row->SetStringField(TEXT("obj"), ck::asset_exporter::corpus::MakeRelative(Result.ObjFilePath, InCorpusRoot)); }
        }
        else
        {
            ++Summary.FailedMeshes;
            Row->SetStringField(TEXT("error"), Result.ErrorMessage);
        }
        MeshesArr.Add(MakeShared<FJsonValueObject>(Row));
    }

    // ---- Index ----
    auto Index = MakeShared<FJsonObject>();
    Index->SetStringField(TEXT("generated"), FDateTime::UtcNow().ToIso8601());
    Index->SetNumberField(TEXT("exporterVersion"), 3); // v3: meshes/ chain (carrier-geometry OBJ + stats)
    auto RootsArr = TArray<TSharedPtr<FJsonValue>>{};
    for (const auto& Root : InContentRoots)
    { RootsArr.Add(MakeShared<FJsonValueString>(Root)); }
    Index->SetArrayField(TEXT("roots"), RootsArr);

    auto SummaryObj = MakeShared<FJsonObject>();
    SummaryObj->SetNumberField(TEXT("systems"), Summary.Systems);
    SummaryObj->SetNumberField(TEXT("niagara"), Summary.Niagara);
    SummaryObj->SetNumberField(TEXT("cascade"), Summary.Cascade);
    SummaryObj->SetNumberField(TEXT("failedSystems"), Summary.FailedSystems);
    SummaryObj->SetNumberField(TEXT("materials"), Summary.Materials);
    SummaryObj->SetNumberField(TEXT("failedMaterials"), Summary.FailedMaterials);
    SummaryObj->SetNumberField(TEXT("textures"), Summary.Textures);
    SummaryObj->SetNumberField(TEXT("failedTextures"), Summary.FailedTextures);
    SummaryObj->SetNumberField(TEXT("meshes"), Summary.Meshes);
    SummaryObj->SetNumberField(TEXT("failedMeshes"), Summary.FailedMeshes);
    Index->SetObjectField(TEXT("summary"), SummaryObj);

    Index->SetArrayField(TEXT("systems"), SystemsArr);
    Index->SetArrayField(TEXT("materials"), MaterialsArr);
    Index->SetArrayField(TEXT("textures"), TexturesArr);
    Index->SetArrayField(TEXT("meshes"), MeshesArr);

    auto IndexString = FString{};
    const auto JsonWriter = TJsonWriterFactory<>::Create(&IndexString);
    FJsonSerializer::Serialize(Index, JsonWriter);

    Summary.IndexPath = FPaths::Combine(InCorpusRoot, TEXT("index.json"));
    if (NOT FFileHelper::SaveStringToFile(IndexString, *Summary.IndexPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
    {
        Summary.ErrorMessage = ck::Format_UE(TEXT("Failed to write index [{}]"), Summary.IndexPath);
        return Summary;
    }

    UE_LOG(CkAssetExporter, Display,
        TEXT("[VfxCorpus] DONE: %d systems (%d niagara, %d cascade, %d failed), %d materials (%d failed), %d textures (%d failed), %d meshes (%d failed) -> %s"),
        Summary.Systems, Summary.Niagara, Summary.Cascade, Summary.FailedSystems,
        Summary.Materials, Summary.FailedMaterials, Summary.Textures, Summary.FailedTextures,
        Summary.Meshes, Summary.FailedMeshes, *InCorpusRoot);

    Summary.Succeeded = true;
    return Summary;
}
