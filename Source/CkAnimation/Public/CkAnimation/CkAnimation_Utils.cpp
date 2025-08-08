#include "CkAnimation_Utils.h"


#include "CkCore/Component/CkActorComponent_Utils.h"
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

auto
    UCk_Utils_Animation_UE::
    DoesMontageMatchMeshSkeleton(
        UAnimMontage* InMontage,
        USkeletalMeshComponent* InSkeletalMeshComponent)
    -> bool
{
    CK_ENSURE_IF_NOT(ck::IsValid(InMontage), TEXT("Invalid Animation Montage supplied to DoesMontageMatchMeshSkeleton"))
    { return false; }

    CK_ENSURE_IF_NOT(ck::IsValid(InSkeletalMeshComponent), TEXT("Invalid Skeletal Mesh Compontnt supplied to DoesMontageMatchMeshSkeleton"))
    { return false; }

    CK_ENSURE_IF_NOT(ck::IsValid(InSkeletalMeshComponent->GetSkinnedAsset()), TEXT("Mesh [{}] has NO valid Skinned Asset!"), InSkeletalMeshComponent)
    { return false; }

    return InMontage->GetSkeleton() == InSkeletalMeshComponent->GetSkinnedAsset()->GetSkeleton();
}


auto
	UCk_Utils_Animation_UE::
	Request_PlayMontage(
		USkeletalMeshComponent* InSkeletalMeshComponent,
		UAnimMontage* MontageToPlay,
		float PlayRate,
		float StartingPosition,
		FName StartingSection,
		bool ShouldStopAllMontages)
	-> UPlayMontageCallbackProxy*
{
	CK_ENSURE_IF_NOT(ck::IsValid(MontageToPlay),
		TEXT("Montage to play is NOT valid!"))
    { return {}; }

	CK_ENSURE_IF_NOT(ck::IsValid(InSkeletalMeshComponent),
		TEXT("Skeletal Mesh is NOT valid! Montage [{}]"), MontageToPlay)
    { return {}; }

	CK_ENSURE_IF_NOT(PlayRate > 0,
		TEXT("Invalid playrate [{}]! Needs to be positive"), PlayRate)
    { return {}; }

	const auto& AnimInstance = InSkeletalMeshComponent->GetAnimInstance();
	CK_ENSURE_IF_NOT(ck::IsValid(AnimInstance),
		TEXT("Anim Instance NOT valid! Skeletal Mesh [{}]"), InSkeletalMeshComponent)
    { return {}; }

	CK_ENSURE_IF_NOT(DoesMontageMatchMeshSkeleton(MontageToPlay, InSkeletalMeshComponent),
		TEXT("Montage [{}] does NOT match Skeletal Mesh [{}]!"), MontageToPlay, InSkeletalMeshComponent)
    { return {}; }

	const auto& NetMode = UCk_Utils_Net_UE::Get_NetMode(InSkeletalMeshComponent);
	const auto& AllowTickOnDedicatedServer = UCk_Utils_ActorComponent_UE::Get_AllowTickOnDedicatedServer(InSkeletalMeshComponent);

	CK_ENSURE_IF_NOT(NetMode != ECk_Net_NetModeType::Host || AllowTickOnDedicatedServer,
		TEXT("Trying to run montage on Mesh Component trying to tick on server while disallowed! Montage [{}] Skeletal Mesh [{}]!"), MontageToPlay, InSkeletalMeshComponent)
    { return {}; }

	CK_ENSURE_IF_NOT(InSkeletalMeshComponent->IsComponentTickEnabled(),
		TEXT("Trying to run montage on Mesh Component [{}] with tick NOT enabled!"), InSkeletalMeshComponent)
    { return {}; }

	return UPlayMontageCallbackProxy::CreateProxyObjectForPlayMontage(InSkeletalMeshComponent, MontageToPlay, PlayRate, StartingPosition, StartingSection, ShouldStopAllMontages);
}

// --------------------------------------------------------------------------------------------------------------------
