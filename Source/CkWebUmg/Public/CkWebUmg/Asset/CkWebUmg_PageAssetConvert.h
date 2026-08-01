#pragma once

#include "CkWebUmg/Asset/CkWebUmg_PageAsset.h"
#include "CkWebUmg/Ir/CkWebUmg_Ir.h"

// ====================================================================================================================
// Loader-IR <-> PageAsset projection (Gate 4). ConvertIrToAsset is the emission path (import-time;
// hard-fails on duplicate data-ck-name per DECISION 3); ConvertAssetToIr rebuilds the loader form
// so runtime consumption reuses the one battle-tested builder instead of forking it.
// ====================================================================================================================

namespace ck::webumg
{
    // Returns false (and ensures) on a duplicate data-ck-name or malformed tree — the asset is
    // left untouched in that case (emission is atomic).
    CKWEBUMG_API auto
    ConvertIrToAsset(
        const FCkWebUmg_IrDocument& InDocument,
        const FString& InSourceHash,
        UCk_WebUmg_PageAsset_UE& InOutAsset)
        -> bool;

    CKWEBUMG_API auto
    ConvertAssetToIr(
        const UCk_WebUmg_PageAsset_UE& InAsset)
        -> FCkWebUmg_IrDocument;
}
