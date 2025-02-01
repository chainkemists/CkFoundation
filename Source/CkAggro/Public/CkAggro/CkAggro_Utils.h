#pragma once

#include "CkAggro/CkAggro_Fragment.h"
#include "CkAggro/CkAggro_Fragment_Data.h"

#include "CkECS/Handle/CkHandle.h"

#include "CkEcs/Handle/CkHandle_TypeSafe.h"

#include "CkEcsExt/CkEcsExt_Utils.h"

#include "CkNet/CkNet_Utils.h"

#include "CkAggro_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

struct FCk_Handle_AggroOwner;

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKAGGRO_API UCk_Utils_Aggro_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Aggro_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_Aggro);

    using RecordOfAggro_Utils = ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfAggro>;

public:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Aggro",
        DisplayName="[Ck][Aggro] Add New Aggro")
    static FCk_Handle_Aggro
    Add(
        UPARAM(ref) FCk_Handle_AggroOwner& InHandle,
        const FCk_Handle& InTarget,
        const FCk_Fragment_Aggro_Params& InParams);

public:
    // Has Feature
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Aggro",
        DisplayName="[Ck][Aggro] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_Aggro
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Aggro",
        DisplayName="[Ck][Aggro] Handle -> Aggro Handle",
        meta = (CompactNodeTitle = "<AsAggro>", BlueprintAutocast))
    static FCk_Handle_Aggro
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid Aggro Handle",
        Category = "Ck|Utils|Aggro",
        meta = (CompactNodeTitle = "INVALID_AggroHandle", Keywords = "make"))
    static FCk_Handle_Aggro
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Aggro",
              DisplayName="[Ck][Aggro] Get Aggro Target")
    static FCk_Handle
    Get_AggroTarget(
        const FCk_Handle_Aggro& InAggro);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Aggro",
              DisplayName="[Ck][Aggro] Get Aggro Score")
    static float
    Get_AggroScore(
        const FCk_Handle_Aggro& InAggro);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Aggro",
              DisplayName="[Ck][Aggro] Request Exclude")
    static FCk_Handle_Aggro
    Request_Exclude(
        UPARAM(ref) FCk_Handle_Aggro& InAggro);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Aggro",
              DisplayName="[Ck][Aggro] Request Include")
    static FCk_Handle_Aggro
    Request_Include(
        UPARAM(ref) FCk_Handle_Aggro& InAggro);
};

// --------------------------------------------------------------------------------------------------------------------
