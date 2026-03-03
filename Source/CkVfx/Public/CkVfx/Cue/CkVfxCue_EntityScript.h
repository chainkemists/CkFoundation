#pragma once

#include "CkCue/CkCue_EntityScript.h"
#include "CkVfxCue_Fragment_Data.h"

#include <GameplayTagContainer.h>
#include <NiagaraSystem.h>

#include "CkVfxCue_EntityScript.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FCk_VfxCue_SpawnParams
{
    GENERATED_BODY()

    CK_GENERATED_BODY(FCk_VfxCue_SpawnParams);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX",
              meta = (AllowPrivateAccess = true, ExposeOnSpawn = true))
    FTransform _SpawnTransform = FTransform::Identity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX",
              meta = (AllowPrivateAccess = true, ExposeOnSpawn = true))
    TArray<FCk_VfxCue_UserParameter> _UserParameters;

public:
    CK_PROPERTY_GET(_SpawnTransform);
    CK_PROPERTY(_UserParameters);

    CK_DEFINE_CONSTRUCTORS(FCk_VfxCue_SpawnParams, _SpawnTransform);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, NotBlueprintable, BlueprintType)
class CKVFX_API UCk_VfxCueBase_EntityScript : public UCk_CueBase_EntityScript
{
    GENERATED_BODY()
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(BlueprintType, Blueprintable)
class CKVFX_API UCk_VfxCue_EntityScript : public UCk_VfxCueBase_EntityScript
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_VfxCue_EntityScript);

protected:
    UCk_VfxCue_EntityScript(const FObjectInitializer& InObjectInitializer);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX",
              meta = (AllowPrivateAccess = true, ExposeOnSpawn = true))
    FTransform _SpawnTransform = FTransform::Identity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX",
              meta = (AllowPrivateAccess = true, ExposeOnSpawn = true))
    TArray<FCk_VfxCue_UserParameter> _UserParameters;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX",
              meta = (AllowPrivateAccess = true))
    TObjectPtr<UNiagaraSystem> _Effect;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Playback",
              meta = (AllowPrivateAccess = true))
    ECk_VfxCue_PlaybackBehavior _PlaybackBehavior = ECk_VfxCue_PlaybackBehavior::AutoPlay;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Playback",
              meta = (AllowPrivateAccess = true,
                     EditCondition = "_PlaybackBehavior == ECk_VfxCue_PlaybackBehavior::DelayedPlay",
                     EditConditionHides))
    FCk_Time _DelayTime = FCk_Time{1.0f};

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Duration",
              meta = (AllowPrivateAccess = true))
    ECk_VfxCue_DurationMode _DurationMode = ECk_VfxCue_DurationMode::UseSystemDuration;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Duration",
              meta = (AllowPrivateAccess = true,
                     EditCondition = "_DurationMode == ECk_VfxCue_DurationMode::Override",
                     EditConditionHides))
    FCk_Time _DurationOverride = FCk_Time{5.0f};

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pooling",
              meta = (AllowPrivateAccess = true))
    ECk_VfxCue_PoolingBehavior _PoolingBehavior = ECk_VfxCue_PoolingBehavior::AutoDestroy;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawning",
              meta = (AllowPrivateAccess = true))
    bool _PreCullCheck = false;

public:
    CK_PROPERTY_GET(_Effect);
    CK_PROPERTY_GET(_SpawnTransform);
    CK_PROPERTY_GET(_UserParameters);
    CK_PROPERTY_GET(_PlaybackBehavior);
    CK_PROPERTY_GET(_DelayTime);
    CK_PROPERTY_GET(_DurationMode);
    CK_PROPERTY_GET(_DurationOverride);
    CK_PROPERTY_GET(_PoolingBehavior);
    CK_PROPERTY_GET(_PreCullCheck);

protected:
    auto
    Construct(
        FCk_Handle& InHandle,
        const FInstancedStruct& InSpawnParams)
        -> ECk_EntityScript_ConstructionFlow override;

    auto BeginPlay() -> void override;

    FGameplayTag Get_CueName_Implementation() const override;

    UFUNCTION()
    void
    OnDelayTimerComplete(
        FCk_Handle_Timer InHandle,
        FCk_Chrono InChrono,
        FCk_Time InDeltaT);

public:
    UFUNCTION(BlueprintPure, Category = "VFX Cue")
    bool
    Get_IsConfigurationValid() const;

private:
    auto DoBindToFinished(FCk_Handle_VfxCue InVfxCueHandle) -> void;

    UFUNCTION()
    void OnEffectFinished(FCk_Handle_VfxCue InVfxCue);
};

// --------------------------------------------------------------------------------------------------------------------
