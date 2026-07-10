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

UCk_VatCollection_Data*
    UCkVat_BakerSubsystem::
    CreateAndBake_TransientCollection(
        USkeleton* InSkeleton,
        USkeletalMesh* InSourceMesh,
        const TArray<FCk_VatCollection_ClipDef>& InClips,
        int32 InSampleFrequency,
        ECk_Vat_BakeMode InBakeMode,
        ECk_Vat_Precision InPrecision)
{
    const auto CollectionName = MakeUniqueObjectName(
        GetTransientPackage(), UCk_VatCollection_Data::StaticClass(), FName{TEXT("VatCollection_Transient")});
    auto* Collection = NewObject<UCk_VatCollection_Data>(GetTransientPackage(), CollectionName, RF_Transient);
    CK_ENSURE_IF_NOT(ck::IsValid(Collection), TEXT("CreateAndBake_TransientCollection: could not create the collection object"))
    { return nullptr; }

    // Friend access: the transient factory is a sanctioned input writer (serialized assets are
    // authored in the details panel instead). Input validation stays in the baker's ensures.
    Collection->_Skeleton = InSkeleton;
    Collection->_SourceMesh = InSourceMesh;
    Collection->_Clips = InClips;
    Collection->_SampleFrequency = FMath::Clamp(InSampleFrequency, 1, 120);
    Collection->_BakeMode = InBakeMode;
    Collection->_Precision = InPrecision;

    if (NOT ck::vat_editor::Bake_VatCollection_Transient(*Collection))
    { return nullptr; }

    return Collection;
}
