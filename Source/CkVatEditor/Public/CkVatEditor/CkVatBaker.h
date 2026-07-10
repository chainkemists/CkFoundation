#pragma once

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_VatCollection_Data;

namespace ck::vat_editor
{
    // Bakes a VAT collection in-editor: samples every clip via ck::anim_bake, encodes the mode-appropriate
    // VAT textures (layout contract: Source/CkVat/Plan/Gate_01_Bake.md), builds the lookup-UV static mesh,
    // saves all generated assets as siblings of the collection (overwrite-in-place), and writes the
    // serialized clip table + bounds back onto the collection (ApplyBakeResults). Editor-only (reads
    // FSkeletalMeshModel source data, uses FTextureSource / FMeshDescription). Idempotent.
    CKVATEDITOR_API auto
    Bake_VatCollection(
        UCk_VatCollection_Data& InCollection)
        -> bool;

    // Same compute as Bake_VatCollection, but every output (textures, static mesh) is outered to the
    // TRANSIENT package and NOTHING is saved to disk — results die with the session and the bake
    // re-runs per session. For gyms/tests building collections programmatically (pair with
    // UCkVat_BakerSubsystem::CreateAndBake_TransientCollection); shipped content uses the asset bake.
    // Still editor-only: the compute reads FSkeletalMeshModel source data.
    CKVATEDITOR_API auto
    Bake_VatCollection_Transient(
        UCk_VatCollection_Data& InCollection)
        -> bool;
}
