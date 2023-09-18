#pragma once

#include "CkAcceleration_Fragment_Params.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcsBasics/CkEcsBasics_Utils.h"
#include "CkEcsBasics/EntityHolder/CkEntityHolder_Utils.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkNet/CkNet_Utils.h"
#include "CkPhysics/Acceleration/CkAcceleration_Fragment.h"
#include "CkRecord/Record/CkRecord_Utils.h"

#include "CkAcceleration_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_AccelerationModifier_SingleTarget_Teardown;
    class FProcessor_Acceleration_Setup;
}

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKPHYSICS_API UCk_Utils_Acceleration_UE : public UCk_Utils_Ecs_Net_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Acceleration_UE);

public:
    friend class UCk_Utils_AccelerationModifier_SingleTarget_UE;

    friend class ck::FProcessor_AccelerationModifier_SingleTarget_Teardown;
    friend class ck::FProcessor_Acceleration_Setup;

private:
    struct AccelerationTarget_Utils : public ck::TUtils_EntityHolder<ck::FFragment_Acceleration_Target> {};

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Acceleration",
              DisplayName="Add Acceleration")
    static void
    Add(
        FCk_Handle InHandle,
        const FCk_Fragment_Acceleration_ParamsData& InParams);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Acceleration",
              DisplayName="Has Acceleration")
    static bool
    Has(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Acceleration",
              DisplayName="Ensure Has Acceleration")
    static bool
    Ensure(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Acceleration")
    static FVector
    Get_CurrentAcceleration(
        FCk_Handle InHandle);

public:
    static auto
    Request_OverrideAcceleration(
        FCk_Handle     InHandle,
        const FVector& InNewAcceleration) -> void;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKPHYSICS_API UCk_Utils_AccelerationModifier_SingleTarget_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_AccelerationModifier_SingleTarget_UE);

private:
    class RecordOfAccelerationModifiers_Utils : public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfAccelerationModifiers> {};

public:
    friend class UCk_Utils_Ecs_Base_UE;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AccelerationModifier",
              DisplayName="Add New Single Target Acceleration Modifier")
    static void
    Add(
        FCk_Handle InAccelerationOwnerEntity,
        FGameplayTag InModifierName,
        const FCk_Fragment_AccelerationModifier_SingleTarget_ParamsData& InParams);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AccelerationModifier",
              DisplayName="Has Single Target Acceleration Modifier")
    static bool
    Has(
        FCk_Handle InHandle,
        FGameplayTag InModifierName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AccelerationModifier",
              DisplayName="Ensure Has Single Target Acceleration Modifier")
    static bool
    Ensure(
        FCk_Handle InHandle,
        FGameplayTag InModifierName);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AccelerationModifier",
              DisplayName="Remove Single Target Acceleration Modifier")
    static void
    Remove(
        FCk_Handle InHandle,
        FGameplayTag InModifierName);

private:
    static auto
    Has(
        FCk_Handle InHandle) -> bool;
};

// --------------------------------------------------------------------------------------------------------------------
