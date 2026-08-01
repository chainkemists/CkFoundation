#include "CkWebUmg_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkWebUmg/Asset/CkWebUmg_PageAssetConvert.h"
#include "CkWebUmg/Ir/CkWebUmg_IrLoader.h"

#include "Interfaces/IPluginManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_WebUmg_UE::
    TryLoad_PageAssetFromJson(
        const FString& InJsonPath)
    -> UCk_WebUmg_PageAsset_UE*
{
    auto Path = InJsonPath;
    if (FPaths::IsRelative(Path))
    {
        const auto Plugin = IPluginManager::Get().FindPlugin(TEXT("CkFoundation"));
        if (Plugin != nullptr)
        { Path = FPaths::Combine(Plugin->GetBaseDir(), Path); }
    }

    auto JsonText = FString{};
    const auto FileWasRead = FFileHelper::LoadFileToString(JsonText, *Path);
    CK_ENSURE_IF_NOT(FileWasRead, TEXT("TryLoad_PageAssetFromJson: cannot read [{}]"), Path)
    {}
    if (NOT FileWasRead)
    { return nullptr; }

    const auto Document = ck::webumg::LoadIrDocument(JsonText);
    if (NOT Document.IsSet())
    { return nullptr; }

    auto* Asset = NewObject<UCk_WebUmg_PageAsset_UE>();
    if (NOT ck::webumg::ConvertIrToAsset(*Document, FMD5::HashAnsiString(*JsonText), *Asset))
    { return nullptr; }
    return Asset;
}

auto
    UCk_Utils_WebUmg_UE::
    Get_NodeCount(
        const UCk_WebUmg_PageAsset_UE* InAsset)
    -> int32
{
    const auto AssetIsValid = ck::IsValid(InAsset, ck::IsValid_Policy_NullptrOnly{});
    CK_ENSURE_IF_NOT(AssetIsValid, TEXT("Get_NodeCount called with an invalid PageAsset"))
    {}
    if (NOT AssetIsValid)
    { return 0; }

    return InAsset->Get_Nodes().Num();
}

auto
    UCk_Utils_WebUmg_UE::
    Get_ConversionReport(
        const UCk_WebUmg_PageAsset_UE* InAsset)
    -> TArray<FCk_WebUmg_ReportEntryData>
{
    const auto AssetIsValid = ck::IsValid(InAsset, ck::IsValid_Policy_NullptrOnly{});
    CK_ENSURE_IF_NOT(AssetIsValid, TEXT("Get_ConversionReport called with an invalid PageAsset"))
    {}
    if (NOT AssetIsValid)
    { return {}; }

    return InAsset->Get_ConversionReport();
}

auto
    UCk_Utils_WebUmg_UE::
    Get_NamedNode(
        const UCk_WebUmg_PageAsset_UE* InAsset,
        const FString& InCkName,
        bool& OutFound)
    -> FCk_WebUmg_NodeData
{
    OutFound = false;

    const auto AssetIsValid = ck::IsValid(InAsset, ck::IsValid_Policy_NullptrOnly{});
    CK_ENSURE_IF_NOT(AssetIsValid, TEXT("Get_NamedNode called with an invalid PageAsset"))
    {}
    if (NOT AssetIsValid)
    { return {}; }

    for (const auto& Node : InAsset->Get_Nodes())
    {
        if (Node.Get_CkName() == InCkName)
        {
            OutFound = true;
            return Node;
        }
    }
    return {};
}

// --------------------------------------------------------------------------------------------------------------------
