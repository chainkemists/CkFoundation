#include "CkIskmAnimCollection_Fragment_Data.h"

#include "CkCore/Validation/CkIsValid.h"

#include "Animation/Skeleton.h"

#if WITH_EDITOR
    #include "Misc/DataValidation.h"
#endif

auto
    UCk_IskmAnimCollection_Data::
    Find_SequenceIndex_ByAsset(const UAnimSequenceBase* InAsset) const
    -> int32
{
    return _Sequences.IndexOfByPredicate([&](const FCk_IskmAnimCollection_SequenceDef& E)
    {
        return E.Get_Sequence() == InAsset;
    });
}

#if WITH_EDITOR
auto
    UCk_IskmAnimCollection_Data::
    PostEditChangeProperty(FPropertyChangedEvent& InPropertyChangedEvent)
    -> void
{
    Super::PostEditChangeProperty(InPropertyChangedEvent);
}

auto
    UCk_IskmAnimCollection_Data::
    IsDataValid(FDataValidationContext& InContext) const
    -> EDataValidationResult
{
    auto Result = Super::IsDataValid(InContext);

    if (ck::Is_NOT_Valid(_Skeleton))
    {
        InContext.AddError(FText::FromString(TEXT("AnimCollection has no Skeleton.")));
        Result = EDataValidationResult::Invalid;
    }

    if (ck::IsValid(_DefaultMesh) && _DefaultMesh->GetSkeleton() != _Skeleton)
    {
        InContext.AddError(FText::FromString(TEXT("DefaultMesh skeleton does not match the AnimCollection's Skeleton.")));
        Result = EDataValidationResult::Invalid;
    }

    for (auto Index = 0; Index < _Sequences.Num(); ++Index)
    {
        const auto& Def = _Sequences[Index];
        const auto* Seq = Def.Get_Sequence().Get();
        if (ck::Is_NOT_Valid(Seq))
        {
            InContext.AddError(FText::FromString(FString::Printf(TEXT("Sequence [%d] (%s) is null."), Index, *Def.Get_Name().ToString())));
            Result = EDataValidationResult::Invalid;
            continue;
        }
        if (Seq->GetSkeleton() != _Skeleton)
        {
            InContext.AddError(FText::FromString(FString::Printf(TEXT("Sequence [%d] (%s) skeleton mismatch."), Index, *Def.Get_Name().ToString())));
            Result = EDataValidationResult::Invalid;
        }
    }

    return Result;
}
#endif
