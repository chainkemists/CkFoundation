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

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|ProceduralRig",
              DisplayName="[Ck][ProceduralRig] Get Clearance")
    static ECk_ProceduralRig_Clearance
    Get_Clearance(
        const FCk_Handle_ProceduralRig& InRig);

    // Crossing when the last pose kept a link through a solid because no fan angle cleared the links and the body slab. When
    // nothing clears, the authored pole's pose is judged by its rays alone, so a joint of it inside the slab still reads Clear.
    // Always Clear for a rig without a clearance policy, which never tests its links.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|ProceduralRig",
              DisplayName="[Ck][ProceduralRig] Get Chain State")
    static ECk_ProceduralRig_ChainState
    Get_ChainState(
        const FCk_Handle_ProceduralRig& InRig);

    // The swivel of the last fully clear pose about the hip-foot line, in degrees; 0 when the authored pole is clear, when
    // no angle was, or without a clearance policy.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|ProceduralRig",
              DisplayName="[Ck][ProceduralRig] Get Swivel Degrees")
    static float
    Get_SwivelDegrees(
        const FCk_Handle_ProceduralRig& InRig);
};

// --------------------------------------------------------------------------------------------------------------------
