#include "CkEulerIntegrator_Processor.h"

#include "CkNet/TimeSync/CkNetTimeSync_Utils.h"

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

        const auto OutermostPawn = UCk_Utils_Actor_UE::Get_OutermostPawn(InVelocityRO);

        CK_ENSURE_IF_NOT(OutermostPawn, TEXT("Expected ReplicatedObject [{}] to have an owning Pawn in the parent chain. "
            "Unable to perform predictive integration."), InVelocityRO)
        { return; }

        const auto PlayerController = Cast<APlayerController>(OutermostPawn->Controller.Get());
        const auto& Latency = UCk_Utils_NetTimeSync_UE::Get_PlayerRoundTripTime(PlayerController, InHandle);

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
