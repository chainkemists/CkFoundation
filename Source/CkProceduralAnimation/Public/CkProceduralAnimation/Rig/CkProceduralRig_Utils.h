#pragma once

#include "CkProceduralAnimation/CkProceduralAnimation_Fragment_Data.h"
#include "CkProceduralAnimation/Leg/CkProceduralLeg_Fragment_Data.h"
#include "CkProceduralAnimation/Rig/CkProceduralRig_Fragment_Data.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkEcsExt/CkEcsExt_Utils.h"

#include "CkProceduralRig_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_ProceduralRig"))
class CKPROCEDURALANIMATION_API UCk_Utils_ProceduralRig_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_ProceduralRig_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_ProceduralRig);

public:
    friend class UCk_Utils_Ecs_Base_UE;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|ProceduralRig",
              DisplayName="[Ck][ProceduralRig] Add")
    static FCk_Handle_ProceduralRig
    Add(
        UPARAM(ref) FCk_Handle_ProceduralLeg& InLeg,
        const FCk_ProceduralRig_Spec& InParams);

public:
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|ProceduralRig",
        DisplayName="[Ck][ProceduralRig] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_ProceduralRig
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|ProceduralRig",
        DisplayName="[Ck][ProceduralRig] Handle -> ProceduralRig Handle",
        meta = (CompactNodeTitle = "<AsProceduralRig>", BlueprintAutocast))
    static FCk_Handle_ProceduralRig
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName="[Ck] Get Invalid ProceduralRig Handle",
        Category = "Ck|Utils|ProceduralRig",
        meta = (CompactNodeTitle = "INVALID_ProceduralRigHandle", Keywords = "make"))
    static FCk_Handle_ProceduralRig
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|ProceduralRig",
              DisplayName="[Ck][ProceduralRig] Get Status")
    static ECk_ProceduralAnimation_Status
    Get_Status(
        const FCk_Handle_ProceduralRig& InRig);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|ProceduralRig",
              DisplayName="[Ck][ProceduralRig] Get Failure")
    static ECk_ProceduralRig_Failure
    Get_Failure(
        const FCk_Handle_ProceduralRig& InRig);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|ProceduralRig",
              DisplayName="[Ck][ProceduralRig] Get Chain")
    static FCk_ProceduralRig_Spec
    Get_Chain(
        const FCk_Handle_ProceduralRig& InRig);
};

// --------------------------------------------------------------------------------------------------------------------
