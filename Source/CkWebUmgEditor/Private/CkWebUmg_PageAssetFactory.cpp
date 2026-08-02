#include "CkWebUmgEditor/CkWebUmg_PageAssetFactory.h"

#include "CkWebUmgEditor/CkWebUmg_Importer.h"

#include "Misc/Paths.h"

// --------------------------------------------------------------------------------------------------------------------

UCk_WebUmg_PageAssetFactory::UCk_WebUmg_PageAssetFactory()
{
    SupportedClass = UCk_WebUmg_PageAsset_UE::StaticClass();
    bCreateNew = false;
    bEditorImport = true;
    Formats.Add(TEXT("html;Web page design (CkWebUmg)"));
}

auto
    UCk_WebUmg_PageAssetFactory::
    FactoryCreateFile(
        UClass* InClass,
        UObject* InParent,
        FName InName,
        EObjectFlags InFlags,
        const FString& InFilename,
        const TCHAR* InParms,
        FFeedbackContext* InWarn,
        bool& OutOperationCanceled)
    -> UObject*
{
    OutOperationCanceled = false;
    // The importer creates/reuses the package named after the file basename inside the drop
    // folder — the same package the Content Browser handed us for a same-named drop.
    constexpr auto SaveToDisk = false; // the editor's import flow owns saving
    return ck::webumg::editor::ImportPageAssetFromHtml(
        InFilename, FPaths::GetPath(InParent->GetPackage()->GetName()), SaveToDisk);
}

auto
    UCk_WebUmg_PageAssetFactory::
    CanReimport(
        UObject* InObj,
        TArray<FString>& OutFilenames)
    -> bool
{
    const auto* Asset = Cast<UCk_WebUmg_PageAsset_UE>(InObj);
    if (Asset == nullptr)
    { return false; }

    const auto& Source = NOT Asset->Get_SourceHtmlPath().IsEmpty()
        ? Asset->Get_SourceHtmlPath()
        : Asset->Get_SourceJsonPath();
    if (Source.IsEmpty())
    { return false; }

    OutFilenames.Add(Source);
    return true;
}

auto
    UCk_WebUmg_PageAssetFactory::
    SetReimportPaths(
        UObject* InObj,
        const TArray<FString>& InNewReimportPaths)
    -> void
{
    auto* Asset = Cast<UCk_WebUmg_PageAsset_UE>(InObj);
    if (Asset == nullptr || InNewReimportPaths.Num() != 1)
    { return; }

    if (InNewReimportPaths[0].EndsWith(TEXT(".html")))
    { Asset->Set_SourceHtmlPath(InNewReimportPaths[0]); }
    else
    { Asset->Set_SourceJsonPath(InNewReimportPaths[0]).Set_SourceHtmlPath(FString{}); }
}

auto
    UCk_WebUmg_PageAssetFactory::
    Reimport(
        UObject* InObj)
    -> EReimportResult::Type
{
    auto* Asset = Cast<UCk_WebUmg_PageAsset_UE>(InObj);
    if (Asset == nullptr)
    { return EReimportResult::Failed; }

    return ck::webumg::editor::ReimportPageAsset(*Asset)
        ? EReimportResult::Succeeded
        : EReimportResult::Failed;
}
