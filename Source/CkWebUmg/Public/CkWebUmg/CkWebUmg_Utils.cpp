#include "CkWebUmg_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"

// --------------------------------------------------------------------------------------------------------------------

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
