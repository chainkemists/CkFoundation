#include "CkIntegrator_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FCk_Processor_Integrator_Update::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FCk_Fragment_Integrator_Current& InIntegrator,
            FCk_Fragment_Velocity_Current& InVelocity,
            const FCk_Fragment_Acceleration_Current& InAcceleration) const
        -> void
    {
        const auto& oldVelocity = InVelocity.Get_CurrentVelocity();
        const auto& acceleration = InAcceleration.Get_CurrentAcceleration();
        const auto& newVelocity = oldVelocity + acceleration * InDeltaT.Get_Seconds();

        InVelocity = FCk_Fragment_Velocity_Current{newVelocity};

        const auto [velDir, velLength] = [&]()
        {
            FVector dir;
            float length;
            newVelocity.ToDirectionAndLength(dir, length);

            return std::make_pair(dir, length);
        }();

        const auto ut             = velLength * InDeltaT.Get_Seconds();
        const auto at2            = acceleration.Length() * (FMath::Square(InDeltaT.Get_Seconds()));
        const auto distance       = ut + 0.5f * at2;
        const auto distanceOffset = velDir * distance;

        InIntegrator._DistanceOffset = distanceOffset;
    }
}

// --------------------------------------------------------------------------------------------------------------------
