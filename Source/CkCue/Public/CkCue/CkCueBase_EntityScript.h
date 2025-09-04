#pragma once

#include "CkEcs/EntityScript/CkEntityScript.h"
#include "CkTimer/CkTimer_Fragment_Data.h"

#include <GameplayTagContainer.h>

#include "CkCueBase_EntityScript.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Cue_LifetimeBehavior : uint8
{
    AfterOneFrame, // Self-destruct after one frame
    Persistent,    // Stay alive until manually destroyed
    Timed,         // Stay alive for specified duration, then self-destruct
    Custom         // As defined by the derived class
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Cue_LifetimeBehavior);

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, Blueprintable, BlueprintType)
class CKCUE_API UCk_CueBase_EntityScript : public UCk_EntityScript_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_CueBase_EntityScript);

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
        Category = "Cue",
        meta = (AllowPrivateAccess = true))
    FGameplayTag _CueName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cue Lifetime",
        meta = (AllowPrivateAccess = true))
    ECk_Cue_LifetimeBehavior _LifetimeBehavior = ECk_Cue_LifetimeBehavior::AfterOneFrame;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cue Lifetime",
        meta = (AllowPrivateAccess = true, EditCondition = "_LifetimeBehavior == ECk_Cue_LifetimeBehavior::Timed"))
    FCk_Time _LifetimeDuration = FCk_Time{30.0f};

public:
    CK_PROPERTY_GET(_CueName);
    CK_PROPERTY_GET(_LifetimeBehavior);
    CK_PROPERTY_GET(_LifetimeDuration);

protected:
    auto BeginPlay() -> void override;

#if WITH_EDITOR
public:
    // Override to add asset registry tags for efficient cue discovery
    auto
    GetAssetRegistryTags(
        FAssetRegistryTagsContext Context) const -> void override;
#endif

private:
    UFUNCTION()
    void
    OnLifetimeExpired(
        FCk_Handle_Timer InTimer,
        FCk_Chrono InChrono,
        FCk_Time InDeltaT);
};

// --------------------------------------------------------------------------------------------------------------------
