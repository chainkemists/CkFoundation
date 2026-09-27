#include "CkProceduralAnimation/Core/CkProceduralChainClearance.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Macros/CkMacros.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        ComputeProceduralPoleSwivel(
            const FVector& InHip,
            const FVector& InFoot,
            const FVector& InPole,
            float InDegrees)
        -> FVector
    {
        const auto InputIsFinite = NOT InHip.ContainsNaN() && NOT InFoot.ContainsNaN() && NOT InPole.ContainsNaN()
            && FMath::IsFinite(InDegrees);
        if (NOT InputIsFinite || InDegrees == 0.0f)
        { return InPole; }

        const auto Axis = (InFoot - InHip).GetSafeNormal();
        if (Axis.IsNearlyZero())
        { return InPole; }

        const auto Swivel = FQuat{Axis, FMath::DegreesToRadians(static_cast<double>(InDegrees))};
        return InHip + Swivel.RotateVector(InPole - InHip);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        Get_ProceduralPoleSwivelOrder(
            float InLastClearDegrees,
            TArrayView<float> OutOrder)
        -> int32
    {
        const auto Fan = MakeArrayView(ProceduralPoleSwivelFanDegrees);
        const auto LastClearLeadsTheOrder = InLastClearDegrees != 0.0f && algo::AnyOf(Fan, [&](float InDegrees)
        {
            return InDegrees == InLastClearDegrees;
        });

        auto Count = 0;
        const auto Append = [&](float InDegrees)
        {
            if (Count < OutOrder.Num())
            { OutOrder[Count++] = InDegrees; }
        };

        if (LastClearLeadsTheOrder)
        { Append(InLastClearDegrees); }

        for (const auto Degrees : Fan)
        {
            if (LastClearLeadsTheOrder && Degrees == InLastClearDegrees)
            { continue; }

            Append(Degrees);
        }
        return Count;
    }
}

// --------------------------------------------------------------------------------------------------------------------
