#include "CkAnimation_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Animation_UE::
    Get_MontageSectionLength_ByIndex(
        UAnimMontage* InAnimMontage,
        int32 InSectionIndex)
    -> FCk_Animation_MontageSection_LengthInfo
{
    CK_ENSURE_IF_NOT(ck::IsValid(InAnimMontage), TEXT("Invalid Animation Montage supplied to Get_MontageSectionLength_ByIndex"))
    { return {}; }

    CK_ENSURE_IF_NOT(InAnimMontage->IsValidSectionIndex(InSectionIndex),
        TEXT("Cannot get the Length of Section with Index [{}] for Animation Montage [{}] because it an invalid section index"),
        InSectionIndex,
        InAnimMontage)
    { return {}; }

    const auto& SectionLength = InAnimMontage->GetSectionLength(InSectionIndex);
    auto SectionStartTime = 0.0f;
    auto SectionEndTime = 0.0f;

    InAnimMontage->GetSectionStartAndEndTime(InSectionIndex, SectionStartTime, SectionEndTime);

    return FCk_Animation_MontageSection_LengthInfo{}
            .Set_Length(FCk_Time{ SectionLength })
            .Set_StartTime(FCk_Time{ SectionStartTime })
            .Set_EndTime(FCk_Time{ SectionEndTime });
}

auto
    UCk_Utils_Animation_UE::
    Get_MontageSectionLength_ByName(
        UAnimMontage* InAnimMontage,
        FName InSectionName)
    -> FCk_Animation_MontageSection_LengthInfo
{
    CK_ENSURE_IF_NOT(ck::IsValid(InAnimMontage), TEXT("Invalid Animation Montage supplied to Get_MontageSectionLength_ByName"))
    { return {}; }


    CK_ENSURE_IF_NOT(InAnimMontage->IsValidSectionName(InSectionName),
        TEXT("Cannot get the Length of Section with Name [{}] for Animation Montage [{}] because it an invalid section name"),
        InSectionName,
        InAnimMontage)
    { return {}; }

    const auto& SectionIndex = InAnimMontage->GetSectionIndex(InSectionName);
    return Get_MontageSectionLength_ByIndex(InAnimMontage, SectionIndex);
}

auto
    UCk_Utils_Animation_UE::
    Get_MontageSectionIndex_FromPosition(
        UAnimMontage* InAnimMontage,
        float InPosition)
    -> int32
{
    CK_ENSURE_IF_NOT(ck::IsValid(InAnimMontage), TEXT("Invalid Animation Montage supplied to Get_MontageSectionIndex_FromPosition"))
    { return {}; }

    return InAnimMontage->GetSectionIndexFromPosition(InPosition);
}

auto
    UCk_Utils_Animation_UE::
    Get_IsValidMontageSectionIndex(
        UAnimMontage* InAnimMontage,
        int32 InSectionIndex)
    -> bool
{
    CK_ENSURE_IF_NOT(ck::IsValid(InAnimMontage), TEXT("Invalid Animation Montage supplied to Get_MontageSectionIndex_FromPosition"))
    { return {}; }

    return InAnimMontage->IsValidSectionIndex(InSectionIndex);
}

// --------------------------------------------------------------------------------------------------------------------
