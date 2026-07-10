#include "CkVatCollection_Data.h"

#include "CkCore/Validation/CkIsValid.h"

#include "Animation/AnimSequenceBase.h"
#include "Animation/Skeleton.h"
#include "Engine/SkeletalMesh.h"

#if WITH_EDITOR
    #include "Misc/DataValidation.h"
    #include "Misc/SecureHash.h"
#endif

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_VatCollection_Data::
    Find_BakedClipIndex_ByName(FName InClipName) const
    -> int32
{
    return _BakedData.Get_BakedClips().IndexOfByPredicate([&](const FCk_Vat_BakedClip& InClip)
    {
        return InClip.Get_Name() == InClipName;
    });
}

// --------------------------------------------------------------------------------------------------------------------

#if WITH_EDITOR
auto
    UCk_VatCollection_Data::
    Compute_BakeInputsHash() const
    -> FString
{
    // Every input the baker reads participates; PlayLength participates because it drives the frame
    // layout (a re-imported/trimmed sequence at the same path must read as stale).
    auto Description = FString{};
    Description += FString::Printf(TEXT("Skeleton:%s|"), *GetPathNameSafe(_Skeleton.Get()));
    Description += FString::Printf(TEXT("SourceMesh:%s|"), *GetPathNameSafe(_SourceMesh.Get()));

    for (const auto& Clip : _Clips)
    {
        const auto* Seq = Clip.Get_Sequence().Get();
        Description += FString::Printf(TEXT("Clip:%s=%s@%.6f|"),
            *Clip.Get_Name().ToString(),
            *GetPathNameSafe(Seq),
            ck::IsValid(Seq) ? Seq->GetPlayLength() : 0.0f);
    }

    Description += FString::Printf(TEXT("Freq:%d|Mode:%d|Precision:%d|Influences:%d|Storage:%d|SourceLOD:%d|LookupUV:%d|MaxW:%d|MaxRows:%d|RootMotion:%d|NoRetarget:%d"),
        _BakeSettings.Get_SampleFrequency(),
        static_cast<int32>(_BakeSettings.Get_BakeMode()),
        static_cast<int32>(_BakeSettings.Get_Precision()),
        static_cast<int32>(_BakeSettings.Get_BoneInfluences()),
        static_cast<int32>(_BakeSettings.Get_BoneWeightStorage()),
        _BakeSettings.Get_SourceLOD(),
        _BakeSettings.Get_LookupUVChannel(),
        _BakeSettings.Get_MaxTextureWidth(),
        _BakeSettings.Get_MaxTextureRows(),
        _BakeSettings.Get_ExtractRootMotion() ? 1 : 0,
        _BakeSettings.Get_DisableRetargeting() ? 1 : 0);

    return FMD5::HashAnsiString(*Description);
}

auto
    UCk_VatCollection_Data::
    Get_IsBakeStale() const
    -> bool
{
    return _BakedData.Get_IsBaked() && _BakedData.Get_BakedInputsHash() != Compute_BakeInputsHash();
}

auto
    UCk_VatCollection_Data::
    ApplyBakeResults(const FCk_Vat_BakeResults& InResults)
    -> void
{
    _BakedData._BakedMesh = InResults.BakedMesh;
    _BakedData._PositionTexture = InResults.PositionTexture;
    _BakedData._NormalTexture = InResults.NormalTexture;
    _BakedData._BonePositionTexture = InResults.BonePositionTexture;
    _BakedData._BoneRotationTexture = InResults.BoneRotationTexture;
    _BakedData._BoneIndexTexture = InResults.BoneIndexTexture;
    _BakedData._BoneWeightTexture = InResults.BoneWeightTexture;
    _BakedData._BakedClips = InResults.BakedClips;
    _BakedData._TextureWidth = InResults.TextureWidth;
    _BakedData._TextureRows = InResults.TextureRows;
    _BakedData._AnimatedBounds = InResults.AnimatedBounds;
    _BakedData._PositionBoundsMin = InResults.PositionBoundsMin;
    _BakedData._PositionBoundsMax = InResults.PositionBoundsMax;
    _BakedData._IsBaked = true;
    _BakedData._BakedInputsHash = Compute_BakeInputsHash();

    MarkPackageDirty();
}

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

    // Unbaked is a legitimate mid-authoring state (warn); a STALE bake ships wrong pixels silently (error).
    if (NOT _BakedData.Get_IsBaked())
    {
        InContext.AddWarning(FText::FromString(TEXT("VatCollection has not been baked — bake it (Bake button in the details panel) before it can render.")));
    }
    else if (Get_IsBakeStale())
    {
        InContext.AddError(FText::FromString(TEXT("VatCollection bake is STALE — its inputs changed after the last bake. Rebake before shipping (the serialized textures/mesh no longer match the inputs).")));
        Result = EDataValidationResult::Invalid;
    }

    return Result;
}
#endif
