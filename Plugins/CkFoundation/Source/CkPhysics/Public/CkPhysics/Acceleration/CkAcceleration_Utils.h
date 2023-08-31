#pragma once

#include "CkAcceleration_Fragment_Params.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcsBasics/EntityHolder/CkEntityHolder_Utils.h"

#include "CkMacros/CkMacros.h"

#include "CkNet/CkNet_Utils.h"
#include "CkPhysics/Acceleration/CkAcceleration_Fragment.h"

#include "CkAcceleration_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FCk_Processor_AccelerationModifier_SingleTarget_Teardown;
}

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKPHYSICS_API UCk_Utils_Acceleration_UE : public UCk_Utils_Ecs_Net_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Acceleration_UE);

public:
    friend class UCk_Utils_Acceleration_SingleTarget_UE;
    friend class UCk_Utils_Acceleration_MultipleTargets_UE;

    friend class ck::FCk_Processor_AccelerationModifier_SingleTarget_Teardown;

private:
    struct AccelerationTarget_Utils : public ck::TUtils_EntityHolder<ck::FCk_Fragment_Acceleration_Target> {};

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
class CKPHYSICS_API UCk_Utils_Acceleration_SingleTarget_UE : public UCk_Utils_Ecs_Net_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Acceleration_SingleTarget_UE);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AccelerationModifier",
              DisplayName="Add Single Target Acceleration Modifier")
    static void
    Add(
        FCk_Handle InHandle,
        const FCk_Fragment_AccelerationModifier_SingleTarget_ParamsData& InParams);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AccelerationModifier",
              DisplayName="Has Single Target Acceleration Modifier")
    static bool
    Has(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AccelerationModifier",
              DisplayName="Ensure Has Single Target Acceleration Modifier")
    static bool
    Ensure(
        FCk_Handle InHandle);
};

// --------------------------------------------------------------------------------------------------------------------
