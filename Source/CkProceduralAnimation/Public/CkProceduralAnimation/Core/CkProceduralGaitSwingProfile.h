#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CoreMinimal.h"
#include "Templates/Function.h"

namespace ck
{
    struct CKPROCEDURALANIMATION_API FProceduralGaitSwingProfile
    {
        CK_GENERATED_BODY(FProceduralGaitSwingProfile);
    public:
        static constexpr int32 NumSamples = 17;
        auto Reset() -> void;
        auto IsEaseValid() const -> bool { return _EaseValid; }
        auto IsArcValid() const -> bool { return _ArcValid; }
        // Tables are sampled atomically: a nonfinite authored value rejects the whole update.
        auto SetEase(TFunctionRef<float(float)> InEvaluator) -> bool;
        auto SetArc(TFunctionRef<float(float)> InEvaluator) -> bool;
        auto ClearEase() -> void { _EaseValid = false; }
        auto ClearArc() -> void { _ArcValid = false; }
        // Nonfinite phase returns NaN without indexing the table.
        auto SampleEase(float InPhase) const -> float;
        auto SampleArc(float InPhase) const -> float;
    private:
        static auto Fill(float (&OutTable)[NumSamples], TFunctionRef<float(float)> InEvaluator) -> bool;
        static auto Read(const float (&InTable)[NumSamples], float InPhase) -> float;
        float _EaseTable[NumSamples] = {};
        float _ArcTable[NumSamples] = {};
        bool _EaseValid = false;
        bool _ArcValid = false;
    };

    namespace procedural_gait_swing
    {
        [[nodiscard]] FORCEINLINE auto ParametricArc(float InPhase, float InApexPhase, float InSharpness) -> float
        {
            const auto Apex = FMath::Clamp(InApexPhase, 0.05f, 0.95f);
            const auto WarpedPhase = InPhase <= Apex
                ? 0.5f * InPhase / Apex
                : 0.5f + 0.5f * (InPhase - Apex) / (1.0f - Apex);
            return FMath::Pow(FMath::Max(FMath::Sin(WarpedPhase * PI), 0.0f), FMath::Max(InSharpness, 0.1f));
        }
    }
}
