#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Signal/CkSignal_Fragment_Data.h"

#include "CkProjectile/BallisticMotion/CkBallisticMotion_Fragment_Data.h"

#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkBallisticMotion_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck { class FProcessor_BallisticMotion_HandleRequests; }

// --------------------------------------------------------------------------------------------------------------------

/**
 * Deterministic ballistic flight for an entity. The entity follows a closed-form linear-drag
 * trajectory anchored on world time — identical on every machine regardless of tick rate.
 *
 * Composition:
 *  - Requires a Transform on the entity.
 *  - For hit detection, also compose a CkShapes shape + a Probe (use MotionQuality LinearCast for
 *    fast projectiles). Impacts then bounce/stop the motion and broadcast OnImpact.
 *  - Without a Probe the entity flies until explicitly stopped.
 */
UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_BallisticMotion"))
class CKPROJECTILE_API UCk_Utils_BallisticMotion_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_BallisticMotion_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_BallisticMotion);

public:
    friend class ck::FProcessor_BallisticMotion_HandleRequests;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName="[Ck][BallisticMotion] Add Feature")
    static FCk_Handle_BallisticMotion
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_BallisticMotion_ParamsData& InParams);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|BallisticMotion",
              DisplayName="[Ck][BallisticMotion] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|BallisticMotion",
        DisplayName="[Ck][BallisticMotion] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_BallisticMotion
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|BallisticMotion",
        DisplayName="[Ck][BallisticMotion] Handle -> BallisticMotion Handle",
        meta = (CompactNodeTitle = "<AsBallisticMotion>", BlueprintAutocast))
    static FCk_Handle_BallisticMotion
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid BallisticMotion Handle",
        Category = "Ck|Utils|BallisticMotion",
        meta = (CompactNodeTitle = "INVALID_BallisticMotionHandle", Keywords = "make"))
    static FCk_Handle_BallisticMotion
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|BallisticMotion",
        DisplayName="[Ck][BallisticMotion] Get Is In Flight")
    static bool
    Get_IsInFlight(
        const FCk_Handle_BallisticMotion& InHandle);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|BallisticMotion",
        DisplayName="[Ck][BallisticMotion] Get Current Velocity")
    static FVector
    Get_CurrentVelocity(
        const FCk_Handle_BallisticMotion& InHandle);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|BallisticMotion",
        DisplayName="[Ck][BallisticMotion] Get Initial Conditions")
    static FCk_Ballistic_InitialConditions
    Get_InitialConditions(
        const FCk_Handle_BallisticMotion& InHandle);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|BallisticMotion",
        DisplayName="[Ck][BallisticMotion] Get Trajectory Segment Index")
    static int32
    Get_TrajectorySegmentIndex(
        const FCk_Handle_BallisticMotion& InHandle);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|BallisticMotion",
        DisplayName="[Ck][BallisticMotion] Get Trajectory Params")
    static FCk_Ballistic_TrajectoryParams
    Get_TrajectoryParams(
        const FCk_Handle_BallisticMotion& InHandle);

public:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|BallisticMotion",
        DisplayName="[Ck][BallisticMotion] Request Launch")
    static FCk_Handle_BallisticMotion
    Request_Launch(
        UPARAM(ref) FCk_Handle_BallisticMotion& InHandle,
        const FCk_Request_BallisticMotion_Launch& InRequest);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|BallisticMotion",
        DisplayName="[Ck][BallisticMotion] Request Stop")
    static FCk_Handle_BallisticMotion
    Request_Stop(
        UPARAM(ref) FCk_Handle_BallisticMotion& InHandle,
        const FCk_Request_BallisticMotion_Stop& InRequest);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|BallisticMotion",
              DisplayName = "[Ck][BallisticMotion] Bind To OnImpact")
    static FCk_Handle_BallisticMotion
    BindTo_OnImpact(
        UPARAM(ref) FCk_Handle_BallisticMotion& InHandle,
        const FCk_Delegate_BallisticMotion_OnImpact& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|BallisticMotion",
              DisplayName = "[Ck][BallisticMotion] Unbind From OnImpact")
    static FCk_Handle_BallisticMotion
    UnbindFrom_OnImpact(
        UPARAM(ref) FCk_Handle_BallisticMotion& InHandle,
        const FCk_Delegate_BallisticMotion_OnImpact& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|BallisticMotion",
              DisplayName = "[Ck][BallisticMotion] Bind To OnTrajectoryChanged")
    static FCk_Handle_BallisticMotion
    BindTo_OnTrajectoryChanged(
        UPARAM(ref) FCk_Handle_BallisticMotion& InHandle,
        const FCk_Delegate_BallisticMotion_OnTrajectoryChanged& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|BallisticMotion",
              DisplayName = "[Ck][BallisticMotion] Unbind From OnTrajectoryChanged")
    static FCk_Handle_BallisticMotion
    UnbindFrom_OnTrajectoryChanged(
        UPARAM(ref) FCk_Handle_BallisticMotion& InHandle,
        const FCk_Delegate_BallisticMotion_OnTrajectoryChanged& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|BallisticMotion",
              DisplayName = "[Ck][BallisticMotion] Bind To OnStopped")
    static FCk_Handle_BallisticMotion
    BindTo_OnStopped(
        UPARAM(ref) FCk_Handle_BallisticMotion& InHandle,
        const FCk_Delegate_BallisticMotion_OnStopped& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|BallisticMotion",
              DisplayName = "[Ck][BallisticMotion] Unbind From OnStopped")
    static FCk_Handle_BallisticMotion
    UnbindFrom_OnStopped(
        UPARAM(ref) FCk_Handle_BallisticMotion& InHandle,
        const FCk_Delegate_BallisticMotion_OnStopped& InDelegate);
};

// --------------------------------------------------------------------------------------------------------------------
