#pragma once

#include "CkVfx/Cue/CkVfxCue_EntityScript.h"

#include <NiagaraDataChannel.h>
#include <NiagaraDataChannelAccessor.h>

#include "CkVfxCue_DataChannel_Scripted_EntityScript.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(BlueprintType, Blueprintable)
class CKVFX_API UCk_VfxCue_DataChannel_Scripted_EntityScript : public UCk_VfxCueBase_EntityScript
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_VfxCue_DataChannel_Scripted_EntityScript);

protected:
    UCk_VfxCue_DataChannel_Scripted_EntityScript(const FObjectInitializer& InObjectInitializer);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data Channel",
              meta = (AllowPrivateAccess = true))
    TObjectPtr<UNiagaraDataChannelAsset> _DataChannel;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data Channel",
              meta = (AllowPrivateAccess = true))
    bool _VisibleToGame = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data Channel",
              meta = (AllowPrivateAccess = true))
    bool _VisibleToNiagaraCPU = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data Channel",
              meta = (AllowPrivateAccess = true))
    bool _VisibleToNiagaraGPU = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data Channel",
              meta = (AllowPrivateAccess = true, ExposeOnSpawn = true))
    FVector _Location = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data Channel",
              meta = (AllowPrivateAccess = true, ExposeOnSpawn = true))
    int32 _BatchCount = 1;

public:
    CK_PROPERTY_GET(_DataChannel);
    CK_PROPERTY_GET(_VisibleToGame);
    CK_PROPERTY_GET(_VisibleToNiagaraCPU);
    CK_PROPERTY_GET(_VisibleToNiagaraGPU);
    CK_PROPERTY_GET(_Location);
    CK_PROPERTY_GET(_BatchCount);

protected:
    auto BeginPlay() -> void override;

    FGameplayTag Get_CueName_Implementation() const override;

    UFUNCTION(BlueprintNativeEvent, Category = "VFX Cue | Data Channel")
    void WriteDataChannelParameters(
        UNiagaraDataChannelWriter* InWriter,
        int32 InBatchCount);

public:
    UFUNCTION(BlueprintPure, Category = "VFX Cue | Data Channel")
    bool Get_IsConfigurationValid() const;
};

// --------------------------------------------------------------------------------------------------------------------
