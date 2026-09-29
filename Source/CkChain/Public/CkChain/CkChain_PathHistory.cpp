#include "CkChain/CkChain_PathHistory.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "Math/RotationMatrix.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_chain_kernel
{
    auto
        DoGet_Orientation(
            const FVector& InTangent,
            const FVector& InUpNormalized,
            const FQuat& InFallbackRotation)
        -> FQuat
    {
        const auto HasOrientationBasis = NOT FVector::CrossProduct(InTangent, InUpNormalized).IsNearlyZero();
        CK_ENSURE_IF_NOT(HasOrientationBasis, TEXT("Chain orientation tangent is parallel to the configured up vector"))
        {
            return InFallbackRotation;
        }
        return FRotationMatrix::MakeFromXZ(InTangent, InUpNormalized).ToQuat();
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::chain::FPathHistory::Reseed(
        const FTransform& InHeadPose,
        float InSpacingCm)
    -> void
{
    const auto IsValidSpacing = FMath::IsFinite(InSpacingCm) && InSpacingCm > 0.0f;
    CK_ENSURE_IF_NOT(IsValidSpacing, TEXT("Chain history spacing must be finite and positive [{}]"), InSpacingCm)
    {
        return;
    }
    const auto IsValidPose = NOT InHeadPose.ContainsNaN();
    CK_ENSURE_IF_NOT(IsValidPose, TEXT("Chain history head pose must be finite"))
    {
        return;
    }
    Reserve_ForDistance(0.0f, InSpacingCm);
    _Start = 0;
    _Count = 0;
    DoAppend({InHeadPose.GetLocation() - InHeadPose.GetRotation().GetForwardVector() * InSpacingCm,
        InHeadPose.GetRotation(), -InSpacingCm});
    DoAppend({InHeadPose.GetLocation(), InHeadPose.GetRotation(), 0.0f});
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::chain::FPathHistory::Record(
        const FTransform& InHeadPose,
        float InSpacingCm)
    -> bool
{
    const auto IsValidSpacing = FMath::IsFinite(InSpacingCm) && InSpacingCm > 0.0f;
    CK_ENSURE_IF_NOT(IsValidSpacing, TEXT("Chain history spacing must be finite and positive [{}]"), InSpacingCm)
    {
        return false;
    }
    const auto HasHistory = _Count > 0;
    CK_ENSURE_IF_NOT(HasHistory, TEXT("Chain history must be seeded before recording"))
    {
        return false;
    }
    const auto IsValidPose = NOT InHeadPose.ContainsNaN();
    CK_ENSURE_IF_NOT(IsValidPose, TEXT("Chain history head pose must be finite"))
    {
        return false;
    }
    const auto& Newest = Get_Sample(_Count - 1);
    const auto Chord = FVector::Distance(InHeadPose.GetLocation(), Newest.Get_Location());
    if (Chord <= 0.0f || Chord < InSpacingCm)
    {
        return false;
    }
    const auto ArcDistance = static_cast<float>(Newest.Get_ArcDistanceCm() + Chord);
    const auto IsValidArcDistance = FMath::IsFinite(ArcDistance) && ArcDistance > Newest.Get_ArcDistanceCm();
    CK_ENSURE_IF_NOT(IsValidArcDistance, TEXT("Chain history arc distance must remain finite and increasing"))
    {
        return false;
    }
    DoAppend({InHeadPose.GetLocation(), InHeadPose.GetRotation(), ArcDistance});
    return true;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::chain::FPathHistory::Reserve_ForDistance(
        float InDistanceCm,
        float InSpacingCm)
    -> void
{
    const auto IsValidRange = FMath::IsFinite(InDistanceCm) && InDistanceCm >= 0.0f
        && FMath::IsFinite(InSpacingCm) && InSpacingCm > 0.0f;
    CK_ENSURE_IF_NOT(IsValidRange, TEXT("Chain history reserve distance and spacing are invalid [{}] [{}]"), InDistanceCm, InSpacingCm)
    {
        return;
    }
    const auto SampleRatio = static_cast<double>(InDistanceCm) / InSpacingCm;
    const auto FitsContainer = SampleRatio <= static_cast<double>(MAX_int32) - 2.0;
    CK_ENSURE_IF_NOT(FitsContainer, TEXT("Chain history reserve exceeds the sample container limit"))
    {
        return;
    }
    const auto Capacity = static_cast<int32>(FMath::CeilToInt64(SampleRatio)) + 2;
    if (Capacity <= _Buffer.Num())
    {
        return;
    }
    auto OrderedSamples = Get_Samples();
    _Buffer.SetNum(Capacity);
    _Start = 0;
    for (auto Index = 0; Index < _Count; ++Index)
    {
        _Buffer[Index] = OrderedSamples[Index];
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::chain::FPathHistory::Trim(
        float InKeepBehindHeadCm)
    -> void
{
    const auto IsValidDistance = FMath::IsFinite(InKeepBehindHeadCm) && InKeepBehindHeadCm >= 0.0f;
    CK_ENSURE_IF_NOT(IsValidDistance, TEXT("Chain history trim distance must be finite and nonnegative [{}]"), InKeepBehindHeadCm)
    {
        return;
    }
    if (_Count <= 2)
    {
        return;
    }
    const auto Cutoff = Get_HeadArcDistance() - InKeepBehindHeadCm;
    while (_Count > 2 && Get_Sample(1).Get_ArcDistanceCm() <= Cutoff)
    {
        _Start = (_Start + 1) % _Buffer.Num();
        --_Count;
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::chain::FPathHistory::Sample_AtArcDistance(
        float InS,
        ECk_Chain_HistorySeed InSeed) const
    -> TOptional<FCk_Chain_PathSample>
{
    const auto IsValidDistance = FMath::IsFinite(InS);
    CK_ENSURE_IF_NOT(IsValidDistance, TEXT("Chain history sample arc distance must be finite [{}]"), InS)
    {
        return {};
    }
    if (_Count == 0)
    {
        return {};
    }
    const auto& Oldest = Get_Sample(0);
    if (InS < Oldest.Get_ArcDistanceCm() && InSeed == ECk_Chain_HistorySeed::HoldUntilCovered)
    {
        return {};
    }
    if (_Count == 1 || InS >= Get_HeadArcDistance())
    {
        return Get_Sample(_Count - 1);
    }
    const auto Index = DoGet_BracketStart(InS);
    const auto& First = Get_Sample(Index);
    const auto& Second = Get_Sample(Index + 1);
    if (InS < Oldest.Get_ArcDistanceCm())
    {
        return FCk_Chain_PathSample{Oldest.Get_Location() + Tangent_AtArcDistance(InS) * (InS - Oldest.Get_ArcDistanceCm()),
            Oldest.Get_Rotation(), InS};
    }
    const auto Alpha = (InS - First.Get_ArcDistanceCm()) / (Second.Get_ArcDistanceCm() - First.Get_ArcDistanceCm());
    return FCk_Chain_PathSample{FMath::Lerp(First.Get_Location(), Second.Get_Location(), Alpha),
        FQuat::Slerp(First.Get_Rotation(), Second.Get_Rotation(), Alpha), InS};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::chain::FPathHistory::Tangent_AtArcDistance(
        float InS) const
    -> FVector
{
    const auto IsValidDistance = FMath::IsFinite(InS);
    CK_ENSURE_IF_NOT(IsValidDistance, TEXT("Chain history tangent arc distance must be finite [{}]"), InS)
    {
        return FVector::ZeroVector;
    }
    if (_Count < 2)
    {
        return FVector::ZeroVector;
    }
    const auto Index = DoGet_BracketStart(InS);
    const auto Segment = Get_Sample(Index + 1).Get_Location() - Get_Sample(Index).Get_Location();
    const auto SegmentLength = Segment.Size();
    const auto HasDirection = FMath::IsFinite(SegmentLength) && SegmentLength > 0.0;
    CK_ENSURE_IF_NOT(HasDirection, TEXT("Chain history segment must have a finite nonzero direction"))
    {
        return FVector::ZeroVector;
    }
    return Segment / SegmentLength;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::chain::FPathHistory::Slice_Behind(
        float InMaxS) const
    -> FPathHistory
{
    const auto IsValidDistance = FMath::IsFinite(InMaxS);
    CK_ENSURE_IF_NOT(IsValidDistance, TEXT("Chain history slice arc distance must be finite [{}]"), InMaxS)
    {
        return {};
    }
    auto Result = FPathHistory{};
    auto SliceCount = 0;
    while (SliceCount < _Count && Get_Sample(SliceCount).Get_ArcDistanceCm() <= InMaxS)
    {
        ++SliceCount;
    }
    if (SliceCount == 0)
    {
        return Result;
    }
    Result.Reserve_ForDistance(static_cast<float>(FMath::Max(0, SliceCount - 2)), 1.0f);
    const auto Origin = Get_Sample(SliceCount - 1).Get_ArcDistanceCm();
    for (auto Index = 0; Index < SliceCount; ++Index)
    {
        const auto& Sample = Get_Sample(Index);
        Result.DoAppend({Sample.Get_Location(), Sample.Get_Rotation(), Sample.Get_ArcDistanceCm() - Origin});
    }
    return Result;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::chain::FPathHistory::Get_HeadArcDistance() const
    -> float
{
    CK_ENSURE_IF_NOT(_Count > 0, TEXT("Chain history has no head sample"))
    {
        return 0.0f;
    }
    return Get_Sample(_Count - 1).Get_ArcDistanceCm();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::chain::FPathHistory::Get_OldestArcDistance() const
    -> float
{
    CK_ENSURE_IF_NOT(_Count > 0, TEXT("Chain history has no oldest sample"))
    {
        return 0.0f;
    }
    return Get_Sample(0).Get_ArcDistanceCm();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::chain::FPathHistory::Get_NumSamples() const
    -> int32
{
    return _Count;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::chain::FPathHistory::Get_Sample(
        int32 InIndex) const
    -> const FCk_Chain_PathSample&
{
    return _Buffer[(_Start + InIndex) % _Buffer.Num()];
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::chain::FPathHistory::Get_Samples() const
    -> TArray<FCk_Chain_PathSample>
{
    auto Result = TArray<FCk_Chain_PathSample>{};
    Result.Reserve(_Count);
    for (auto Index = 0; Index < _Count; ++Index)
    {
        Result.Add(Get_Sample(Index));
    }
    return Result;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::chain::FPathHistory::Get_AllocatedSize() const
    -> SIZE_T
{
    return _Buffer.GetAllocatedSize();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::chain::FPathHistory::DoGet_BracketStart(
        float InS) const
    -> int32
{
    auto First = 0;
    auto Last = _Count - 1;
    while (Last - First > 1)
    {
        const auto Middle = First + (Last - First) / 2;
        if (Get_Sample(Middle).Get_ArcDistanceCm() <= InS)
        {
            First = Middle;
        }
        else
        {
            Last = Middle;
        }
    }
    return First;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::chain::FPathHistory::DoAppend(
        const FCk_Chain_PathSample& InSample)
    -> void
{
    const auto Index = (_Start + _Count) % _Buffer.Num();
    _Buffer[Index] = InSample;
    if (_Count == _Buffer.Num())
    {
        _Start = (_Start + 1) % _Buffer.Num();
    }
    else
    {
        ++_Count;
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::chain::Get_LeadingArcDistance(
        const FPathHistory& InHistory,
        const FTransform& InHeadPose)
    -> float
{
    if (InHistory.Get_NumSamples() == 0)
    {
        return 0.0f;
    }
    const auto& Newest = InHistory.Get_Sample(InHistory.Get_NumSamples() - 1);
    const auto PendingChord = FVector::Distance(InHeadPose.GetLocation(), Newest.Get_Location());
    return static_cast<float>(Newest.Get_ArcDistanceCm() + PendingChord);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::chain::Solve_PathHistoryPose(
        const FPathHistory& InHistory,
        const FTransform& InHeadPose,
        float InDistanceFromHeadCm,
        ECk_Chain_LinkOrientation InOrientation,
        const FVector& InUpNormalized,
        ECk_Chain_HistorySeed InSeed,
        const FTransform& InLinkCurrentPose)
    -> TOptional<FTransform>
{
    const auto IsValidDistance = FMath::IsFinite(InDistanceFromHeadCm) && InDistanceFromHeadCm >= 0.0f;
    CK_ENSURE_IF_NOT(IsValidDistance, TEXT("Chain link distance must be finite and nonnegative [{}]"), InDistanceFromHeadCm)
    {
        return {};
    }
    const auto IsValidPose = NOT InLinkCurrentPose.ContainsNaN();
    CK_ENSURE_IF_NOT(IsValidPose, TEXT("Chain link current pose must be finite"))
    {
        return {};
    }
    const auto IsValidUp = NOT InUpNormalized.ContainsNaN() && InUpNormalized.IsNormalized();
    CK_ENSURE_IF_NOT(IsValidUp, TEXT("Chain solver up vector must be finite and normalized"))
    {
        return {};
    }
    const auto IsValidHeadPose = NOT InHeadPose.ContainsNaN();
    CK_ENSURE_IF_NOT(IsValidHeadPose, TEXT("Chain solver head pose must be finite"))
    {
        return {};
    }
    if (InHistory.Get_NumSamples() == 0)
    {
        return {};
    }
    const auto& Newest = InHistory.Get_Sample(InHistory.Get_NumSamples() - 1);
    const auto PendingChord = FVector::Distance(InHeadPose.GetLocation(), Newest.Get_Location());
    const auto ArcDistance = Get_LeadingArcDistance(InHistory, InHeadPose) - InDistanceFromHeadCm;
    auto Location = FVector::ZeroVector;
    auto SampledRotation = FQuat::Identity;
    auto Tangent = FVector::ZeroVector;
    if (ArcDistance >= Newest.Get_ArcDistanceCm())
    {
        const auto HasPendingChord = PendingChord > KINDA_SMALL_NUMBER;
        const auto Alpha = HasPendingChord ? (ArcDistance - Newest.Get_ArcDistanceCm()) / PendingChord : 1.0;
        Location = FMath::Lerp(Newest.Get_Location(), InHeadPose.GetLocation(), Alpha);
        SampledRotation = FQuat::Slerp(Newest.Get_Rotation(), InHeadPose.GetRotation(), Alpha);
        Tangent = HasPendingChord
            ? (InHeadPose.GetLocation() - Newest.Get_Location()) / PendingChord
            : InHistory.Tangent_AtArcDistance(Newest.Get_ArcDistanceCm());
    }
    else
    {
        const auto Sample = InHistory.Sample_AtArcDistance(ArcDistance, InSeed);
        if (NOT Sample.IsSet())
        {
            return {};
        }
        Location = Sample.GetValue().Get_Location();
        SampledRotation = Sample.GetValue().Get_Rotation();
        Tangent = InHistory.Tangent_AtArcDistance(ArcDistance);
    }
    auto Rotation = SampledRotation;
    switch (InOrientation)
    {
        case ECk_Chain_LinkOrientation::FollowPath:
        {
            // No direction of travel exists yet (fewer than two samples and no pending chord); that is not a
            // configuration error, so it keeps the sampled rotation without the parallel-up diagnostic.
            if (NOT Tangent.IsNearlyZero())
            {
                Rotation = ck_chain_kernel::DoGet_Orientation(Tangent, InUpNormalized, SampledRotation);
            }
            break;
        }
        case ECk_Chain_LinkOrientation::KeepOwn:
        {
            Rotation = InLinkCurrentPose.GetRotation();
            break;
        }
        case ECk_Chain_LinkOrientation::CopyHead:
        {
            break;
        }
        default:
        {
            CK_INVALID_ENUM(InOrientation);
            return {};
        }
    }
    return FTransform{Rotation, Location, InLinkCurrentPose.GetScale3D()};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::chain::Solve_DistanceConstraint(
        const FTransform& InHeadPose,
        TArrayView<const float> InSegmentLengthsCm,
        TArrayView<const ECk_Chain_LinkOrientation> InOrientations,
        const FVector& InUpNormalized,
        TArrayView<FTransform> InOutLinkPoses)
    -> void
{
    const auto HasMatchingInputs = InSegmentLengthsCm.Num() == InOutLinkPoses.Num()
        && InOrientations.Num() == InOutLinkPoses.Num();
    CK_ENSURE_IF_NOT(HasMatchingInputs, TEXT("Chain distance solver input arrays must have matching lengths"))
    {
        return;
    }
    const auto IsValidHead = NOT InHeadPose.ContainsNaN();
    CK_ENSURE_IF_NOT(IsValidHead, TEXT("Chain distance solver head pose must be finite"))
    {
        return;
    }
    const auto IsValidUp = NOT InUpNormalized.ContainsNaN() && InUpNormalized.IsNormalized();
    CK_ENSURE_IF_NOT(IsValidUp, TEXT("Chain solver up vector must be finite and normalized"))
    {
        return;
    }
    for (auto Index = 0; Index < InOutLinkPoses.Num(); ++Index)
    {
        const auto IsValidLink = FMath::IsFinite(InSegmentLengthsCm[Index]) && InSegmentLengthsCm[Index] >= 0.0f
            && NOT InOutLinkPoses[Index].ContainsNaN();
        CK_ENSURE_IF_NOT(IsValidLink, TEXT("Chain distance solver link input is invalid [{}]"), Index)
        {
            return;
        }
        const auto Orientation = InOrientations[Index];
        const auto IsValidOrientation = Orientation == ECk_Chain_LinkOrientation::FollowPath
            || Orientation == ECk_Chain_LinkOrientation::CopyHead || Orientation == ECk_Chain_LinkOrientation::KeepOwn;
        CK_ENSURE_IF_NOT(IsValidOrientation, TEXT("Chain distance solver orientation is invalid [{}]"), Orientation)
        {
            return;
        }
    }
    auto Predecessor = InHeadPose.GetLocation();
    for (auto Index = 0; Index < InOutLinkPoses.Num(); ++Index)
    {
        auto& Pose = InOutLinkPoses[Index];
        auto Direction = Pose.GetLocation() - Predecessor;
        if (Direction.SizeSquared() < FMath::Square(KINDA_SMALL_NUMBER))
        {
            Direction = -InHeadPose.GetRotation().GetForwardVector();
        }
        Direction.Normalize(0.0);
        Pose.SetLocation(Predecessor + Direction * InSegmentLengthsCm[Index]);
        if (InSegmentLengthsCm[Index] > 0.0f && InOrientations[Index] != ECk_Chain_LinkOrientation::KeepOwn)
        {
            Pose.SetRotation(ck_chain_kernel::DoGet_Orientation(-Direction, InUpNormalized, Pose.GetRotation()));
        }
        Predecessor = Pose.GetLocation();
    }
}

// --------------------------------------------------------------------------------------------------------------------
