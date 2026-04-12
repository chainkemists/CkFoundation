#pragma once

#include "CkSmTask_EntityScript.h"

#include "CkStateMachine/StateMachine/CkStateMachine_Fragment_Data.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Request_Data.h"

#include "CkStateMachine/State/EntityScripts/CkSmState_EntityScript.h"

#include "CkSmTask_SubStateMachine.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Blueprintable, BlueprintType,
    meta = (DisplayName = "SM Task: Sub-StateMachine"))
class CKSTATEMACHINE_API UCk_SmTask_SubStateMachine : public UCk_SmTask_EntityScript
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_SmTask_SubStateMachine);

    UCk_SmTask_SubStateMachine()
    {
        _TaskMode = ECk_SmTaskMode::EnterExitOnly;
    }

protected:
    auto
    BeginPlay() -> void override;

    auto
    EndPlay() -> void override;

public:
    UFUNCTION(BlueprintNativeEvent, BlueprintPure,
        Category = "Ck|SM|Task|SubStateMachine",
        DisplayName = "[Ck][SM] Get SubStateMachine Class")
    TSubclassOf<UCk_SmState_EntityScript>
    Get_SubStateMachineClass(
        FCk_Handle_SmTask InTaskEntity) const;

    virtual TSubclassOf<UCk_SmState_EntityScript>
    Get_SubStateMachineClass_Implementation(
        FCk_Handle_SmTask InTaskEntity) const;

protected:
    UFUNCTION()
    void
    OnSubSmStopped(
        FCk_Handle_StateMachine InHandle,
        FCk_Sm_Payload_OnStopped InPayload);

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
        Category = "SM|Task|SubStateMachine",
        meta = (AllowPrivateAccess = true))
    TSubclassOf<UCk_SmState_EntityScript> _InitialStateClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly,
        Category = "SM|Task|SubStateMachine",
        meta = (AllowPrivateAccess = true))
    ECk_SmTask_SubSm_CompletionBehavior _CompletionBehavior =
        ECk_SmTask_SubSm_CompletionBehavior::KeepRunning;

public:
    CK_PROPERTY_GET(_InitialStateClass);
    CK_PROPERTY_GET(_CompletionBehavior);

private:
    FCk_Handle_StateMachine _SubSmHandle;
};

// --------------------------------------------------------------------------------------------------------------------
