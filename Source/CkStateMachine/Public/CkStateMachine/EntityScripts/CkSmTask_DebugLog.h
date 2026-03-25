#pragma once

#include "CkSmTask_EntityScript.h"

#include "CkSmTask_DebugLog.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Blueprintable, BlueprintType,
    meta = (DisplayName = "SM Task: Debug Log"))
class CKSTATEMACHINE_API UCk_SmTask_DebugLog : public UCk_SmTask_EntityScript
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_SmTask_DebugLog);

    UCk_SmTask_DebugLog()
    {
        _TaskMode = ECk_SmTaskMode::EnterExitOnly;
    }

    auto
    OnStateEnter() -> void override;

    auto
    OnStateExit() -> void override;

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
        Category = "SM|Task|DebugLog",
        meta = (AllowPrivateAccess = true))
    FString _EnterMessage = TEXT("State Enter");

    UPROPERTY(EditAnywhere, BlueprintReadOnly,
        Category = "SM|Task|DebugLog",
        meta = (AllowPrivateAccess = true))
    FString _ExitMessage = TEXT("State Exit");

public:
    CK_PROPERTY_GET(_EnterMessage);
    CK_PROPERTY_GET(_ExitMessage);
};

// --------------------------------------------------------------------------------------------------------------------
