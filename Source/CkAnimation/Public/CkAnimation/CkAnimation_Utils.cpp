#include "CkAnimation_Utils.h"

#include "Animation/AnimNotifies/AnimNotifyState.h"

#include "CkAnimation/AnimNotify/CkAnimNotify.h"
#include "CkAnimation/AnimNotify/CkAnimNotifyState.h"

#include "CkCore/Component/CkActorComponent_Utils.h"
#include "CkCore/Ensure/CkEnsure.h"

#include <Misc/EngineVersionComparison.h>

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

    auto SectionStartTime = 0.0f;
    auto SectionEndTime = 0.0f;

    InAnimMontage->GetSectionStartAndEndTime(InSectionIndex, SectionStartTime, SectionEndTime);

    return FCk_Animation_MontageSection_LengthInfo{
        FCk_Time{SectionStartTime},
        FCk_Time{SectionEndTime}};
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
    TryGet_MontageNotifyTimes(
        UAnimMontage* InAnimMontage,
        FName InNotifyName)
    -> TArray<FCk_Animation_MontageNotify_TimeInfo>
{
    CK_ENSURE_IF_NOT(ck::IsValid(InAnimMontage), TEXT("Invalid Animation Montage supplied to Get_MontageNotifyLength"))
    { return {}; }

    auto NotifyTimes = TArray<FCk_Animation_MontageNotify_TimeInfo>{};

    for (const FAnimNotifyEvent& NotifyEvent : InAnimMontage->Notifies)
    {
        const auto& NotifyEventName = [&NotifyEvent]()
        {
            if (ck::IsValid(NotifyEvent.Notify))
            {
                if (const auto& CkNotify = Cast<UCk_AnimNotify_UE>(NotifyEvent.Notify);
                    ck::IsValid(CkNotify))
                {
                    return CkNotify->Get_NameForPlayMontageNotify().ToString();
                }
                return NotifyEvent.Notify->GetNotifyName();
            }
            if (ck::IsValid(NotifyEvent.NotifyStateClass))
            {
                if (const auto& CkNotifyState = Cast<UCk_AnimNotifyState_UE>(NotifyEvent.NotifyStateClass);
                    ck::IsValid(CkNotifyState))
                {
                    return CkNotifyState->Get_NameForPlayMontageNotify().ToString();
                }
                return NotifyEvent.NotifyStateClass->GetNotifyName();
            }
            return FString{};
        }();

        if (NotifyEventName != InNotifyName)
        { continue; }

        NotifyTimes.Emplace(
            FCk_Time{NotifyEvent.GetTriggerTime()},
            FCk_Time{NotifyEvent.GetEndTriggerTime()});
    }

    return NotifyTimes;
}

auto
    UCk_Utils_Animation_UE::
    TryGet_MontageNotifyTime(
        UAnimMontage* InAnimMontage,
        FName InNotifyName,
        ECk_SucceededFailed& OutResult)
    -> FCk_Animation_MontageNotify_TimeInfo
{
    OutResult = ECk_SucceededFailed::Failed;

    const auto& NotifyTimes = TryGet_MontageNotifyTimes(InAnimMontage, InNotifyName);
    if (NotifyTimes.IsEmpty())
    {
        return {};
    }
    OutResult = ECk_SucceededFailed::Succeeded;
    return NotifyTimes[0];
}

auto
    UCk_Utils_Animation_UE::
    Get_CanPlayMontage(
        USkeletalMeshComponent* InSkeletalMeshComponent,
        UAnimMontage* InMontage,
        float InPlayRate,
        ECk_SucceededFailed& OutResult)
    -> ECk_PlayMontageFailureReason
{
    OutResult = ECk_SucceededFailed::Failed;

    if (ck::Is_NOT_Valid(InMontage))
    { return ECk_PlayMontageFailureReason::InvalidMontage; }

    if (ck::Is_NOT_Valid(InSkeletalMeshComponent))
    { return ECk_PlayMontageFailureReason::InvalidMeshComponent; }

    if (InPlayRate <= 0.0f)
    { return ECk_PlayMontageFailureReason::InvalidPlayRate; }

    if (ck::Is_NOT_Valid(InSkeletalMeshComponent->GetAnimInstance()))
    { return ECk_PlayMontageFailureReason::MissingAnimInstanceOnMeshComponent; }

    if (NOT DoesMontageMatchMeshSkeleton(InMontage, InSkeletalMeshComponent))
    { return ECk_PlayMontageFailureReason::SkeletonMismatch; }

    const auto NetMode = UCk_Utils_Net_UE::Get_NetMode(InSkeletalMeshComponent);
    const auto AllowTickOnDedicatedServer = UCk_Utils_ActorComponent_UE::Get_AllowTickOnDedicatedServer(InSkeletalMeshComponent);

    if (NetMode == ECk_Net_NetModeType::Host && NOT AllowTickOnDedicatedServer)
    { return ECk_PlayMontageFailureReason::MeshComponentCannotTickOnServer; }

    if (NOT InSkeletalMeshComponent->IsComponentTickEnabled())
    { return ECk_PlayMontageFailureReason::MeshTickDisabled; }

    OutResult = ECk_SucceededFailed::Succeeded;
    return ECk_PlayMontageFailureReason::InvalidMontage; // sentinel; caller must check OutResult
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
    -> FCk_PlayMontage_Result
{
    auto Validation = ECk_SucceededFailed::Failed;
    const auto FailureReason = Get_CanPlayMontage(InSkeletalMeshComponent, MontageToPlay, PlayRate, Validation);

    if (Validation == ECk_SucceededFailed::Failed)
    { return FCk_PlayMontage_Result{false, FailureReason, nullptr}; }

    auto* Proxy = UPlayMontageCallbackProxy::CreateProxyObjectForPlayMontage(
        InSkeletalMeshComponent, MontageToPlay, PlayRate, StartingPosition, StartingSection, ShouldStopAllMontages);

    return FCk_PlayMontage_Result{true, FailureReason, Proxy};
}

auto
    UCk_Utils_Animation_UE::
    Get_RootMotion(
        UAnimSequence* InAnimSequence)
    -> FTransform
{
    CK_ENSURE_IF_NOT(InAnimSequence,
        TEXT("Anim Sequence provided is NOT valid!"))
    { return {}; }

    const auto& DeltaTime = [InAnimSequence]()
    {
        auto Dt = FDeltaTimeRecord{};
        Dt.Delta = InAnimSequence->GetPlayLength();
        return Dt;
    }();
    // Putting in 0.f as a literal tries to use the float constructor which is deprecated
    static constexpr double ZeroAsADouble = 0.f;
#if UE_VERSION_OLDER_THAN(5, 6, 0)
    constexpr auto AllowLooping = false;
    return InAnimSequence->ExtractRootMotion(ZeroAsADouble, DeltaTime.Delta, AllowLooping);
#else
    constexpr auto AllowLooping = false;
    constexpr auto ExtractRootMotion = true;
    const auto& ExtractionParams = FAnimExtractContext{ZeroAsADouble, ExtractRootMotion, DeltaTime, AllowLooping};
    return InAnimSequence->ExtractRootMotion(ExtractionParams);
#endif
}

// --------------------------------------------------------------------------------------------------------------------
