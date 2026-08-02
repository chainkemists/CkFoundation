#pragma once

#include "CkWebUmg/Asset/CkWebUmg_PageAsset.h"

// ====================================================================================================================
// Gate 4 emission entry point: *.ckui.json (+ its ckui-assets/ bundle) -> UCk_WebUmg_PageAsset_UE
// in a content package. Regeneration contract (DECISION 3): the asset stamps its source hash;
// re-importing an unchanged source is a NO-OP (returns the existing asset untouched); a changed
// source overwrites the node tree + textures wholesale. Textures are outered to the asset so one
// package carries the whole page.
// ====================================================================================================================

namespace ck::webumg::editor
{
    // InPackageFolder e.g. "/Game/WebUmg"; the asset name derives from the json basename.
    // InSaveToDisk=false imports in-memory only (tests); true also saves the package.
    CKWEBUMGEDITOR_API auto
    ImportPageAsset(
        const FString& InJsonPath,
        const FString& InPackageFolder,
        bool InSaveToDisk)
        -> UCk_WebUmg_PageAsset_UE*;

    // The full toolchain in one call: runs the Node extractor (headless Chrome) on the .html,
    // then imports the produced bundle. Requires node + Chrome on the machine (the extractor's
    // own contract); fails loudly on a non-zero extractor exit. Extraction output lands under
    // Saved/CkWebUmg/<page>/ so reruns overwrite deterministically.
    CKWEBUMGEDITOR_API auto
    ImportPageAssetFromHtml(
        const FString& InHtmlPath,
        const FString& InPackageFolder,
        bool InSaveToDisk)
        -> UCk_WebUmg_PageAsset_UE*;

    // Re-runs the import from the asset's stamped source (html re-extracts; json re-reads).
    // Unchanged sources no-op via the hash stamp. False (ensured) when no source is stamped.
    CKWEBUMGEDITOR_API auto
    ReimportPageAsset(
        UCk_WebUmg_PageAsset_UE& InOutAsset)
        -> bool;
}
