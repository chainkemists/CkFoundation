#pragma once

#include "NativeGameplayTags.h"
#include "CkEcs/EntityScript/CkEntityScript.h"
#include "CkTimer/CkTimer_Fragment_Data.h"

#include <GameplayTagContainer.h>

#include "CkCue_EntityScript.generated.h"

/*─────────────────────────────────────────────────────────────────────────────┐
│                            GAMEPLAY TAGS                                     │
└─────────────────────────────────────────────────────────────────────────────*/

CKCUE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Cue_DoNotExecute);

/*─────────────────────────────────────────────────────────────────────────────┐
│                            LIFETIME BEHAVIOR                                 │
└─────────────────────────────────────────────────────────────────────────────*/

UENUM(BlueprintType)
enum class ECk_Cue_LifetimeBehavior : uint8
{
    AfterOneFrame,
    Persistent,
    Timed,
    Custom
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Cue_LifetimeBehavior);

/*─────────────────────────────────────────────────────────────────────────────┐
│                          CONCURRENCY POLICY                                  │
└─────────────────────────────────────────────────────────────────────────────*/

UENUM(BlueprintType)
enum class ECk_Cue_ConcurrencyPolicy : uint8
{
    AllowMultiple,
    RestartExisting
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Cue_ConcurrencyPolicy);

/*─────────────────────────────────────────────────────────────────────────────┐
│                           EXECUTION POLICY                                   │
└─────────────────────────────────────────────────────────────────────────────*/

UENUM(BlueprintType)
enum class ECk_Cue_ExecutionPolicy : uint8
{
    Replicated,
    ReplicatedAndLocal,
    Local
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Cue_ExecutionPolicy);

/*─────────────────────────────────────────────────────────────────────────────┐
│                           CUE BASE ENTITY SCRIPT                             │
└─────────────────────────────────────────────────────────────────────────────*/

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

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
    FGameplayTag Get_CueName() const;

public:
    virtual auto Restart() -> void;

protected:
    auto Construct(
        FCk_Handle& InHandle,
        const FInstancedStruct& InSpawnParams) -> ECk_EntityScript_ConstructionFlow override;
    auto BeginPlay() -> void override;

protected:
    UFUNCTION(BlueprintImplementableEvent,
        Category = "Ck|EntityScript|Cue",
        DisplayName = "Restart")
    void DoRestart(
        FCk_Handle InHandle);

#if WITH_EDITOR
public:
    auto GetAssetRegistryTags(
        FAssetRegistryTagsContext Context) const -> void override;
#endif

private:
    UFUNCTION()
    void OnLifetimeExpired(
        FCk_Handle_Timer InTimer,
        FCk_Chrono InChrono,
        FCk_Time InDeltaT);
};

/*─────────────────────────────────────────────────────────────────────────────┐
│                         GENERIC CUE ENTITY SCRIPT                           │
└─────────────────────────────────────────────────────────────────────────────*/

UCLASS(Abstract, Blueprintable, BlueprintType)
class CKCUE_API UCk_GenericCue_EntityScript : public UCk_CueBase_EntityScript
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_GenericCue_EntityScript);
};
