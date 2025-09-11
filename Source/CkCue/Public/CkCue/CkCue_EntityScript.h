#pragma once

#include "CkEcs/EntityScript/CkEntityScript.h"
#include "CkTimer/CkTimer_Fragment_Data.h"

#include <GameplayTagContainer.h>

#include "CkCue_EntityScript.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Cue_LifetimeBehavior : uint8
{
    // Self-destruct after one frame
    AfterOneFrame,
    // Stay alive until manually destroyed
    Persistent,
    // Stay alive for specified duration, then self-destruct
    Timed,
    // As defined by the derived class
    Custom
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Cue_LifetimeBehavior);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Cue_ConcurrencyPolicy : uint8
{
    // Allow unlimited concurrent instances
    AllowMultiple,
    // Restart existing instances instead of spawning new ones
    RestartExisting
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Cue_ConcurrencyPolicy);

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, NotBlueprintable, BlueprintType)
class CKCUE_API UCk_CueBase_EntityScript : public UCk_EntityScript_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_CueBase_EntityScript);

public:
    UCk_CueBase_EntityScript(const FObjectInitializer& InInitializer);

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cue Concurrency",
    meta = (AllowPrivateAccess = true))
    ECk_Cue_ConcurrencyPolicy _ConcurrencyPolicy = ECk_Cue_ConcurrencyPolicy::AllowMultiple;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cue Lifetime",
        meta = (AllowPrivateAccess = true))
    ECk_Cue_LifetimeBehavior _LifetimeBehavior = ECk_Cue_LifetimeBehavior::AfterOneFrame;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cue Lifetime",
        meta = (AllowPrivateAccess = true, EditCondition = "_LifetimeBehavior == ECk_Cue_LifetimeBehavior::Timed"))
    FCk_Time _LifetimeDuration = FCk_Time{30.0f};

public:
    CK_PROPERTY_GET(_ConcurrencyPolicy);
    CK_PROPERTY_GET(_LifetimeBehavior);
    CK_PROPERTY_GET(_LifetimeDuration);

    UFUNCTION(BlueprintNativeEvent)
    FGameplayTag
    Get_CueName() const;

public:
    virtual auto
    Restart() -> void;

protected:
    auto
    Construct(
        FCk_Handle& InHandle,
        const FInstancedStruct& InSpawnParams) -> ECk_EntityScript_ConstructionFlow override;
    auto BeginPlay() -> void override;

protected:
    UFUNCTION(BlueprintImplementableEvent,
        Category = "Ck|EntityScript|Cue",
        DisplayName = "Restart")
    void
    DoRestart(
        FCk_Handle InHandle);

#if WITH_EDITOR
public:
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

UCLASS(Abstract, Blueprintable, BlueprintType)
class CKCUE_API UCk_GenericCue_EntityScript : public UCk_CueBase_EntityScript
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_GenericCue_EntityScript);
};

// --------------------------------------------------------------------------------------------------------------------
