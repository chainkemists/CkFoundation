#pragma once

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_VatCollection_Data;

namespace ck::vat_editor
{
    // Bakes a VAT collection in-editor and saves every generated asset as a sibling of the collection
    // (overwrite-in-place), writing the clip table + bounds back via ApplyBakeResults. Idempotent.
    // Editor-only: reads FSkeletalMeshModel source data and uses FTextureSource / FMeshDescription.
    CKVATEDITOR_API auto
    Bake_VatCollection(
        UCk_VatCollection_Data& InCollection)
        -> bool;

    // Same compute as Bake_VatCollection, but every output is outered to the TRANSIENT package and NOTHING
    // is saved to disk — results die with the session. For gyms/tests building collections programmatically;
    // shipped content uses the asset bake. Still editor-only.
    CKVATEDITOR_API auto
    Bake_VatCollection_Transient(
        UCk_VatCollection_Data& InCollection)
        -> bool;
}
