#pragma once

#include "CkEcs/EntityScript/CkEntityScript.h"
#include "CkObjective_Fragment_Data.h"

#include <GameplayTagContainer.h>

#include "CkObjective_EntityScript.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, Blueprintable, BlueprintType)
class CKOBJECTIVE_API UCk_Objective_EntityScript : public UCk_EntityScript_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Objective_EntityScript);

private:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective",
              meta = (AllowPrivateAccess = true, Categories = "Objective"))
    FGameplayTag _ObjectiveName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective",
              meta = (AllowPrivateAccess = true))
    FText _DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective",
              meta = (AllowPrivateAccess = true, MultiLine = true))
    FText _Description;

public:
    CK_PROPERTY_GET(_ObjectiveName);
    CK_PROPERTY_GET(_DisplayName);
    CK_PROPERTY_GET(_Description);

protected:
    auto
    Construct(
        FCk_Handle& InHandle,
        const FInstancedStruct& InSpawnParams) -> ECk_EntityScript_ConstructionFlow override;

    auto BeginPlay() -> void override;

public:
    UFUNCTION(BlueprintCallable, Category = "Objective",
              DisplayName = "[Ck][Objective] Start")
    void
    StartObjective();

    UFUNCTION(BlueprintCallable, Category = "Objective",
              DisplayName = "[Ck][Objective] Complete")
    void
    CompleteObjective(
        FGameplayTag InMetaData);

    UFUNCTION(BlueprintCallable, Category = "Objective",
              DisplayName = "[Ck][Objective] Fail")
    void
    FailObjective(
        FGameplayTag InMetaData);

    UFUNCTION(BlueprintPure, Category = "Objective",
              DisplayName = "[Ck][Objective] Get Current Status")
    ECk_ObjectiveStatus
    Get_CurrentStatus() const;

protected:
    UFUNCTION(BlueprintImplementableEvent, Category = "Objective",
              DisplayName = "On Objective Started")
    void
    OnObjectiveStarted(
        FCk_Handle_Objective InThisObjective);

    UFUNCTION(BlueprintImplementableEvent, Category = "Objective",
              DisplayName = "On Objective Completed")
    void
    OnObjectiveCompleted(
        FCk_Handle_Objective InThisObjective,
        FGameplayTag InMetaData);

    UFUNCTION(BlueprintImplementableEvent, Category = "Objective",
              DisplayName = "On Objective Failed")
    void
    OnObjectiveFailed(
        FCk_Handle_Objective InThisObjective,
        FGameplayTag InMetaData);

private:
    UFUNCTION()
    void
    HandleStatusChanged(
        FCk_Handle_Objective InObjective,
        ECk_ObjectiveStatus NewStatus);

    UFUNCTION()
    void
    HandleObjectiveCompleted(
        FCk_Handle_Objective InObjective,
        FGameplayTag InMetaData);

    UFUNCTION()
    void
    HandleObjectiveFailed(
        FCk_Handle_Objective InObjective,
        FGameplayTag InMetaData);
};

// --------------------------------------------------------------------------------------------------------------------