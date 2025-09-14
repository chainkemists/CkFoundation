#include "CkAssetRegistryConfig.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkAssetRegistryConfig::
    GetDisplayName() const
    -> FString
{
    auto AssetName = GetName();
    return ck::Format_UE(TEXT("{} ({}→{} [{}])"), AssetName, AssetDiscoveryRoot, OutputFileName, Namespace);
}

// --------------------------------------------------------------------------------------------------------------------
