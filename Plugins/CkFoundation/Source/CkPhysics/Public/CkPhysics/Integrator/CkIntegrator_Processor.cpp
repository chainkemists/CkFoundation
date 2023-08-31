#include "CkIntegrator_Processor.h"

#include "CkNet/TimeSync/CkNetTimeSync_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    auto
        FCk_Processor_Integrator_Setup::
        Tick(TimeType InDeltaT)
        -> void
    {
        TProcessor::Tick(InDeltaT);

        _Registry.Clear<FCk_Tag_Integrator_Setup>();
    }

    auto
        FCk_Processor_Integrator_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FCk_Fragment_Integrator_Current& InIntegrator,
            FCk_Fragment_Velocity_Current& InVelocity,
            const FCk_Fragment_Acceleration_Current& InAcceleration) const
        -> void
    {
        const auto& VelocityRO = InHandle.Get<TObjectPtr<UCk_Fragment_Velocity_Rep>>();

        // TODO: this was hacked together quickly, improve this code
        const auto PlayerController =
            Cast<APlayerController>(Cast<APawn>(VelocityRO->GetOwningActor())->Controller.Get());

        const auto& Latency = UCk_Utils_NetTimeSync_UE::Get_PlayerRoundTripTime(PlayerController, InHandle);

        InDeltaT = FCk_Time{Latency};

        const auto& OldVelocity = InVelocity.Get_CurrentVelocity();
        const auto& Acceleration = InAcceleration.Get_CurrentAcceleration();
        const auto& NewVelocity = OldVelocity + Acceleration * InDeltaT.Get_Seconds();

        InVelocity = FCk_Fragment_Velocity_Current{NewVelocity};

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
        FCk_Processor_Integrator_Update::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FCk_Fragment_Integrator_Current& InIntegrator,
            FCk_Fragment_Velocity_Current& InVelocity,
            const FCk_Fragment_Acceleration_Current& InAcceleration) const
        -> void
    {
        const auto& OldVelocity = InVelocity.Get_CurrentVelocity();
        const auto& Acceleration = InAcceleration.Get_CurrentAcceleration();
        const auto& NewVelocity = OldVelocity + Acceleration * InDeltaT.Get_Seconds();

        InVelocity = FCk_Fragment_Velocity_Current{NewVelocity};

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
}

// --------------------------------------------------------------------------------------------------------------------
