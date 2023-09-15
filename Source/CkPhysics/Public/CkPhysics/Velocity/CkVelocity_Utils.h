#pragma once

#include "CkVelocity_Fragment_Params.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcsBasics/CkEcsBasics_Utils.h"
#include "CkEcsBasics/EntityHolder/CkEntityHolder_Utils.h"
#include "CkMacros/CkMacros.h"


#include "CkNet/CkNet_Utils.h"
#include "CkPhysics/Velocity/CkVelocity_Fragment.h"
#include "CkRecord/Record/CkRecord_Utils.h"

#include "CkVelocity_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_VelocityModifier_SingleTarget_Teardown;
    class FProcessor_Velocity_Setup;
}

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKPHYSICS_API UCk_Utils_Velocity_UE : public UCk_Utils_Ecs_Net_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Velocity_UE);

public:
    friend class UCk_Utils_VelocityModifier_SingleTarget_UE;

    friend class ck::FProcessor_VelocityModifier_SingleTarget_Teardown;
    friend class ck::FProcessor_Velocity_Setup;

private:
    struct VelocityTarget_Utils : public ck::TUtils_EntityHolder<ck::FFragment_Velocity_Target> {};

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Velocity",
              DisplayName="Add Velocity")
    static void
    Add(
        FCk_Handle InHandle,
        const FCk_Fragment_Velocity_ParamsData& InParams);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Velocity",
              DisplayName="Has Velocity")
    static bool
    Has(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Velocity",
              DisplayName="Ensure Has Velocity")
    static bool
    Ensure(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Velocity")
    static FVector
    Get_CurrentVelocity(
        FCk_Handle InHandle);

public:
    static auto
    Request_OverrideVelocity(
        FCk_Handle InHandle,
        const FVector& InNewVelocity) -> void;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKPHYSICS_API UCk_Utils_VelocityModifier_SingleTarget_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_VelocityModifier_SingleTarget_UE);

private:
    class RecordOfVelocityModifiers_Utils : public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfVelocityModifiers> {};

public:
    friend class UCk_Utils_Ecs_Base_UE;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VelocityModifier",
              DisplayName="Add New Single Target Velocity Modifier")
    static void
    Add(
        FCk_Handle InVelocityOwnerEntity,
        FGameplayTag InModifierName,
        const FCk_Fragment_VelocityModifier_SingleTarget_ParamsData& InParams);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VelocityModifier",
              DisplayName="Has Single Target Velocity Modifier")
    static bool
    Has(
        FCk_Handle InHandle,
        FGameplayTag InModifierName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VelocityModifier",
              DisplayName="Ensure Has Single Target Velocity Modifier")
    static bool
    Ensure(
        FCk_Handle InHandle,
        FGameplayTag InModifierName);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VelocityModifier",
              DisplayName="Remove Single Target Velocity Modifier")
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
