#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Signal/CkSignal_Fragment_Data.h"

#include "CkEcsExt/CkEcsExt_Utils.h"

#include "CkProjectile/Homing/CkHoming_Fragment_Data.h"
#include "CkProjectile/Homing/CkHoming_ProNav.h"

#include "CkHoming_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

/**
 * Proportional-navigation homing for any entity that moves through Velocity + Acceleration
 * (e.g. anything set up via UCk_Utils_Projectile_UE::Add). While a target is set, the Homing
 * processor owns the entity's Acceleration channel: it writes (base + guidance) every frame and
 * restores the base when homing clears, disables, or loses its target
 */
UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_Homing"))
class CKPROJECTILE_API UCk_Utils_Homing_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Homing_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_Homing);

public:
    // Requires the entity to already have Velocity, Acceleration and Transform features
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Homing",
              DisplayName="[Ck][Homing] Add Feature")
    static FCk_Handle_Homing
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_Homing_ParamsData& InParams);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Homing",
              DisplayName="[Ck][Homing] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Homing",
        DisplayName="[Ck][Homing] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_Homing
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Homing",
        DisplayName="[Ck][Homing] Handle -> Homing Handle",
        meta = (CompactNodeTitle = "<AsHoming>", BlueprintAutocast))
    static FCk_Handle_Homing
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid Homing Handle",
        Category = "Ck|Utils|Homing",
        meta = (CompactNodeTitle = "INVALID_HomingHandle", Keywords = "make"))
    static FCk_Handle_Homing
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Homing",
              DisplayName="[Ck][Homing] Request Set Target Entity")
    static FCk_Handle_Homing
    Request_SetTargetEntity(
        UPARAM(ref) FCk_Handle_Homing& InHandle,
        const FCk_Request_Homing_SetTargetEntity& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Homing",
              DisplayName="[Ck][Homing] Request Set Target Location")
    static FCk_Handle_Homing
    Request_SetTargetLocation(
        UPARAM(ref) FCk_Handle_Homing& InHandle,
        const FCk_Request_Homing_SetTargetLocation& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Homing",
              DisplayName="[Ck][Homing] Request Clear Target")
    static FCk_Handle_Homing
    Request_ClearTarget(
        UPARAM(ref) FCk_Handle_Homing& InHandle);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Homing",
              DisplayName="[Ck][Homing] Request Set Desired Time To Impact")
    static FCk_Handle_Homing
    Request_SetDesiredTimeToImpact(
        UPARAM(ref) FCk_Handle_Homing& InHandle,
        const FCk_Request_Homing_SetDesiredTimeToImpact& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Homing",
              DisplayName="[Ck][Homing] Request Enable/Disable")
    static FCk_Handle_Homing
    Request_EnableDisable(
        UPARAM(ref) FCk_Handle_Homing& InHandle,
        const FCk_Request_Homing_EnableDisable& InRequest);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Homing",
              DisplayName="[Ck][Homing] Get Is Active")
    static bool
    Get_IsActive(
        const FCk_Handle_Homing& InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Homing",
              DisplayName="[Ck][Homing] Get Target Mode")
    static ECk_Homing_TargetMode
    Get_TargetMode(
        const FCk_Handle_Homing& InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Homing",
              DisplayName="[Ck][Homing] Get Target Entity")
    static FCk_Handle
    Get_TargetEntity(
        const FCk_Handle_Homing& InHandle);

    // The guidance state evaluated on the most recent homing update — zeroed until the first update
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Homing",
              DisplayName="[Ck][Homing] Get Last Guidance State")
    static FCk_Homing_GuidanceState
    Get_LastGuidanceState(
        const FCk_Handle_Homing& InHandle);

public:
    /**
     * Where to aim so a projectile fired at InProjectileSpeed intercepts the target, assuming
     * constant velocities and no drag/gravity. Pass the shooter's velocity when the projectile
     * inherits it
     */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Homing",
              DisplayName="[Ck][Homing] Get Firing Solution")
    static FCk_Homing_FiringSolution
    Get_FiringSolution(
        const FVector& InShooterLocation,
        const FVector& InShooterVelocity,
        const FVector& InTargetLocation,
        const FVector& InTargetVelocity,
        float InProjectileSpeed,
        ECk_Homing_InterceptPreference InPreference = ECk_Homing_InterceptPreference::EarliestIntercept);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Homing",
              DisplayName = "[Ck][Homing] Bind To OnTargetMissed")
    static FCk_Handle_Homing
    BindTo_OnTargetMissed(
        UPARAM(ref) FCk_Handle_Homing& InHandle,
        const FCk_Delegate_Homing_OnTargetMissed& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Homing",
              DisplayName = "[Ck][Homing] Unbind From OnTargetMissed")
    static FCk_Handle_Homing
    UnbindFrom_OnTargetMissed(
        UPARAM(ref) FCk_Handle_Homing& InHandle,
        const FCk_Delegate_Homing_OnTargetMissed& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Homing",
              DisplayName = "[Ck][Homing] Bind To OnTargetLost")
    static FCk_Handle_Homing
    BindTo_OnTargetLost(
        UPARAM(ref) FCk_Handle_Homing& InHandle,
        const FCk_Delegate_Homing_OnTargetLost& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Homing",
              DisplayName = "[Ck][Homing] Unbind From OnTargetLost")
    static FCk_Handle_Homing
    UnbindFrom_OnTargetLost(
        UPARAM(ref) FCk_Handle_Homing& InHandle,
        const FCk_Delegate_Homing_OnTargetLost& InDelegate);
};

// --------------------------------------------------------------------------------------------------------------------
