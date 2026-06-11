#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Time/CkTime.h"

#include "CkProjectile/Ballistic/CkBallistic_Fragment_Data.h"

#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkBallistic_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::ballistics
{
    // p(t) = p0 + ((v0 + k·t·Vinf)·t) / (1 + k·t), as per the Carpentier paper.
    // Stateless — evaluating any time in any order yields identical results on every machine
    CKPROJECTILE_API auto
    Get_PositionAtTime(
        const FCk_Ballistic_InitialConditions& InInitialConditions,
        const FCk_Ballistic_TrajectoryParams& InParams,
        FCk_Time InTimeSinceStart) -> FVector;

    // v(t) = (v0 + k·t·(2 + k·t)·Vinf) / (1 + k·t)², the exact derivative of Get_PositionAtTime.
    // Converges to TerminalVelocity + Wind as t grows
    CKPROJECTILE_API auto
    Get_VelocityAtTime(
        const FCk_Ballistic_InitialConditions& InInitialConditions,
        const FCk_Ballistic_TrajectoryParams& InParams,
        FCk_Time InTimeSinceStart) -> FVector;

    // Inverse of Get_PositionAtTime: the time since segment start at which the trajectory passes
    // through InEndLocation. The quadratic has two roots when the dominant axis reverses (apex);
    // InCurrentVelocity disambiguates whether the projectile is before or after it.
    // InEndLocation is expected to lie on the trajectory
    CKPROJECTILE_API auto
    Get_TimeOfFlightTo(
        const FCk_Ballistic_InitialConditions& InInitialConditions,
        const FCk_Ballistic_TrajectoryParams& InParams,
        const FVector& InEndLocation,
        const FVector& InCurrentVelocity) -> FCk_Time;
}

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKPROJECTILE_API UCk_Utils_Ballistic_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Ballistic_UE);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ballistic",
              DisplayName = "[Ck][Ballistic] Get Position At Time")
    static FVector
    Get_PositionAtTime(
        const FCk_Ballistic_InitialConditions& InInitialConditions,
        const FCk_Ballistic_TrajectoryParams& InParams,
        FCk_Time InTimeSinceStart);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ballistic",
              DisplayName = "[Ck][Ballistic] Get Velocity At Time")
    static FVector
    Get_VelocityAtTime(
        const FCk_Ballistic_InitialConditions& InInitialConditions,
        const FCk_Ballistic_TrajectoryParams& InParams,
        FCk_Time InTimeSinceStart);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ballistic",
              DisplayName = "[Ck][Ballistic] Get Time Of Flight To")
    static FCk_Time
    Get_TimeOfFlightTo(
        const FCk_Ballistic_InitialConditions& InInitialConditions,
        const FCk_Ballistic_TrajectoryParams& InParams,
        const FVector& InEndLocation,
        const FVector& InCurrentVelocity);

    // Sample the trajectory at a fixed interval — for path prediction visuals and debugging
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ballistic",
              DisplayName = "[Ck][Ballistic] Get Sampled Trajectory")
    static TArray<FVector>
    Get_SampledTrajectory(
        const FCk_Ballistic_InitialConditions& InInitialConditions,
        const FCk_Ballistic_TrajectoryParams& InParams,
        FCk_Time InDuration,
        FCk_Time InSampleInterval);
};

// --------------------------------------------------------------------------------------------------------------------
