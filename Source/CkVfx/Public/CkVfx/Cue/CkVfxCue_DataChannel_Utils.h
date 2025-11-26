#pragma once

#include "CkVfxCue_DataChannel_Fragment_Data.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkVfxCue_DataChannel_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKVFX_API UCk_Utils_VfxCue_DataChannel_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_VfxCue_DataChannel_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_VfxCue_DataChannel);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VfxCue|DataChannel",
              DisplayName="[Ck][VfxCue DataChannel] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|VfxCue|DataChannel",
        DisplayName="[Ck][VfxCue DataChannel] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_VfxCue_DataChannel
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|VfxCue|DataChannel",
        DisplayName="[Ck][VfxCue DataChannel] Handle -> VfxCue DataChannel Handle",
        meta = (CompactNodeTitle = "<AsVfxCueDataChannel>", BlueprintAutocast))
    static FCk_Handle_VfxCue_DataChannel
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid VfxCue DataChannel Handle",
        Category = "Ck|Utils|VfxCue|DataChannel",
        meta = (CompactNodeTitle = "INVALID_VfxCueDataChannelHandle", Keywords = "make"))
    static FCk_Handle_VfxCue_DataChannel
    Get_InvalidHandle() { return {}; };
};

// --------------------------------------------------------------------------------------------------------------------
