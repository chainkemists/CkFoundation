#include "CkWebUmgEditor/CkWebUmg_Importer.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkWebUmg/Asset/CkWebUmg_PageAssetConvert.h"
#include "CkWebUmg/Ir/CkWebUmg_IrLoader.h"
#include "CkWebUmg_Log.h"

#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::webumg::editor
{
    auto
    ImportPageAsset(
        const FString& InJsonPath,
        const FString& InPackageFolder,
        bool InSaveToDisk)
        -> UCk_WebUmg_PageAsset_UE*
    {
        auto JsonText = FString{};
        const auto FileWasRead = FFileHelper::LoadFileToString(JsonText, *InJsonPath);
        CK_ENSURE_IF_NOT(FileWasRead, TEXT("ImportPageAsset: cannot read [{}]"), InJsonPath)
        {}
        if (NOT FileWasRead)
        { return nullptr; }

        const auto SourceHash = FMD5::HashAnsiString(*JsonText);
        const auto AssetName = FPaths::GetBaseFilename(FPaths::GetBaseFilename(InJsonPath));
        const auto PackageName = InPackageFolder / AssetName;

        auto* Package = CreatePackage(*PackageName);
        if (auto* Existing = FindObject<UCk_WebUmg_PageAsset_UE>(Package, *AssetName);
            Existing != nullptr && Existing->Get_SourceHash() == SourceHash)
        {
            webumg::Display(TEXT("ImportPageAsset: [{}] unchanged (hash match) — no-op"), AssetName);
            return Existing;
        }

        const auto Document = LoadIrDocument(JsonText);
        if (NOT Document.IsSet())
        { return nullptr; }

        auto* Asset = FindObject<UCk_WebUmg_PageAsset_UE>(Package, *AssetName);
        if (Asset == nullptr)
        {
            Asset = NewObject<UCk_WebUmg_PageAsset_UE>(
                Package, *AssetName, RF_Public | RF_Standalone);
        }

        if (NOT ConvertIrToAsset(*Document, SourceHash, *Asset))
        { return nullptr; } // duplicate data-ck-name etc. — already ensured; existing asset untouched

        // Textures ride the same package (outered to the asset), sourced from the browser-
        // normalized ckui-assets bundle next to the json.
        auto Textures = TMap<FString, TObjectPtr<UTexture2D>>{};
        const auto BaseDir = FPaths::GetPath(InJsonPath);
        for (const auto& [AssetId, Src] : Document->AssetSourcesById)
        {
            const auto PngPath = FPaths::Combine(BaseDir, Src);
            auto* Imported = FImageUtils::ImportFileAsTexture2D(PngPath);
            if (Imported == nullptr)
            {
                webumg::Warning(TEXT("ImportPageAsset: texture [{}] failed from [{}]"), AssetId, PngPath);
                continue;
            }
            Imported->SRGB = true;
            Imported->UpdateResource();
            Imported->Rename(*AssetId, Asset);
            Textures.Add(AssetId, Imported);
        }
        Asset->Set_Textures(Textures);
        Asset->MarkPackageDirty();

        if (InSaveToDisk)
        {
            const auto FilePath = FPackageName::LongPackageNameToFilename(
                PackageName, FPackageName::GetAssetPackageExtension());
            auto SaveArgs = FSavePackageArgs{};
            SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
            const auto Saved = UPackage::SavePackage(Package, Asset, *FilePath, SaveArgs);
            CK_ENSURE_IF_NOT(Saved, TEXT("ImportPageAsset: SavePackage failed for [{}]"), PackageName)
            {}
        }
        webumg::Display(TEXT("ImportPageAsset: [{}] imported ({} nodes, {} textures)"),
            AssetName, Asset->Get_Nodes().Num(), Textures.Num());
        return Asset;
    }
}

// --------------------------------------------------------------------------------------------------------------------
