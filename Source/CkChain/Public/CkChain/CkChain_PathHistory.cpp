#include "CkChain/CkChain_PathHistory.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "Math/RotationMatrix.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_chain_kernel
{
    constexpr auto MaxArcDistanceCm = static_cast<double>(TNumericLimits<float>::Max());
    constexpr auto SeedSpacingRelativeTolerance = 1.0e-3;
    constexpr auto SegmentLengthToleranceCm = 1.0e-3;

    auto
        DoGet_Orientation(
            const FVector& InTangent,
            const FVector& InUpNormalized,
            const FQuat& InFallbackRotation)
        -> FQuat
    {
        const auto HasOrientationBasis = NOT FVector::CrossProduct(InTangent, InUpNormalized).IsNearlyZero();
        CK_ENSURE_IF_NOT(HasOrientationBasis, TEXT("Chain orientation tangent is parallel to the configured up vector"))
        { return InFallbackRotation; }

        return FRotationMatrix::MakeFromXZ(InTangent, InUpNormalized).ToQuat();
    }

    auto
        DoGet_IsValidPose(
            const FTransform& InPose)
        -> bool
    {
        return NOT InPose.ContainsNaN() && InPose.IsRotationNormalized();
    }

    auto
        DoGet_IsValidOrientation(
            ECk_Chain_LinkOrientation InOrientation)
        -> bool
    {
        return InOrientation == ECk_Chain_LinkOrientation::FollowPath || InOrientation == ECk_Chain_LinkOrientation::CopyHead
            || InOrientation == ECk_Chain_LinkOrientation::KeepOwn;
    }

    auto
        DoGet_IsValidSeed(
            ECk_Chain_HistorySeed InSeed)
        -> bool
    {
        return InSeed == ECk_Chain_HistorySeed::StraightBehindHead || InSeed == ECk_Chain_HistorySeed::HoldUntilCovered;
    }

    auto
        DoGet_IsRepresentableArc(
            double InArcDistanceCm)
        -> bool
    {
        return FMath::IsFinite(InArcDistanceCm) && FMath::Abs(InArcDistanceCm) <= MaxArcDistanceCm;
    }

    // Dividing by the largest component first keeps the squared magnitude inside the double domain for every finite
    // vector, so neither a 1e-5 cm segment nor a 1e200 cm one loses its length or direction.
    auto
        DoGet_StableLength(
            const FVector& InVector)
        -> double
    {
        const auto Largest = InVector.GetAbsMax();
        if (Largest == 0.0 || NOT FMath::IsFinite(Largest))
        { return Largest; }

        return Largest * (InVector / Largest).Size();
    }

    auto
        DoGet_StableDirection(
            const FVector& InVector)
        -> FVector
    {
        const auto Largest = InVector.GetAbsMax();
        if (Largest == 0.0 || NOT FMath::IsFinite(Largest))
        { return FVector::ZeroVector; }

        const auto Scaled = InVector / Largest;
        return Scaled / Scaled.Size();
    }

    struct FLeadingChord
    {
        double ChordCm = 0.0;
        float ArcDistanceCm = 0.0f;
    };

    // Requires a nonempty history and a valid head pose.
    auto
        DoGet_LeadingChord(
            const ck::chain::FPathHistory& InHistory,
            const FTransform& InHeadPose)
        -> TOptional<FLeadingChord>
    {
        const auto& Newest = InHistory.Get_Sample(InHistory.Get_NumSamples() - 1);
        const auto Chord = DoGet_StableLength(InHeadPose.GetLocation() - Newest.Get_Location());
        const auto ArcDistance = static_cast<double>(Newest.Get_ArcDistanceCm()) + Chord;
        const auto IsRepresentable = DoGet_IsRepresentableArc(ArcDistance);
        CK_ENSURE_IF_NOT(IsRepresentable, TEXT("Chain history leading arc distance exceeded the finite float domain"))
        { return {}; }

        return FLeadingChord{Chord, static_cast<float>(ArcDistance)};
    }

    struct FSolvedSegment
    {
        FVector Location = FVector::ZeroVector;
        FVector Direction = FVector::ZeroVector;
    };
}

// --------------------------------------------------------------------------------------------------------------------

ck::chain::FPathHistory::
    FPathHistory(
        FPathHistory&& InOther) noexcept
    : _Buffer(MoveTemp(InOther._Buffer))
    , _Start(InOther._Start)
    , _Count(InOther._Count)
{
    InOther._Start = 0;
    InOther._Count = 0;
}

auto
    ck::chain::FPathHistory::
    operator=(
        FPathHistory&& InOther) noexcept
    -> FPathHistory&
{
    if (this == &InOther)
    { return *this; }

    _Buffer = MoveTemp(InOther._Buffer);
    _Start = InOther._Start;
    _Count = InOther._Count;
    InOther._Start = 0;
    InOther._Count = 0;
    return *this;
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
    { return; }

    const auto IsValidPose = ck_chain_kernel::DoGet_IsValidPose(InHeadPose);
    CK_ENSURE_IF_NOT(IsValidPose, TEXT("Chain history head pose must be finite and normalized"))
    { return; }

    // IsRotationNormalized admits |q|^2 within 1% of one; rotating by such a quaternion both scales and tilts the vector.
    const auto SeedDirection = InHeadPose.GetRotation().GetNormalized().GetForwardVector();
    const auto SeedLocation = InHeadPose.GetLocation() - SeedDirection * InSpacingCm;
    const auto SeedDistance = ck_chain_kernel::DoGet_StableLength(InHeadPose.GetLocation() - SeedLocation);
    const auto IsRepresentableSeed = NOT SeedLocation.ContainsNaN()
        && FMath::Abs(SeedDistance - InSpacingCm) <= ck_chain_kernel::SeedSpacingRelativeTolerance * InSpacingCm;
    CK_ENSURE_IF_NOT(IsRepresentableSeed, TEXT("Chain history seed spacing is not representable at this location [{}]"), InSpacingCm)
    { return; }

    Reserve_ForDistance(0.0f, InSpacingCm);
    _Start = 0;
    _Count = 0;
    DoAppend({SeedLocation, InHeadPose.GetRotation(), -InSpacingCm});
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
    { return false; }

    const auto HasHistory = _Count > 0;
    CK_ENSURE_IF_NOT(HasHistory, TEXT("Chain history must be seeded before recording"))
    { return false; }

    const auto IsValidPose = ck_chain_kernel::DoGet_IsValidPose(InHeadPose);
    CK_ENSURE_IF_NOT(IsValidPose, TEXT("Chain history head pose must be finite and normalized"))
    { return false; }

    const auto& Newest = Get_Sample(_Count - 1);
    const auto Chord = ck_chain_kernel::DoGet_StableLength(InHeadPose.GetLocation() - Newest.Get_Location());
    if (Chord <= 0.0 || Chord < InSpacingCm)
    { return false; }

    const auto NewArcDistance = static_cast<double>(Newest.Get_ArcDistanceCm()) + Chord;
    const auto IsRepresentableArc = ck_chain_kernel::DoGet_IsRepresentableArc(NewArcDistance);
    const auto ArcDistance = IsRepresentableArc ? static_cast<float>(NewArcDistance) : 0.0f;
    const auto IsValidArcDistance = IsRepresentableArc && ArcDistance > Newest.Get_ArcDistanceCm();
    CK_ENSURE_IF_NOT(IsValidArcDistance, TEXT("Chain history arc distance must remain finite and increasing"))
    { return false; }

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
    { return; }

    const auto SampleRatio = static_cast<double>(InDistanceCm) / InSpacingCm;
    const auto FitsContainer = SampleRatio <= static_cast<double>(MAX_int32) - 2.0;
    CK_ENSURE_IF_NOT(FitsContainer, TEXT("Chain history reserve exceeds the sample container limit"))
    { return; }

    const auto Capacity = static_cast<int32>(FMath::CeilToInt64(SampleRatio)) + 2;
    if (Capacity <= _Buffer.Num())
    { return; }

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
    { return; }

    if (_Count <= 2)
    { return; }

    const auto Cutoff = Get_HeadArcDistance() - InKeepBehindHeadCm;
    while (_Count > 2 && Get_Sample(1).Get_ArcDistanceCm() <= Cutoff)
    {
        _Start = static_cast<int32>((static_cast<int64>(_Start) + 1) % _Buffer.Num());
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
    { return {}; }

    const auto IsValidSeed = ck_chain_kernel::DoGet_IsValidSeed(InSeed);
    CK_ENSURE_IF_NOT(IsValidSeed, TEXT("Chain history sample seed policy is invalid [{}]"), InSeed)
    { return {}; }

    if (_Count == 0)
    { return {}; }

    const auto& Oldest = Get_Sample(0);
    if (InS < Oldest.Get_ArcDistanceCm() && InSeed == ECk_Chain_HistorySeed::HoldUntilCovered)
    { return {}; }

    if (_Count == 1 || InS >= Get_HeadArcDistance())
    { return Get_Sample(_Count - 1); }

    if (InS < Oldest.Get_ArcDistanceCm())
    {
        const auto ExtrapolationCm = static_cast<double>(InS) - static_cast<double>(Oldest.Get_ArcDistanceCm());
        return FCk_Chain_PathSample{Oldest.Get_Location() + Tangent_AtArcDistance(InS) * ExtrapolationCm, Oldest.Get_Rotation(), InS};
    }

    const auto Index = DoGet_BracketStart(InS);
    const auto& First = Get_Sample(Index);
    const auto& Second = Get_Sample(Index + 1);
    const auto Alpha = (static_cast<double>(InS) - First.Get_ArcDistanceCm())
        / (static_cast<double>(Second.Get_ArcDistanceCm()) - First.Get_ArcDistanceCm());
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
    { return FVector::ZeroVector; }

    if (_Count < 2)
    { return FVector::ZeroVector; }

    const auto Index = DoGet_BracketStart(InS);
    const auto Direction = ck_chain_kernel::DoGet_StableDirection(Get_Sample(Index + 1).Get_Location() - Get_Sample(Index).Get_Location());
    const auto HasDirection = NOT Direction.IsZero();
    CK_ENSURE_IF_NOT(HasDirection, TEXT("Chain history segment must have a finite nonzero direction"))
    { return FVector::ZeroVector; }

    return Direction;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::chain::FPathHistory::Slice_Behind(
        float InMaxS) const
    -> FPathHistory
{
    const auto IsValidDistance = FMath::IsFinite(InMaxS);
    CK_ENSURE_IF_NOT(IsValidDistance, TEXT("Chain history slice arc distance must be finite [{}]"), InMaxS)
    { return {}; }

    auto Result = FPathHistory{};
    auto SliceCount = 0;
    while (SliceCount < _Count && Get_Sample(SliceCount).Get_ArcDistanceCm() <= InMaxS)
    {
        ++SliceCount;
    }

    if (SliceCount == 0)
    { return Result; }

    const auto Origin = static_cast<double>(Get_Sample(SliceCount - 1).Get_ArcDistanceCm());
    auto PreviousArcDistance = 0.0f;
    for (auto Index = 0; Index < SliceCount; ++Index)
    {
        const auto RebasedArcDistance = static_cast<double>(Get_Sample(Index).Get_ArcDistanceCm()) - Origin;
        const auto IsRepresentable = ck_chain_kernel::DoGet_IsRepresentableArc(RebasedArcDistance);
        CK_ENSURE_IF_NOT(IsRepresentable, TEXT("Chain history slice arc rebase exceeded the finite float domain"))
        { return {}; }

        const auto ArcDistance = static_cast<float>(RebasedArcDistance);
        const auto IsOrdered = Index == 0 || ArcDistance > PreviousArcDistance;
        CK_ENSURE_IF_NOT(IsOrdered, TEXT("Chain history slice arc rebase cannot preserve sample ordering"))
        { return {}; }

        PreviousArcDistance = ArcDistance;
    }

    Result.Reserve_ForDistance(static_cast<float>(FMath::Max(0, SliceCount - 2)), 1.0f);
    for (auto Index = 0; Index < SliceCount; ++Index)
    {
        const auto& Sample = Get_Sample(Index);
        Result.DoAppend({Sample.Get_Location(), Sample.Get_Rotation(),
            static_cast<float>(static_cast<double>(Sample.Get_ArcDistanceCm()) - Origin)});
    }

    return Result;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::chain::FPathHistory::Get_HeadArcDistance() const
    -> float
{
    CK_ENSURE_IF_NOT(_Count > 0, TEXT("Chain history has no head sample"))
    { return 0.0f; }

    return Get_Sample(_Count - 1).Get_ArcDistanceCm();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::chain::FPathHistory::Get_OldestArcDistance() const
    -> float
{
    CK_ENSURE_IF_NOT(_Count > 0, TEXT("Chain history has no oldest sample"))
    { return 0.0f; }

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
    check(InIndex >= 0 && InIndex < _Count);
    return _Buffer[static_cast<int32>((static_cast<int64>(_Start) + InIndex) % _Buffer.Num())];
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
    check(_Buffer.Num() > 0);
    const auto Index = static_cast<int32>((static_cast<int64>(_Start) + _Count) % _Buffer.Num());
    _Buffer[Index] = InSample;
    if (_Count == _Buffer.Num())
    {
        _Start = static_cast<int32>((static_cast<int64>(_Start) + 1) % _Buffer.Num());
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
    { return 0.0f; }

    const auto NewestArcDistance = InHistory.Get_HeadArcDistance();
    const auto IsValidHeadPose = ck_chain_kernel::DoGet_IsValidPose(InHeadPose);
    CK_ENSURE_IF_NOT(IsValidHeadPose, TEXT("Chain solver head pose must be finite and normalized"))
    { return NewestArcDistance; }

    const auto Leading = ck_chain_kernel::DoGet_LeadingChord(InHistory, InHeadPose);
    return Leading.IsSet() ? Leading.GetValue().ArcDistanceCm : NewestArcDistance;
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
    { return {}; }

    const auto IsValidPose = ck_chain_kernel::DoGet_IsValidPose(InLinkCurrentPose);
    CK_ENSURE_IF_NOT(IsValidPose, TEXT("Chain link current pose must be finite and normalized"))
    { return {}; }

    const auto IsValidUp = NOT InUpNormalized.ContainsNaN() && InUpNormalized.IsNormalized();
    CK_ENSURE_IF_NOT(IsValidUp, TEXT("Chain solver up vector must be finite and normalized"))
    { return {}; }

    const auto IsValidHeadPose = ck_chain_kernel::DoGet_IsValidPose(InHeadPose);
    CK_ENSURE_IF_NOT(IsValidHeadPose, TEXT("Chain solver head pose must be finite and normalized"))
    { return {}; }

    const auto IsValidOrientation = ck_chain_kernel::DoGet_IsValidOrientation(InOrientation);
    CK_ENSURE_IF_NOT(IsValidOrientation, TEXT("Chain solver orientation is invalid [{}]"), InOrientation)
    { return {}; }

    const auto IsValidSeed = ck_chain_kernel::DoGet_IsValidSeed(InSeed);
    CK_ENSURE_IF_NOT(IsValidSeed, TEXT("Chain solver history seed is invalid [{}]"), InSeed)
    { return {}; }

    if (InHistory.Get_NumSamples() == 0)
    { return {}; }

    const auto Leading = ck_chain_kernel::DoGet_LeadingChord(InHistory, InHeadPose);
    if (NOT Leading.IsSet())
    { return {}; }

    // The newest arc is never negative, so a leading arc and a distance in [0, FLT_MAX] keep this in float range.
    const auto ArcDistance = static_cast<float>(static_cast<double>(Leading.GetValue().ArcDistanceCm) - InDistanceFromHeadCm);
    const auto& Newest = InHistory.Get_Sample(InHistory.Get_NumSamples() - 1);
    const auto PendingChord = Leading.GetValue().ChordCm;
    auto Location = FVector::ZeroVector;
    auto SampledRotation = FQuat::Identity;
    auto Tangent = FVector::ZeroVector;
    if (ArcDistance >= Newest.Get_ArcDistanceCm())
    {
        const auto HasPendingChord = PendingChord > KINDA_SMALL_NUMBER;
        const auto Alpha = HasPendingChord ? (static_cast<double>(ArcDistance) - Newest.Get_ArcDistanceCm()) / PendingChord : 1.0;
        Location = FMath::Lerp(Newest.Get_Location(), InHeadPose.GetLocation(), Alpha);
        SampledRotation = FQuat::Slerp(Newest.Get_Rotation(), InHeadPose.GetRotation(), Alpha);
        Tangent = HasPendingChord
            ? ck_chain_kernel::DoGet_StableDirection(InHeadPose.GetLocation() - Newest.Get_Location())
            : InHistory.Tangent_AtArcDistance(Newest.Get_ArcDistanceCm());
    }
    else
    {
        const auto Sample = InHistory.Sample_AtArcDistance(ArcDistance, InSeed);
        if (NOT Sample.IsSet())
        { return {}; }

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
    { return; }

    const auto IsValidHead = ck_chain_kernel::DoGet_IsValidPose(InHeadPose);
    CK_ENSURE_IF_NOT(IsValidHead, TEXT("Chain distance solver head pose must be finite and normalized"))
    { return; }

    const auto IsValidUp = NOT InUpNormalized.ContainsNaN() && InUpNormalized.IsNormalized();
    CK_ENSURE_IF_NOT(IsValidUp, TEXT("Chain solver up vector must be finite and normalized"))
    { return; }

    for (auto Index = 0; Index < InOutLinkPoses.Num(); ++Index)
    {
        const auto IsValidLink = FMath::IsFinite(InSegmentLengthsCm[Index]) && InSegmentLengthsCm[Index] >= 0.0f
            && ck_chain_kernel::DoGet_IsValidPose(InOutLinkPoses[Index]);
        CK_ENSURE_IF_NOT(IsValidLink, TEXT("Chain distance solver link input is invalid [{}]"), Index)
        { return; }

        const auto Orientation = InOrientations[Index];
        const auto IsValidOrientation = ck_chain_kernel::DoGet_IsValidOrientation(Orientation);
        CK_ENSURE_IF_NOT(IsValidOrientation, TEXT("Chain distance solver orientation is invalid [{}]"), Orientation)
        { return; }
    }

    // Copied before the first write: the head or the up vector may alias an element of InOutLinkPoses.
    const auto HeadLocation = InHeadPose.GetLocation();
    const auto HeadBackward = -InHeadPose.GetRotation().GetNormalized().GetForwardVector();
    const auto UpNormalized = InUpNormalized;

    const auto SolveSegment = [&](int32 InIndex, const FVector& InPredecessor) -> TOptional<ck_chain_kernel::FSolvedSegment>
    {
        const auto Offset = InOutLinkPoses[InIndex].GetLocation() - InPredecessor;
        if (Offset.ContainsNaN())
        { return {}; }

        const auto Direction = ck_chain_kernel::DoGet_StableLength(Offset) < KINDA_SMALL_NUMBER
            ? HeadBackward
            : ck_chain_kernel::DoGet_StableDirection(Offset);
        const auto Location = InPredecessor + Direction * InSegmentLengthsCm[InIndex];
        if (Location.ContainsNaN())
        { return {}; }

        const auto SolvedLength = ck_chain_kernel::DoGet_StableLength(Location - InPredecessor);
        if (FMath::Abs(SolvedLength - InSegmentLengthsCm[InIndex]) > ck_chain_kernel::SegmentLengthToleranceCm)
        { return {}; }

        return ck_chain_kernel::FSolvedSegment{Location, Direction};
    };

    auto Predecessor = HeadLocation;
    for (auto Index = 0; Index < InOutLinkPoses.Num(); ++Index)
    {
        const auto Segment = SolveSegment(Index, Predecessor);
        const auto IsRepresentable = Segment.IsSet();
        CK_ENSURE_IF_NOT(IsRepresentable, TEXT("Chain distance solver produced an unrepresentable segment [{}]"), Index)
        { return; }

        Predecessor = Segment.GetValue().Location;
    }

    Predecessor = HeadLocation;
    for (auto Index = 0; Index < InOutLinkPoses.Num(); ++Index)
    {
        const auto Segment = SolveSegment(Index, Predecessor).GetValue();
        auto& Pose = InOutLinkPoses[Index];
        Pose.SetLocation(Segment.Location);
        if (InSegmentLengthsCm[Index] > 0.0f && InOrientations[Index] != ECk_Chain_LinkOrientation::KeepOwn)
        {
            Pose.SetRotation(ck_chain_kernel::DoGet_Orientation(-Segment.Direction, UpNormalized, Pose.GetRotation()));
        }

        Predecessor = Segment.Location;
    }
}

// --------------------------------------------------------------------------------------------------------------------
