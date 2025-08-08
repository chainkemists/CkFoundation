#pragma once

#include "CkAudio/CkAudio_Fragment_Data.h"

#include "CkECS/Handle/CkHandle.h"
#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Signal/CkSignal_Fragment_Data.h"

#include "CkAudio_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_Audio"))
class CKAUDIO_API UCk_Utils_Audio_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Audio_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_Audio);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Audio",
              DisplayName="[Ck][Audio] Add Feature")
    static FCk_Handle_Audio
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_Audio_ParamsData& InParams,
        ECk_Replication InReplicates = ECk_Replication::Replicates);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Audio",
              DisplayName="[Ck][Audio] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Audio",
        DisplayName="[Ck][Audio] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_Audio
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Audio",
        DisplayName="[Ck][Audio] Handle -> Audio Handle",
        meta = (CompactNodeTitle = "<AsAudio>", BlueprintAutocast))
    static FCk_Handle_Audio
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid Audio Handle",
        Category = "Ck|Utils|Audio",
        meta = (CompactNodeTitle = "INVALID_AudioHandle", Keywords = "make"))
    static FCk_Handle_Audio
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Audio",
        DisplayName="[Ck][Audio] Request ExampleRequest")
    static FCk_Handle_Audio
    Request_ExampleRequest(
        UPARAM(ref) FCk_Handle_Audio& InAudio,
        const FCk_Request_Audio_ExampleRequest& InRequest);
};

// --------------------------------------------------------------------------------------------------------------------