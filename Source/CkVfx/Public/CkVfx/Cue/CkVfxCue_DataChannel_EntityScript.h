#pragma once

#include "CkVfxCue_DataChannel_Fragment_Data.h"

#include "CkVfx/Cue/CkVfxCue_EntityScript.h"

#include <NiagaraDataChannel.h>
#include <NiagaraDataChannelAccessor.h>

#include "CkVfxCue_DataChannel_EntityScript.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(BlueprintType, Blueprintable)
class CKVFX_API UCk_VfxCue_DataChannel_EntityScript : public UCk_VfxCueBase_EntityScript
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_VfxCue_DataChannel_EntityScript);

protected:
    UCk_VfxCue_DataChannel_EntityScript(const FObjectInitializer& InObjectInitializer);

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
    TArray<FCk_VfxCue_DataChannel_SpawnParams> _BatchData;

public:
    CK_PROPERTY_GET(_DataChannel);
    CK_PROPERTY_GET(_VisibleToGame);
    CK_PROPERTY_GET(_VisibleToNiagaraCPU);
    CK_PROPERTY_GET(_VisibleToNiagaraGPU);
    CK_PROPERTY_GET(_BatchData);

protected:
    auto BeginPlay() -> void override;
    auto Restart() -> void override;

    FGameplayTag Get_CueName_Implementation() const override;

public:
    UFUNCTION(BlueprintPure, Category = "VFX Cue | Data Channel")
    bool Get_IsConfigurationValid() const;

private:
    auto WriteBatchDataToChannel() -> void;

    auto WriteParameterToChannel(
        UNiagaraDataChannelWriter* InWriter,
        int32 InIndex,
        const FCk_VfxCue_UserParameter& InParameter) -> void;
};

// --------------------------------------------------------------------------------------------------------------------