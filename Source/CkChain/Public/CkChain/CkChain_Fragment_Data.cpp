#include "CkChain_Fragment_Data.h"

#include "NativeGameplayTags.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Label_Chain, TEXT("Chain"));

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Chain_Spec::
    Get_IsValid() const
    -> bool
{
    const auto IsValidSolver = _Solver == ECk_Chain_Solver::PathHistory || _Solver == ECk_Chain_Solver::DistanceConstraint;
    const auto IsValidHistorySeed = _HistorySeed == ECk_Chain_HistorySeed::StraightBehindHead
        || _HistorySeed == ECk_Chain_HistorySeed::HoldUntilCovered;
    const auto IsValidNetPolicy = _NetPolicy == ECk_Chain_NetPolicy::AuthorityOnly || _NetPolicy == ECk_Chain_NetPolicy::Everywhere;
    const auto IsValidStartingState = _StartingState == ECk_EnableDisable::Enable || _StartingState == ECk_EnableDisable::Disable;
    const auto IsValidSpacing = FMath::IsFinite(_SampleSpacingCm) && _SampleSpacingCm >= 1.0f;
    const auto IsValidTeleportDistance = FMath::IsFinite(_TeleportDistanceCm) && _TeleportDistanceCm >= 0.0f;
    const auto IsValidUpVector = NOT _UpVector.ContainsNaN() && _UpVector.GetSafeNormal().IsNormalized();

    return IsValidSolver && IsValidHistorySeed && IsValidNetPolicy && IsValidStartingState && IsValidSpacing
        && IsValidTeleportDistance && IsValidUpVector;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_ChainLink_Spec::
    Get_IsValid() const
    -> bool
{
    const auto IsValidDistance = FMath::IsFinite(_DistanceFromHeadCm) && _DistanceFromHeadCm > 0.0f;
    const auto IsValidOrientation = _Orientation == ECk_Chain_LinkOrientation::FollowPath
        || _Orientation == ECk_Chain_LinkOrientation::CopyHead || _Orientation == ECk_Chain_LinkOrientation::KeepOwn;

    return IsValidDistance && IsValidOrientation;
}

// --------------------------------------------------------------------------------------------------------------------
