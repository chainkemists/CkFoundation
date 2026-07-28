#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Signal/CkSignal_Fragment_Data.h"

#include "CkLagCompensation/LagCompProjectile/CkLagCompProjectile_Fragment_Data.h"

#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkLagCompProjectile_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck { class FProcessor_LagCompProjectile_HandleRequests; }

// --------------------------------------------------------------------------------------------------------------------

/**
 * Lag-compensated launching for BallisticMotion entities.
 *
 * Server flow: on receiving a (late) fire request, call Request_LaunchCompensated with the
 * shooter's ping as the compensation window. The trajectory is anchored in the past, the catch-up
 * window is swept against every RewindHistory entity (OnRewindHit per confirmed hit), and the
 * deterministic simulation takes over from there.
 *
 * Remote-proxy flow: no extra machinery — launch via BallisticMotion with OverrideTime in the
 * past (½ shooter ping + ½ own ping) and the closed form catches the projectile up visually.
 */
UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_BallisticMotion"))
class CKLAGCOMPENSATION_API UCk_Utils_LagCompProjectile_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_LagCompProjectile_UE);

public:
    friend class ck::FProcessor_LagCompProjectile_HandleRequests;

public:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|LagCompProjectile",
        DisplayName="[Ck][LagCompProjectile] Request Launch Compensated",
        meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_BallisticMotion
    Request_LaunchCompensated(
        UPARAM(ref) FCk_Handle_BallisticMotion& InHandle,
        const FCk_Request_LagCompProjectile_LaunchCompensated& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|LagCompProjectile",
              DisplayName = "[Ck][LagCompProjectile] Bind To OnRewindHit")
    static FCk_Handle_BallisticMotion
    BindTo_OnRewindHit(
        UPARAM(ref) FCk_Handle_BallisticMotion& InHandle,
        const FCk_Delegate_LagCompProjectile_OnRewindHit& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|LagCompProjectile",
              DisplayName = "[Ck][LagCompProjectile] Unbind From OnRewindHit")
    static FCk_Handle_BallisticMotion
    UnbindFrom_OnRewindHit(
        UPARAM(ref) FCk_Handle_BallisticMotion& InHandle,
        const FCk_Delegate_LagCompProjectile_OnRewindHit& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|LagCompProjectile",
              DisplayName = "[Ck][LagCompProjectile] Bind To OnLaunchCompensated")
    static FCk_Handle_BallisticMotion
    BindTo_OnLaunchCompensated(
        UPARAM(ref) FCk_Handle_BallisticMotion& InHandle,
        const FCk_Delegate_LagCompProjectile_OnLaunchCompensated& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|LagCompProjectile",
              DisplayName = "[Ck][LagCompProjectile] Unbind From OnLaunchCompensated")
    static FCk_Handle_BallisticMotion
    UnbindFrom_OnLaunchCompensated(
        UPARAM(ref) FCk_Handle_BallisticMotion& InHandle,
        const FCk_Delegate_LagCompProjectile_OnLaunchCompensated& InDelegate);
};

// --------------------------------------------------------------------------------------------------------------------
