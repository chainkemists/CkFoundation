#pragma once

#include "CkWebUmg/Asset/CkWebUmg_PageAsset.h"
#include "CkWebUmg/Ir/CkWebUmg_Ir.h"

// ====================================================================================================================
// Loader-IR <-> PageAsset projection (Gate 4). ConvertIrToAsset is the emission path (import-time;
// hard-fails on duplicate data-ck-name per DECISION 3); ConvertAssetToIr rebuilds the loader form
// so runtime consumption reuses the one battle-tested builder instead of forking it.
// ====================================================================================================================

struct CKWEBUMG_API FCkWebUmg_ValidationIssue
{
    FString NodeId;   // empty for page-level issues
    FString Property; // the declaration or bundle entry that triggered the issue
    FString Message;  // human-readable, includes how to fix it
};

struct CKWEBUMG_API FCkWebUmg_ValidationResult
{
    TArray<FCkWebUmg_ValidationIssue> Errors;   // emission would hard-fail — block the import
    TArray<FCkWebUmg_ValidationIssue> Warnings; // conversion proceeds; content will be dropped or approximated
};

namespace ck::webumg
{
    // Read-only pre-import validation — mutates nothing, never touches the project. Errors are
    // exactly the conditions ConvertIrToAsset hard-fails on; warnings enumerate what the
    // conversion report will carry plus bundle problems (missing texture files, checked only when
    // InBundleBaseDir is non-empty).
    CKWEBUMG_API auto
    ValidateIrForEmission(
        const FCkWebUmg_IrDocument& InDocument,
        const FString& InBundleBaseDir = {})
        -> FCkWebUmg_ValidationResult;

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
