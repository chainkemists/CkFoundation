#include "CkProceduralAnimation/Core/CkProceduralFootProbe.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_procedural_foot_probe
{
    auto
        Get_IsTrustedHit(
            const ck::FProceduralSurfaceHit& InHit,
            const FVector& InRayDirection)
        -> bool
    {
        return InHit.Get_Hit() && InHit.Get_Fraction() > 0.0f && InHit.Get_Fraction() <= 1.0f
            && NOT InHit.Get_Position().ContainsNaN() && NOT InHit.Get_Normal().ContainsNaN()
            && NOT InHit.Get_Normal().IsNearlyZero()
            && FVector::DotProduct(InHit.Get_Normal(), InRayDirection) < -KINDA_SMALL_NUMBER;
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProceduralFootProbeState::
        Advance(
            bool InHit,
            FCk_Time InDeltaTime,
            FCk_Time InGraceDuration)
        -> bool
    {
        if (NOT FMath::IsFinite(InDeltaTime.Get_Seconds()) || InDeltaTime < FCk_Time{}
            || NOT FMath::IsFinite(InGraceDuration.Get_Seconds()) || InGraceDuration < FCk_Time{})
        { return false; }
        if (InDeltaTime == FCk_Time{})
        { return true; }
        if (InHit)
        {
            Reset();
            return true;
        }
        _MissingDuration = FMath::Min(_MissingDuration + InDeltaTime, InGraceDuration);
        _State = _MissingDuration >= InGraceDuration
            ? EProceduralFootProbeState::Lost : EProceduralFootProbeState::Guessing;
        return true;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProceduralFootProbeState::
        Reset()
        -> void
    {
        _MissingDuration = FCk_Time{};
        _State = EProceduralFootProbeState::Grounded;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        MakeProceduralGroundProbeSpan(
            int32 InAttempt,
            float InUpDistance,
            float InDownDistance,
            float InOutwardLean)
        -> FProceduralGroundProbeSpan
    {
        constexpr float UpScale[ProceduralGroundProbeAttempts] = {1.0f, 4.0f, 12.0f};
        constexpr float DownScale[ProceduralGroundProbeAttempts] = {1.0f, 1.5f, 2.5f};
        const auto Attempt = FMath::Clamp(InAttempt, 0, ProceduralGroundProbeAttempts - 1);
        return FProceduralGroundProbeSpan{}
            .Set_UpDistance(InUpDistance * UpScale[Attempt])
            .Set_DownDistance(InDownDistance * DownScale[Attempt])
            .Set_OutwardLean(Attempt == 0 ? InOutwardLean : 0.0f);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        ResolveProceduralTouchdown(
            const FVector& InPlant,
            const FVector& InValidatedTarget,
            const FVector& InTargetNormal,
            const FVector& InUp,
            float InHalfSpan,
            FProceduralSurfaceRayCast InRayCast)
        -> FProceduralTouchdown
    {
        const auto Normal = InTargetNormal.GetSafeNormal();
        auto Rays = int32{0};
        const auto Confirm = [&](const FVector& InPoint) -> TOptional<FProceduralSurfaceHit>
        {
            const auto Hit = InRayCast(InPoint + Normal * InHalfSpan, InPoint - Normal * InHalfSpan);
            ++Rays;
            if (NOT ck_procedural_foot_probe::Get_IsTrustedHit(Hit, -Normal))
            { return {}; }
            return Hit;
        };

        const auto AtPlant = Confirm(InPlant);
        if (AtPlant.IsSet())
        {
            return FProceduralTouchdown{}.Set_Position(AtPlant->Get_Position()).Set_Normal(AtPlant->Get_Normal().GetSafeNormal())
                .Set_Trusted(true).Set_Rays(Rays);
        }

        if (NOT InValidatedTarget.Equals(InPlant))
        {
            const auto AtValidatedTarget = Confirm(InValidatedTarget);
            if (AtValidatedTarget.IsSet())
            {
                return FProceduralTouchdown{}.Set_Position(AtValidatedTarget->Get_Position())
                    .Set_Normal(AtValidatedTarget->Get_Normal().GetSafeNormal()).Set_Trusted(true).Set_Rays(Rays);
            }
        }

        return FProceduralTouchdown{}.Set_Position(InPlant).Set_Normal(InUp).Set_Trusted(false).Set_Rays(Rays);
    }
}

// --------------------------------------------------------------------------------------------------------------------
