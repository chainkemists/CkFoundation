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
}
