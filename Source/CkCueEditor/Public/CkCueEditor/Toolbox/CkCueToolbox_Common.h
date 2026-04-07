#pragma once

#include "CkCue/CkCueSubsystem_Base.h"

#include <CoreMinimal.h>
#include <GameplayTagContainer.h>
#include <Templates/SubclassOf.h>

#include "CkCueToolbox_Common.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCUEEDITOR_API FCk_CueToolbox_CueInfo
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_CueToolbox_CueInfo);

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGameplayTag CueName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSubclassOf<UCk_CueBase_EntityScript> CueClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString SubsystemName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool IsValid = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString ValidationMessage;

public:
    FCk_CueToolbox_CueInfo() = default;
    FCk_CueToolbox_CueInfo(const FGameplayTag& InCueName, TSubclassOf<UCk_CueBase_EntityScript> InCueClass, const FString& InSubsystemName)
        : CueName(InCueName), CueClass(InCueClass), SubsystemName(InSubsystemName) {}
};

// --------------------------------------------------------------------------------------------------------------------
