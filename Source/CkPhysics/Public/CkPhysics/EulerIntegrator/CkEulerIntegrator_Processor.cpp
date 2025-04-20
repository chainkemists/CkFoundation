#include "CkEulerIntegrator_Processor.h"

#include "CkEcs/Net/TimeSync/CkNetTimeSync_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_EulerIntegrator_DoOnePredictiveUpdate::
        DoTick(TimeType InDeltaT)
        -> void
    {
        TProcessor::DoTick(InDeltaT);

        _TransientEntity.Clear<FTag_EulerIntegrator_DoOnePredictiveUpdate>();
    }

    auto
        FProcessor_EulerIntegrator_DoOnePredictiveUpdate::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_EulerIntegrator_Current& InIntegrator,
            FFragment_Velocity_Current& InVelocity,
            const FFragment_Acceleration_Current& InAcceleration,
            const TObjectPtr<UCk_Fragment_Velocity_Rep>& InVelocityRO) const
        -> void
    {
        CK_ENSURE_VALID_UNREAL_WORLD_IF_NOT(InVelocityRO)
        { return; }

        const auto& Latency = UCk_Utils_Net_UE::Get_AveragePing(UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle));

        InDeltaT = FCk_Time{Latency};

        const auto& OldVelocity = InVelocity.Get_CurrentVelocity();
        const auto& Acceleration = InAcceleration.Get_CurrentAcceleration();
        const auto& NewVelocity = OldVelocity + Acceleration * InDeltaT.Get_Seconds();

        InVelocity = FFragment_Velocity_Current{NewVelocity};

        const auto [VelDir, VelLength] = [&]()
        {
            FVector Dir;
            float Length;
            NewVelocity.ToDirectionAndLength(Dir, Length);

            return std::make_pair(Dir, Length);
        }();

        const auto Ut             = VelLength * InDeltaT.Get_Seconds();
        const auto AT2            = Acceleration.Length() * (FMath::Square(InDeltaT.Get_Seconds()));
        const auto Distance       = Ut + 0.5f * AT2;
        const auto DistanceOffset = VelDir * Distance;

        InIntegrator._DistanceOffset = DistanceOffset;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_EulerIntegrator_Update::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_EulerIntegrator_Current& InIntegrator,
            FFragment_Velocity_Current& InVelocity,
            const FFragment_Acceleration_Current& InAcceleration)
        -> void
    {
        const auto& OldVelocity = InVelocity.Get_CurrentVelocity();
        const auto& Acceleration = InAcceleration.Get_CurrentAcceleration();
        const auto& NewVelocity = OldVelocity + Acceleration * InDeltaT.Get_Seconds();

        InVelocity = FFragment_Velocity_Current{NewVelocity};

        const auto [VelDir, VelLength] = [&]()
        {
            FVector Dir;
            float Length;
            NewVelocity.ToDirectionAndLength(Dir, Length);

            return std::make_pair(Dir, Length);
        }();

        const auto Ut             = VelLength * InDeltaT.Get_Seconds();
        const auto AT2            = Acceleration.Length() * (FMath::Square(InDeltaT.Get_Seconds()));
        const auto Distance       = Ut + 0.5f * AT2;
        const auto DistanceOffset = VelDir * Distance;

        InIntegrator._DistanceOffset = DistanceOffset;
    }
}

// --------------------------------------------------------------------------------------------------------------------
