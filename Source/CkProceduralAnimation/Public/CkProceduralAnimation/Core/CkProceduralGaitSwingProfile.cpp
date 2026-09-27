#include "CkProceduralAnimation/Core/CkProceduralGaitSwingProfile.h"

#include "CkProceduralAnimation/Core/CkProceduralGaitSolver.h"

#include <limits>

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProceduralGaitSwingProfile::
        Reset()
        -> void
    {
        _EaseValid = false;
        _ArcValid = false;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProceduralGaitSwingProfile::
        Fill(
            float (&OutTable)[NumSamples],
            TFunctionRef<float(float)> InEvaluator)
        -> bool
    {
        float Samples[NumSamples];
        constexpr auto SampleStep = 1.0f / static_cast<float>(NumSamples - 1);
        for (auto Index = 0; Index < NumSamples; ++Index)
        {
            Samples[Index] = InEvaluator(static_cast<float>(Index) * SampleStep);
            if (NOT FMath::IsFinite(Samples[Index]))
            { return false; }
        }
        for (auto Index = 0; Index < NumSamples; ++Index)
        { OutTable[Index] = Samples[Index]; }
        return true;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProceduralGaitSwingProfile::
        SetEase(
            TFunctionRef<float(float)> InEvaluator)
        -> bool
    {
        if (NOT Fill(_EaseTable, InEvaluator))
        { return false; }
        _EaseValid = true;
        return true;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProceduralGaitSwingProfile::
        SetArc(
            TFunctionRef<float(float)> InEvaluator)
        -> bool
    {
        if (NOT Fill(_ArcTable, InEvaluator))
        { return false; }
        _ArcValid = true;
        return true;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProceduralGaitSwingProfile::
        Read(
            const float (&InTable)[NumSamples],
            float InPhase)
        -> float
    {
        const auto Scaled = FMath::Clamp(InPhase, 0.0f, 1.0f) * static_cast<float>(NumSamples - 1);
        const auto Lower = FMath::Clamp(static_cast<int32>(Scaled), 0, NumSamples - 1);
        const auto Upper = FMath::Min(Lower + 1, NumSamples - 1);
        return FMath::Lerp(InTable[Lower], InTable[Upper], Scaled - static_cast<float>(Lower));
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProceduralGaitSwingProfile::
        SampleEase(
            float InPhase) const
        -> float
    {
        if (NOT FMath::IsFinite(InPhase))
        { return std::numeric_limits<float>::quiet_NaN(); }
        return _EaseValid ? Read(_EaseTable, InPhase) : FMath::Clamp(InPhase, 0.0f, 1.0f);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProceduralGaitSwingProfile::
        SampleArc(
            float InPhase) const
        -> float
    {
        if (NOT FMath::IsFinite(InPhase))
        { return std::numeric_limits<float>::quiet_NaN(); }
        return _ArcValid ? Read(_ArcTable, InPhase) : 0.0f;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        ComputeProceduralSwingPoint(
            const FVector& InStart,
            const FVector& InTarget,
            const FVector& InUp,
            const FProceduralGaitSwingSettings& InSwing,
            float InHeight,
            float InAlpha)
        -> FVector
    {
        const auto& Profile = InSwing.Get_Profile();
        const auto Ease = Profile.IsEaseValid() ? Profile.SampleEase(InAlpha) : FMath::SmoothStep(0.0f, 1.0f, InAlpha);
        const auto Arc = Profile.IsArcValid()
            ? Profile.SampleArc(InAlpha)
            : procedural_gait_swing::ParametricArc(InAlpha, InSwing.Get_ApexPhase(), InSwing.Get_ApexSharpness());
        return FMath::Lerp(InStart, InTarget, Ease) + InUp * (Arc * InHeight);
    }
}

// --------------------------------------------------------------------------------------------------------------------
