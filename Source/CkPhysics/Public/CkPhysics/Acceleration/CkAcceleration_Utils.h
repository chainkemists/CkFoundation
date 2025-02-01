#pragma once

#include "CkAcceleration_Fragment_Data.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcsExt/CkEcsExt_Utils.h"
#include "CkEcsExt/EntityHolder/CkEntityHolder_Utils.h"
#include "CkCore/Macros/CkMacros.h"


#include "CkNet/CkNet_Utils.h"
#include "CkPhysics/Acceleration/CkAcceleration_Fragment.h"
#include "CkRecord/Record/CkRecord_Utils.h"

#include "CkAcceleration_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_BulkAccelerationModifier_AddNewTargets;
    class FProcessor_BulkAccelerationModifier_Setup;
    class FProcessor_AccelerationModifier_Teardown;
    class FProcessor_Acceleration_Setup;
}

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKPHYSICS_API UCk_Utils_Acceleration_UE : public UCk_Utils_Ecs_Net_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Acceleration_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_Acceleration);

public:
    friend class UCk_Utils_AccelerationModifier_UE;

    friend class ck::FProcessor_AccelerationModifier_Teardown;
    friend class ck::FProcessor_Acceleration_Setup;

private:
    struct AccelerationTarget_Utils : public ck::TUtils_EntityHolder<ck::FFragment_Acceleration_Target> {};

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Acceleration",
              DisplayName="[Ck][Acceleration] Add Feature")
    static FCk_Handle_Acceleration
    Add(
        UPARAM(ref) FCk_Handle InHandle,
        const FCk_Fragment_Acceleration_ParamsData& InParams,
        ECk_Replication InReplicates = ECk_Replication::Replicates);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Acceleration",
              DisplayName="[Ck][Acceleration] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Acceleration",
        DisplayName="[Ck][Acceleration] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_Acceleration
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Acceleration",
        DisplayName="[Ck][Acceleration] Handle -> Acceleration Handle",
        meta = (CompactNodeTitle = "<AsAcceleration>", BlueprintAutocast))
    static FCk_Handle_Acceleration
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid Acceleration Handle",
        Category = "Ck|Utils|Acceleration",
        meta = (CompactNodeTitle = "INVALID_AccelerationHandle", Keywords = "make"))
    static FCk_Handle_Acceleration
    Get_InvalidHandle() { return {}; };


public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Acceleration",
              DisplayName="[Ck][Acceleration] Get Acceleration")
    static FVector
    Get_CurrentAcceleration(
        const FCk_Handle_Acceleration& InHandle);

public:
    static auto
    Request_OverrideAcceleration(
        UPARAM(ref) FCk_Handle_Acceleration& InHandle,
        const FVector& InNewAcceleration) -> void;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKPHYSICS_API UCk_Utils_AccelerationChannel_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_AccelerationChannel_UE);

private:
    class RecordOfAccelerationChannels_Utils : public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfAccelerationChannels> {};

public:
    friend class UCk_Utils_Ecs_Base_UE;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Acceleration|Channel",
              DisplayName="Add Multiple New Acceleration Channels")
    static void
    AddMultiple(
        FCk_Handle InAccelerationOwnerEntity,
        FGameplayTagContainer InAccelerationChannels);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Acceleration|Channel",
              DisplayName="Add New Acceleration Channel")
    static void
    Add(
        FCk_Handle InAccelerationOwnerEntity,
        FGameplayTag InAccelerationChannel);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Acceleration|Channel",
              DisplayName="Has Acceleration Channel")
    static bool
    Has(
        FCk_Handle InAccelerationOwnerEntity,
        FGameplayTag InAccelerationChannel);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Acceleration|Channel",
              DisplayName="Ensure Has Acceleration Channel")
    static bool
    Ensure(
        FCk_Handle InAccelerationOwnerEntity,
        FGameplayTag InAccelerationChannel);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Acceleration|Channel",
              DisplayName="Get Is Acceleration Channel Affected By Other Channel")
    static bool
    Get_IsAffectedByOtherChannel(
        FCk_Handle InAccelerationOwnerEntity,
        FGameplayTag InOtherAccelerationChannel);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Acceleration|Channel",
              DisplayName="Get Is Acceleration Channel Affected By Any Other Channel")
    static bool
    Get_IsAffectedByAnyOtherChannel(
        FCk_Handle InAccelerationOwnerEntity,
        FGameplayTagContainer InOtherAccelerationChannels);

private:
    static auto
    Has(
        FCk_Handle InHandle) -> bool;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKPHYSICS_API UCk_Utils_AccelerationModifier_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_AccelerationModifier_UE);

private:
    class RecordOfAccelerationModifiers_Utils : public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfAccelerationModifiers> {};

public:
    friend class UCk_Utils_Ecs_Base_UE;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Acceleration|Modifier",
              DisplayName="Add New Acceleration Modifier")
    static UPARAM(DisplayName = "Acceleration Modifier Handle") FCk_Handle
    Add(
        FCk_Handle InAccelerationOwnerEntity,
        FGameplayTag InModifierName,
        const FCk_Fragment_AccelerationModifier_ParamsData& InParams);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Acceleration|Modifier",
              DisplayName="Has Acceleration Modifier")
    static bool
    Has(
        FCk_Handle InAccelerationOwnerEntity,
        FGameplayTag InModifierName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Acceleration|Modifier",
              DisplayName="Ensure Has Acceleration Modifier")
    static bool
    Ensure(
        FCk_Handle InAccelerationOwnerEntity,
        FGameplayTag InModifierName);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Acceleration|Modifier",
              DisplayName="Remove Acceleration Modifier")
    static void
    Remove(
        FCk_Handle InAccelerationOwnerEntity,
        FGameplayTag InModifierName);

private:
    static auto
    Has(
        const FCk_Handle& InHandle) -> bool;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKPHYSICS_API UCk_Utils_BulkAccelerationModifier_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_BulkAccelerationModifier_UE);

private:
    class RecordOfBulkAccelerationModifiers_Utils : public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfBulkAccelerationModifiers> {};

public:
    friend class UCk_Utils_Ecs_Base_UE;
    friend class ck::FProcessor_BulkAccelerationModifier_AddNewTargets;
    friend class ck::FProcessor_BulkAccelerationModifier_Setup;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Acceleration|BulkModifier",
              DisplayName="Add New Bulk Acceleration Modifier")
    static UPARAM(DisplayName = "Bulk Acceleration Modifier Handle") FCk_Handle
    Add(
        UPARAM(ref) FCk_Handle InHandle,
        FGameplayTag InModifierName,
        const FCk_Fragment_BulkAccelerationModifier_ParamsData& InParams);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Acceleration|BulkModifier",
              DisplayName="Has Bulk Acceleration Modifier")
    static bool
    Has(
        FCk_Handle InHandle,
        FGameplayTag InModifierName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Acceleration|BulkModifier",
              DisplayName="Ensure Has Bulk Acceleration Modifier")
    static bool
    Ensure(
        FCk_Handle InHandle,
        FGameplayTag InModifierName);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Acceleration|BulkModifier",
              DisplayName="Remove Bulk Acceleration Modifier")
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
              Category = "Ck|Utils|Acceleration|BulkModifier",
              DisplayName="Request Add Bulk Acceleration Modifier Target")
    static void
    Request_AddTarget(
        FCk_Handle InHandle,
        FGameplayTag InModifierName,
        const FCk_Request_BulkAccelerationModifier_AddTarget& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Acceleration|BulkModifier",
              DisplayName="Request Remove Bulk Acceleration Modifier Target")
    static void
    Request_RemoveTarget(
        FCk_Handle InHandle,
        FGameplayTag InModifierName,
        const FCk_Request_BulkAccelerationModifier_RemoveTarget& InRequest);

private:
    static void
    DoRequest_AddTarget(
        FCk_Handle AccelerationModifierEntity,
        const FCk_Request_BulkAccelerationModifier_AddTarget& InRequest);

    static void
    DoRequest_RemoveTarget(
        FCk_Handle AccelerationModifierEntity,
        const FCk_Request_BulkAccelerationModifier_RemoveTarget& InRequest);
};

// --------------------------------------------------------------------------------------------------------------------
