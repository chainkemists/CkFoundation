#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcsExt/CkEcsExt_Utils.h"

#include "CkJolt/Body/CkJoltBody_Fragment.h"
#include "CkJolt/Body/CkJoltBody_Fragment_Data.h"

#include "CkJoltBody_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_JoltBody"))
class CKJOLT_API UCk_Utils_JoltBody_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_JoltBody_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_JoltBody);

public:
    friend class UCk_Utils_Ecs_Base_UE;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName="[Ck][JoltBody] Add New JoltBody")
    static FCk_Handle_JoltBody
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_JoltBody_ParamsData& InParams);

public:
    // Has Feature
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|JoltBody",
        DisplayName="[Ck][JoltBody] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_JoltBody
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|JoltBody",
        DisplayName="[Ck][JoltBody] Handle -> JoltBody Handle",
        meta = (CompactNodeTitle = "<AsJoltBody>", BlueprintAutocast))
    static FCk_Handle_JoltBody
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid JoltBody Handle",
        Category = "Ck|Utils|JoltBody",
        meta = (CompactNodeTitle = "INVALID_JoltBodyHandle", Keywords = "make"))
    static FCk_Handle_JoltBody
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|JoltBody",
              DisplayName="[Ck][JoltBody] Get Motion Type")
    static ECk_MotionType
    Get_MotionType(
        const FCk_Handle_JoltBody& InJoltBody);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|JoltBody",
              DisplayName="[Ck][JoltBody] Get Sleep State")
    static ECk_Jolt_SleepState
    Get_SleepState(
        const FCk_Handle_JoltBody& InJoltBody);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|JoltBody",
              DisplayName="[Ck][JoltBody] Get Is Body Added")
    static bool
    Get_IsBodyAdded(
        const FCk_Handle_JoltBody& InJoltBody);

    // The body's CURRENT simulation velocity (UE units/s), read straight from the Jolt body via the
    // locking BodyInterface (safe against an in-flight async step). Zero until the body is added.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|JoltBody",
              DisplayName="[Ck][JoltBody] Get Linear Velocity")
    static FVector
    Get_LinearVelocity(
        const FCk_Handle_JoltBody& InJoltBody);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|JoltBody",
              DisplayName="[Ck][JoltBody] Request Set Sleep State")
    static FCk_Handle_JoltBody
    Request_SetSleepState(
        UPARAM(ref) FCk_Handle_JoltBody& InJoltBody,
        const FCk_Request_JoltBody_SetSleepState& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|JoltBody",
              DisplayName="[Ck][JoltBody] Request Add Force")
    static FCk_Handle_JoltBody
    Request_AddForce(
        UPARAM(ref) FCk_Handle_JoltBody& InJoltBody,
        const FCk_Request_JoltBody_AddForce& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|JoltBody",
              DisplayName="[Ck][JoltBody] Request Add Force At Location")
    static FCk_Handle_JoltBody
    Request_AddForceAtLocation(
        UPARAM(ref) FCk_Handle_JoltBody& InJoltBody,
        const FCk_Request_JoltBody_AddForceAtLocation& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|JoltBody",
              DisplayName="[Ck][JoltBody] Request Add Torque")
    static FCk_Handle_JoltBody
    Request_AddTorque(
        UPARAM(ref) FCk_Handle_JoltBody& InJoltBody,
        const FCk_Request_JoltBody_AddTorque& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|JoltBody",
              DisplayName="[Ck][JoltBody] Request Add Impulse")
    static FCk_Handle_JoltBody
    Request_AddImpulse(
        UPARAM(ref) FCk_Handle_JoltBody& InJoltBody,
        const FCk_Request_JoltBody_AddImpulse& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|JoltBody",
              DisplayName="[Ck][JoltBody] Request Add Impulse At Location")
    static FCk_Handle_JoltBody
    Request_AddImpulseAtLocation(
        UPARAM(ref) FCk_Handle_JoltBody& InJoltBody,
        const FCk_Request_JoltBody_AddImpulseAtLocation& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|JoltBody",
              DisplayName="[Ck][JoltBody] Request Add Angular Impulse")
    static FCk_Handle_JoltBody
    Request_AddAngularImpulse(
        UPARAM(ref) FCk_Handle_JoltBody& InJoltBody,
        const FCk_Request_JoltBody_AddAngularImpulse& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|JoltBody",
              DisplayName="[Ck][JoltBody] Request Set Linear Velocity")
    static FCk_Handle_JoltBody
    Request_SetLinearVelocity(
        UPARAM(ref) FCk_Handle_JoltBody& InJoltBody,
        const FCk_Request_JoltBody_SetLinearVelocity& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|JoltBody",
              DisplayName="[Ck][JoltBody] Request Set Angular Velocity")
    static FCk_Handle_JoltBody
    Request_SetAngularVelocity(
        UPARAM(ref) FCk_Handle_JoltBody& InJoltBody,
        const FCk_Request_JoltBody_SetAngularVelocity& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|JoltBody",
              DisplayName="[Ck][JoltBody] Request Teleport")
    static FCk_Handle_JoltBody
    Request_Teleport(
        UPARAM(ref) FCk_Handle_JoltBody& InJoltBody,
        const FCk_Request_JoltBody_Teleport& InRequest);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|JoltBody",
              DisplayName = "[Ck][JoltBody] Bind To OnContactAdded")
    static FCk_Handle_JoltBody
    BindTo_OnJoltBodyContactAdded(
        UPARAM(ref) FCk_Handle_JoltBody& InJoltBody,
        const FCk_Delegate_JoltBody_OnContact& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|JoltBody",
              DisplayName = "[Ck][JoltBody] Unbind From OnContactAdded")
    static FCk_Handle_JoltBody
    UnbindFrom_OnJoltBodyContactAdded(
        UPARAM(ref) FCk_Handle_JoltBody& InJoltBody,
        const FCk_Delegate_JoltBody_OnContact& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|JoltBody",
              DisplayName = "[Ck][JoltBody] Bind To OnContactPersisted")
    static FCk_Handle_JoltBody
    BindTo_OnJoltBodyContactPersisted(
        UPARAM(ref) FCk_Handle_JoltBody& InJoltBody,
        const FCk_Delegate_JoltBody_OnContact& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|JoltBody",
              DisplayName = "[Ck][JoltBody] Unbind From OnContactPersisted")
    static FCk_Handle_JoltBody
    UnbindFrom_OnJoltBodyContactPersisted(
        UPARAM(ref) FCk_Handle_JoltBody& InJoltBody,
        const FCk_Delegate_JoltBody_OnContact& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|JoltBody",
              DisplayName = "[Ck][JoltBody] Bind To OnContactRemoved")
    static FCk_Handle_JoltBody
    BindTo_OnJoltBodyContactRemoved(
        UPARAM(ref) FCk_Handle_JoltBody& InJoltBody,
        const FCk_Delegate_JoltBody_OnContactRemoved& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|JoltBody",
              DisplayName = "[Ck][JoltBody] Unbind From OnContactRemoved")
    static FCk_Handle_JoltBody
    UnbindFrom_OnJoltBodyContactRemoved(
        UPARAM(ref) FCk_Handle_JoltBody& InJoltBody,
        const FCk_Delegate_JoltBody_OnContactRemoved& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|JoltBody",
              DisplayName = "[Ck][JoltBody] Bind To OnSleepStateChanged")
    static FCk_Handle_JoltBody
    BindTo_OnJoltBodySleepStateChanged(
        UPARAM(ref) FCk_Handle_JoltBody& InJoltBody,
        const FCk_Delegate_JoltBody_OnSleepStateChanged& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|JoltBody",
              DisplayName = "[Ck][JoltBody] Unbind From OnSleepStateChanged")
    static FCk_Handle_JoltBody
    UnbindFrom_OnJoltBodySleepStateChanged(
        UPARAM(ref) FCk_Handle_JoltBody& InJoltBody,
        const FCk_Delegate_JoltBody_OnSleepStateChanged& InDelegate);
};

// --------------------------------------------------------------------------------------------------------------------
