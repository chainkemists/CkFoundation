#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcsExt/CkEcsExt_Utils.h"

#include "CkJolt/Body/CkJoltBody_Fragment.h"
#include "CkJolt/Body/CkJoltBody_Fragment_Data.h"

#include "CkJoltBody_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_JoltBody"))
class CKJOLT_API UCk_Utils_JoltBody_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_JoltBody_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_JoltBody);

public:
    friend class UCk_Utils_Ecs_Base_UE;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName="[Ck][JoltBody] Add New JoltBody")
    static FCk_Handle_JoltBody
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_JoltBody_ParamsData& InParams);

public:
    // Has Feature
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|JoltBody",
        DisplayName="[Ck][JoltBody] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_JoltBody
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|JoltBody",
        DisplayName="[Ck][JoltBody] Handle -> JoltBody Handle",
        meta = (CompactNodeTitle = "<AsJoltBody>", BlueprintAutocast))
    static FCk_Handle_JoltBody
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid JoltBody Handle",
        Category = "Ck|Utils|JoltBody",
        meta = (CompactNodeTitle = "INVALID_JoltBodyHandle", Keywords = "make"))
    static FCk_Handle_JoltBody
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|JoltBody",
              DisplayName="[Ck][JoltBody] Get Motion Type")
    static ECk_MotionType
    Get_MotionType(
        const FCk_Handle_JoltBody& InJoltBody);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|JoltBody",
              DisplayName="[Ck][JoltBody] Get Sleep State")
    static ECk_Jolt_SleepState
    Get_SleepState(
        const FCk_Handle_JoltBody& InJoltBody);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|JoltBody",
              DisplayName="[Ck][JoltBody] Get Is Body Added")
    static bool
    Get_IsBodyAdded(
        const FCk_Handle_JoltBody& InJoltBody);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|JoltBody",
              DisplayName="[Ck][JoltBody] Request Set Sleep State")
    static FCk_Handle_JoltBody
    Request_SetSleepState(
        UPARAM(ref) FCk_Handle_JoltBody& InJoltBody,
        const FCk_Request_JoltBody_SetSleepState& InRequest);
};

// --------------------------------------------------------------------------------------------------------------------
