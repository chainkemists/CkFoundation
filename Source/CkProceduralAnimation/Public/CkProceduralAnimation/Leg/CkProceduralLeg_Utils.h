#pragma once

#include "CkProceduralAnimation/Leg/CkProceduralLeg_Fragment_Data.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Signal/CkSignal_Fragment_Data.h"

#include "CkEcsExt/CkEcsExt_Utils.h"
#include "CkEcsExt/Transform/CkTransform_Fragment_Data.h"

#include "CkProceduralLeg_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_ProceduralLeg"))
class CKPROCEDURALANIMATION_API UCk_Utils_ProceduralLeg_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_ProceduralLeg_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_ProceduralLeg);

public:
    friend class UCk_Utils_Ecs_Base_UE;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|ProceduralLeg",
              DisplayName="[Ck][ProceduralLeg] Create")
    static FCk_Handle_ProceduralLeg
    Create(
        UPARAM(ref) FCk_Handle_Transform& InBody,
        const FCk_ProceduralLeg_Spec& InParams);

public:
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|ProceduralLeg",
        DisplayName="[Ck][ProceduralLeg] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_ProceduralLeg
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|ProceduralLeg",
        DisplayName="[Ck][ProceduralLeg] Handle -> ProceduralLeg Handle",
        meta = (CompactNodeTitle = "<AsProceduralLeg>", BlueprintAutocast))
    static FCk_Handle_ProceduralLeg
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName="[Ck] Get Invalid ProceduralLeg Handle",
        Category = "Ck|Utils|ProceduralLeg",
        meta = (CompactNodeTitle = "INVALID_ProceduralLegHandle", Keywords = "make"))
    static FCk_Handle_ProceduralLeg
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|ProceduralLeg",
              DisplayName="[Ck][ProceduralLeg] Get Id")
    static FName
    Get_Id(
        const FCk_Handle_ProceduralLeg& InLeg);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|ProceduralLeg",
              DisplayName="[Ck][ProceduralLeg] Get Placement")
    static FCk_ProceduralLeg_Placement
    Get_Placement(
        const FCk_Handle_ProceduralLeg& InLeg);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|ProceduralLeg",
              DisplayName="[Ck][ProceduralLeg] Get Chain Geometry")
    static FCk_ProceduralLeg_ChainGeometry
    Get_ChainGeometry(
        const FCk_Handle_ProceduralLeg& InLeg);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|ProceduralLeg",
              DisplayName="[Ck][ProceduralLeg] Get Foot")
    static FCk_ProceduralLeg_Foot
    Get_Foot(
        const FCk_Handle_ProceduralLeg& InLeg);

    // The verdict of the ideal target on the last solve that probed the leg: Usable when the ideal was the target or a hold
    // agreed with it, otherwise why the gait looked elsewhere. A disabled leg keeps the verdict of its last enabled solve.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|ProceduralLeg",
              DisplayName="[Ck][ProceduralLeg] Get Ideal Verdict")
    static ECk_ProceduralLeg_FootholdVerdict
    Get_IdealVerdict(
        const FCk_Handle_ProceduralLeg& InLeg);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|ProceduralLeg",
              DisplayName="[Ck][ProceduralLeg] Get Enable Disable")
    static ECk_EnableDisable
    Get_EnableDisable(
        const FCk_Handle_ProceduralLeg& InLeg);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|ProceduralLeg",
              DisplayName="[Ck][ProceduralLeg] Request Enable/Disable",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_ProceduralLeg
    Request_EnableDisable(
        UPARAM(ref) FCk_Handle_ProceduralLeg& InLeg,
        const FCk_Request_ProceduralLeg_EnableDisable& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|ProceduralLeg",
              DisplayName="[Ck][ProceduralLeg] Request Detach",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_ProceduralLeg
    Request_Detach(
        UPARAM(ref) FCk_Handle_ProceduralLeg& InLeg,
        const FCk_Request_ProceduralLeg_Detach& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|ProceduralLeg",
              DisplayName="[Ck][ProceduralLeg] Bind To OnDetached")
    static FCk_Handle_ProceduralLeg
    BindTo_OnDetached(
        UPARAM(ref) FCk_Handle_ProceduralLeg& InLeg,
        const FCk_Delegate_ProceduralLeg_OnDetached& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|ProceduralLeg",
              DisplayName="[Ck][ProceduralLeg] Unbind From OnDetached")
    static FCk_Handle_ProceduralLeg
    UnbindFrom_OnDetached(
        UPARAM(ref) FCk_Handle_ProceduralLeg& InLeg,
        const FCk_Delegate_ProceduralLeg_OnDetached& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|ProceduralLeg",
              DisplayName="[Ck][ProceduralLeg] Bind To OnPlanted")
    static FCk_Handle_ProceduralLeg
    BindTo_OnPlanted(
        UPARAM(ref) FCk_Handle_ProceduralLeg& InLeg,
        const FCk_Delegate_ProceduralLeg_OnPlanted& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|ProceduralLeg",
              DisplayName="[Ck][ProceduralLeg] Unbind From OnPlanted")
    static FCk_Handle_ProceduralLeg
    UnbindFrom_OnPlanted(
        UPARAM(ref) FCk_Handle_ProceduralLeg& InLeg,
        const FCk_Delegate_ProceduralLeg_OnPlanted& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|ProceduralLeg",
              DisplayName="[Ck][ProceduralLeg] Bind To OnLifted")
    static FCk_Handle_ProceduralLeg
    BindTo_OnLifted(
        UPARAM(ref) FCk_Handle_ProceduralLeg& InLeg,
        const FCk_Delegate_ProceduralLeg_OnLifted& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|ProceduralLeg",
              DisplayName="[Ck][ProceduralLeg] Unbind From OnLifted")
    static FCk_Handle_ProceduralLeg
    UnbindFrom_OnLifted(
        UPARAM(ref) FCk_Handle_ProceduralLeg& InLeg,
        const FCk_Delegate_ProceduralLeg_OnLifted& InDelegate);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|ProceduralLeg",
              DisplayName="[Ck][ProceduralLeg] Get Legs")
    static TArray<FCk_Handle_ProceduralLeg>
    Get_Legs(
        const FCk_Handle& InBody);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|ProceduralLeg",
              DisplayName="[Ck][ProceduralLeg] Try Get Leg")
    static FCk_Handle_ProceduralLeg
    TryGet_Leg(
        const FCk_Handle& InBody,
        FName InId);
};

// --------------------------------------------------------------------------------------------------------------------
