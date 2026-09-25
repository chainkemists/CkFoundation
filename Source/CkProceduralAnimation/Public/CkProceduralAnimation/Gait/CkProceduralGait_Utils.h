#pragma once

#include "CkProceduralAnimation/Gait/CkProceduralGait_Fragment_Data.h"
#include "CkProceduralAnimation/Leg/CkProceduralLeg_Fragment_Data.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Signal/CkSignal_Fragment_Data.h"

#include "CkEcsExt/CkEcsExt_Utils.h"
#include "CkEcsExt/Transform/CkTransform_Fragment_Data.h"

#include "CkProceduralGait_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_ProceduralGait_HandleRequests;
    class FProcessor_ProceduralGait_Update;
    struct FFragment_ProceduralGait_Tunables;
    struct FProceduralGaitSettings;
}

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_ProceduralGait"))
class CKPROCEDURALANIMATION_API UCk_Utils_ProceduralGait_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_ProceduralGait_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_ProceduralGait);

public:
    friend class UCk_Utils_Ecs_Base_UE;
    friend class ck::FProcessor_ProceduralGait_HandleRequests;
    friend class ck::FProcessor_ProceduralGait_Update;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|ProceduralGait",
              DisplayName="[Ck][ProceduralGait] Add")
    static FCk_Handle_ProceduralGait
    Add(
        UPARAM(ref) FCk_Handle_Transform& InBody,
        const UCk_ProceduralGait_Data* InData);

public:
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|ProceduralGait",
        DisplayName="[Ck][ProceduralGait] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_ProceduralGait
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|ProceduralGait",
        DisplayName="[Ck][ProceduralGait] Handle -> ProceduralGait Handle",
        meta = (CompactNodeTitle = "<AsProceduralGait>", BlueprintAutocast))
    static FCk_Handle_ProceduralGait
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName="[Ck] Get Invalid ProceduralGait Handle",
        Category = "Ck|Utils|ProceduralGait",
        meta = (CompactNodeTitle = "INVALID_ProceduralGaitHandle", Keywords = "make"))
    static FCk_Handle_ProceduralGait
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|ProceduralGait",
              DisplayName="[Ck][ProceduralGait] Get Is Ready")
    static bool
    Get_IsReady(
        const FCk_Handle_ProceduralGait& InGait);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|ProceduralGait",
              DisplayName="[Ck][ProceduralGait] Get Has Failed")
    static bool
    Get_HasFailed(
        const FCk_Handle_ProceduralGait& InGait);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|ProceduralGait",
              DisplayName="[Ck][ProceduralGait] Get Gait Clock")
    static float
    Get_GaitClock(
        const FCk_Handle_ProceduralGait& InGait);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|ProceduralGait",
              DisplayName="[Ck][ProceduralGait] Get Trusted Contact Count")
    static int32
    Get_TrustedContactCount(
        const FCk_Handle_ProceduralGait& InGait);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|ProceduralGait",
              DisplayName="[Ck][ProceduralGait] Get Planted Count")
    static int32
    Get_PlantedCount(
        const FCk_Handle_ProceduralGait& InGait);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|ProceduralGait",
              DisplayName="[Ck][ProceduralGait] Get Legs")
    static TArray<FCk_Handle_ProceduralLeg>
    Get_Legs(
        const FCk_Handle_ProceduralGait& InGait);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|ProceduralGait",
              DisplayName="[Ck][ProceduralGait] Get Enabled Leg Count")
    static int32
    Get_EnabledLegCount(
        const FCk_Handle_ProceduralGait& InGait);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|ProceduralGait",
              DisplayName="[Ck][ProceduralGait] Request Apply Preset",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_ProceduralGait
    Request_ApplyPreset(
        UPARAM(ref) FCk_Handle_ProceduralGait& InGait,
        const UCk_ProceduralGait_Data* InData,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|ProceduralGait",
              DisplayName="[Ck][ProceduralGait] Bind To OnLegSetChanged")
    static FCk_Handle_ProceduralGait
    BindTo_OnLegSetChanged(
        UPARAM(ref) FCk_Handle_ProceduralGait& InGait,
        const FCk_Delegate_ProceduralGait_OnLegSetChanged& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|ProceduralGait",
              DisplayName="[Ck][ProceduralGait] Unbind From OnLegSetChanged")
    static FCk_Handle_ProceduralGait
    UnbindFrom_OnLegSetChanged(
        UPARAM(ref) FCk_Handle_ProceduralGait& InGait,
        const FCk_Delegate_ProceduralGait_OnLegSetChanged& InDelegate);

private:
    static auto
    DoBuild_SolverSettings(
        const ck::FFragment_ProceduralGait_Tunables& InTunables,
        int32 InEnabledCount)
        -> ck::FProceduralGaitSettings;
};

// --------------------------------------------------------------------------------------------------------------------
