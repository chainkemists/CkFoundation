#include "CkGroundNav_AgentProfile.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_GroundNav_AgentProfile::
    Get_StandingHeightUu() const
    -> float
{
    switch (_StandingExtents.Get_ShapeType())
    {
        case ECk_Shape_Type::Box:
            return static_cast<float>(_StandingExtents.Get_Box().Get_HalfExtents().Z * 2.0);

        case ECk_Shape_Type::Capsule:
            return (_StandingExtents.Get_Capsule().Get_HalfHeight() +
                    _StandingExtents.Get_Capsule().Get_Radius()) * 2.0f;

        case ECk_Shape_Type::Cylinder:
            return _StandingExtents.Get_Cylinder().Get_HalfHeight() * 2.0f;

        case ECk_Shape_Type::Sphere:
            return _StandingExtents.Get_Sphere().Get_Radius() * 2.0f;

        case ECk_Shape_Type::None:
        default:
            return 0.0f;
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    auto
        Get_ProfileRejection(
            const FCk_GroundNav_AgentProfile& InProfile)
        -> EProfileRejection
    {
        const auto Slope = InProfile.Get_MaxSlopeDegrees();
        const auto SlopeChange = InProfile.Get_MaxSlopeChangeDegrees();
        const auto StepHeight = InProfile.Get_StepHeightUu();
        const auto RoughPerch = InProfile.Get_RoughPerchToleranceUu();
        const auto LedgeSensitivity = InProfile.Get_LedgeSensitivity();

        if (NOT FMath::IsFinite(Slope) || NOT FMath::IsFinite(SlopeChange) ||
            NOT FMath::IsFinite(StepHeight) || NOT FMath::IsFinite(RoughPerch) ||
            NOT FMath::IsFinite(LedgeSensitivity))
        { return EProfileRejection::NonFiniteValue; }

        // A walkable surface is bounded by the vertical: at 90 degrees the surface is a wall and the
        // whole notion of standing on it stops being defined.
        if (Slope < 0.0f || Slope >= 90.0f)
        { return EProfileRejection::SlopeOutOfRange; }

        if (SlopeChange < 0.0f || SlopeChange >= 90.0f)
        { return EProfileRejection::SlopeChangeOutOfRange; }

        if (StepHeight < 0.0f)
        { return EProfileRejection::NegativeStepHeight; }

        if (RoughPerch < 0.0f)
        { return EProfileRejection::NegativeRoughPerchTolerance; }

        if (LedgeSensitivity < 0.0f)
        { return EProfileRejection::NegativeLedgeSensitivity; }

        if (InProfile.Get_StandingHeightUu() <= 0.0f)
        { return EProfileRejection::MissingStandingExtents; }

        return EProfileRejection::None;
    }
}

// --------------------------------------------------------------------------------------------------------------------
