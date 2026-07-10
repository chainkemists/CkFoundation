#include "CkVatCollection_Data.h"

#include "CkCore/Validation/CkIsValid.h"

#include "Animation/AnimSequenceBase.h"
#include "Animation/Skeleton.h"
#include "Engine/SkeletalMesh.h"

#if WITH_EDITOR
    #include "Misc/DataValidation.h"
#endif

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_VatCollection_Data::
    Find_BakedClipIndex_ByName(FName InClipName) const
    -> int32
{
    return _BakedClips.IndexOfByPredicate([&](const FCk_Vat_BakedClip& InClip)
    {
        return InClip.Get_Name() == InClipName;
    });
}

// --------------------------------------------------------------------------------------------------------------------

#if WITH_EDITOR
auto
    UCk_VatCollection_Data::
    IsDataValid(FDataValidationContext& InContext) const
    -> EDataValidationResult
{
    auto Result = Super::IsDataValid(InContext);

    if (ck::Is_NOT_Valid(_Skeleton))
    {
        InContext.AddError(FText::FromString(TEXT("VatCollection has no Skeleton.")));
        Result = EDataValidationResult::Invalid;
    }

    if (ck::Is_NOT_Valid(_SourceMesh))
    {
        InContext.AddError(FText::FromString(TEXT("VatCollection has no SourceMesh.")));
        Result = EDataValidationResult::Invalid;
    }
    else if (ck::IsValid(_Skeleton) && _SourceMesh->GetSkeleton() != _Skeleton)
    {
        InContext.AddError(FText::FromString(TEXT("SourceMesh skeleton does not match the VatCollection's Skeleton.")));
        Result = EDataValidationResult::Invalid;
    }

    TSet<FName> SeenClipNames;
    for (auto Index = 0; Index < _Clips.Num(); ++Index)
    {
        const auto& Def = _Clips[Index];
        const auto* Seq = Def.Get_Sequence().Get();

        if (Def.Get_Name().IsNone())
        {
            InContext.AddError(FText::FromString(FString::Printf(TEXT("Clip [%d] has no Name."), Index)));
            Result = EDataValidationResult::Invalid;
        }
        else if (SeenClipNames.Contains(Def.Get_Name()))
        {
            InContext.AddError(FText::FromString(FString::Printf(TEXT("Clip [%d] (%s) duplicates another clip's Name — runtime clip lookup is by name."),
                Index, *Def.Get_Name().ToString())));
            Result = EDataValidationResult::Invalid;
        }
        SeenClipNames.Add(Def.Get_Name());

        if (ck::Is_NOT_Valid(Seq))
        {
            InContext.AddError(FText::FromString(FString::Printf(TEXT("Clip [%d] (%s) has a null Sequence."), Index, *Def.Get_Name().ToString())));
            Result = EDataValidationResult::Invalid;
            continue;
        }
        if (ck::IsValid(_Skeleton) && Seq->GetSkeleton() != _Skeleton)
        {
            InContext.AddError(FText::FromString(FString::Printf(TEXT("Clip [%d] (%s) skeleton mismatch."), Index, *Def.Get_Name().ToString())));
            Result = EDataValidationResult::Invalid;
        }
    }

    return Result;
}
#endif
