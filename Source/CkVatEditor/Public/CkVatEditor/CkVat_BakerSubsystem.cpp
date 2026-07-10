#include "CkVat_BakerSubsystem.h"

#include "CkVatEditor/CkVatBaker.h"

#include "CkVat/Collection/CkVatCollection_Data.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"

// --------------------------------------------------------------------------------------------------------------------

bool
    UCkVat_BakerSubsystem::
    Bake_VatCollection(UCk_VatCollection_Data* InCollection)
{
    CK_ENSURE_IF_NOT(ck::IsValid(InCollection), TEXT("Bake_VatCollection called with an invalid collection"))
    { return false; }

    return ck::vat_editor::Bake_VatCollection(*InCollection);
}
