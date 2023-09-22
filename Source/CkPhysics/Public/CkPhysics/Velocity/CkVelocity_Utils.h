#pragma once

#include "CkVelocity_Fragment_Params.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcsBasics/CkEcsBasics_Utils.h"
#include "CkEcsBasics/EntityHolder/CkEntityHolder_Utils.h"
#include "CkCore/Macros/CkMacros.h"


#include "CkNet/CkNet_Utils.h"
#include "CkPhysics/Velocity/CkVelocity_Fragment.h"
#include "CkRecord/Record/CkRecord_Utils.h"

#include "CkVelocity_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_BulkVelocityModifier_AddNewTargets;
    class FProcessor_BulkVelocityModifier_Setup;
    class FProcessor_VelocityModifier_Teardown;
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
    friend class UCk_Utils_VelocityModifier_UE;

    friend class ck::FProcessor_VelocityModifier_Teardown;
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
class CKPHYSICS_API UCk_Utils_VelocityChannel_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_VelocityChannel_UE);

private:
    class RecordOfVelocityChannels_Utils : public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfVelocityChannels> {};

public:
    friend class UCk_Utils_Ecs_Base_UE;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Velocity|Channel",
              DisplayName="Add Multiple New Velocity Channels")
    static void
    AddMultiple(
        FCk_Handle InVelocityOwnerEntity,
        FGameplayTagContainer InVelocityChannels);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Velocity|Channel",
              DisplayName="Add New Velocity Channel")
    static void
    Add(
        FCk_Handle InVelocityOwnerEntity,
        FGameplayTag InVelocityChannel);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Velocity|Channel",
              DisplayName="Has Velocity Channel")
    static bool
    Has(
        FCk_Handle InVelocityOwnerEntity,
        FGameplayTag InVelocityChannel);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Velocity|Channel",
              DisplayName="Ensure Has Velocity Channel")
    static bool
    Ensure(
        FCk_Handle InVelocityOwnerEntity,
        FGameplayTag InVelocityChannel);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Velocity|Channel",
              DisplayName="Get Is Velocity Channel Affected By Other Channel")
    static bool
    Get_IsAffectedByOtherChannel(
        FCk_Handle InVelocityOwnerEntity,
        FGameplayTag InOtherVelocityChannel);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Velocity|Channel",
              DisplayName="Get Is Velocity Channel Affected By Any Other Channel")
    static bool
    Get_IsAffectedByAnyOtherChannel(
        FCk_Handle InVelocityOwnerEntity,
        FGameplayTagContainer InOtherVelocityChannels);

private:
    static auto
    Has(
        FCk_Handle InHandle) -> bool;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKPHYSICS_API UCk_Utils_VelocityModifier_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_VelocityModifier_UE);

private:
    class RecordOfVelocityModifiers_Utils : public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfVelocityModifiers> {};

public:
    friend class UCk_Utils_Ecs_Base_UE;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Velocity|Modifier",
              DisplayName="Add New Velocity Modifier")
    static UPARAM(DisplayName = "Velocity Modifier Handle") FCk_Handle
    Add(
        FCk_Handle InVelocityOwnerEntity,
        FGameplayTag InModifierName,
        const FCk_Fragment_VelocityModifier_ParamsData& InParams);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Velocity|Modifier",
              DisplayName="Has Velocity Modifier")
    static bool
    Has(
        FCk_Handle InVelocityOwnerEntity,
        FGameplayTag InModifierName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Velocity|Modifier",
              DisplayName="Ensure Has Velocity Modifier")
    static bool
    Ensure(
        FCk_Handle InVelocityOwnerEntity,
        FGameplayTag InModifierName);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Velocity|Modifier",
              DisplayName="Remove Velocity Modifier")
    static void
    Remove(
        FCk_Handle InVelocityOwnerEntity,
        FGameplayTag InModifierName);

private:
    static auto
    Has(
        FCk_Handle InHandle) -> bool;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKPHYSICS_API UCk_Utils_BulkVelocityModifier_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_BulkVelocityModifier_UE);

private:
    class RecordOfBulkVelocityModifiers_Utils : public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfBulkVelocityModifiers> {};

public:
    friend class UCk_Utils_Ecs_Base_UE;
    friend class ck::FProcessor_BulkVelocityModifier_AddNewTargets;
    friend class ck::FProcessor_BulkVelocityModifier_Setup;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Velocity|BulkModifier",
              DisplayName="Add New Bulk Velocity Modifier")
    static UPARAM(DisplayName = "Bulk Velocity Modifier Handle") FCk_Handle
    Add(
        FCk_Handle InHandle,
        FGameplayTag InModifierName,
        const FCk_Fragment_BulkVelocityModifier_ParamsData& InParams);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Velocity|BulkModifier",
              DisplayName="Has Bulk Velocity Modifier")
    static bool
    Has(
        FCk_Handle InHandle,
        FGameplayTag InModifierName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Velocity|BulkModifier",
              DisplayName="Ensure Has Bulk Velocity Modifier")
    static bool
    Ensure(
        FCk_Handle InHandle,
        FGameplayTag InModifierName);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Velocity|BulkModifier",
              DisplayName="Remove Bulk Velocity Modifier")
    static void
    Remove(
        FCk_Handle InHandle,
        FGameplayTag InModifierName);

private:
    static auto
    Has(
        FCk_Handle InHandle) -> bool;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Velocity|BulkModifier",
              DisplayName="Request Add Bulk Velocity Modifier Target")
    static void
    Request_AddTarget(
        FCk_Handle InHandle,
        FGameplayTag InModifierName,
        const FCk_Request_BulkVelocityModifier_AddTarget& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Velocity|BulkModifier",
              DisplayName="Request Remove Bulk Velocity Modifier Target")
    static void
    Request_RemoveTarget(
        FCk_Handle InHandle,
        FGameplayTag InModifierName,
        const FCk_Request_BulkVelocityModifier_RemoveTarget& InRequest);

private:
    static void
    DoRequest_AddTarget(
        FCk_Handle VelocityModifierEntity,
        const FCk_Request_BulkVelocityModifier_AddTarget& InRequest);

    static void
    DoRequest_RemoveTarget(
        FCk_Handle VelocityModifierEntity,
        const FCk_Request_BulkVelocityModifier_RemoveTarget& InRequest);
};

// --------------------------------------------------------------------------------------------------------------------
