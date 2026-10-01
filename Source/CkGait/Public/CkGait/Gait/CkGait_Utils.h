#pragma once

#include "CkGait/Gait/CkGait_Fragment.h"
#include "CkEcsExt/CkEcsExt_Utils.h"
#include "CkEcs/Request/CkRequest_Completion.h"

#include "CkGait_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_Gait"))
class CKGAIT_API UCk_Utils_Gait_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Gait_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_Gait);

    friend class UCk_Utils_Ecs_Base_UE;

public:
    // The locomotion rhythm of InHandle, which may be any entity. The spec's movement component must be valid; it is
    // sampled every frame from here on, and Request_UpdateSpec re-points it.
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Gait", DisplayName = "[Ck][Gait] Add Feature")
    static FCk_Handle_Gait
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Gait_Spec& InSpec);

    static auto Has(const FCk_Handle& InHandle) -> bool;

private:
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Gait", DisplayName = "[Ck][Gait] DoCast", meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_Gait
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Gait", DisplayName = "[Ck][Gait] DoCastChecked", meta = (CompactNodeTitle = "<AsGait>", BlueprintAutocast))
    static FCk_Handle_Gait
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Gait", DisplayName = "[Ck] Get Invalid Gait Handle",
        meta = (CompactNodeTitle = "INVALID_GaitHandle", Keywords = "make"))
    static FCk_Handle_Gait
    Get_InvalidHandle() { return {}; }

public:
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Gait", DisplayName = "[Ck][Gait] Get Spec")
    static FCk_Gait_Spec
    Get_Spec(
        const FCk_Handle_Gait& InGait);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Gait", DisplayName = "[Ck][Gait] Get Is Enabled")
    static bool
    Get_IsEnabled(
        const FCk_Handle_Gait& InGait);

    // Stride phase, radians [0, 2pi). A footfall every pi; sin(phase) is the left/right swing.
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Gait", DisplayName = "[Ck][Gait] Get Phase")
    static float
    Get_Phase(
        const FCk_Handle_Gait& InGait);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Gait", DisplayName = "[Ck][Gait] Get Amount")
    static float
    Get_Amount(
        const FCk_Handle_Gait& InGait);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Gait", DisplayName = "[Ck][Gait] Get Speed Ratio")
    static float
    Get_SpeedRatio(
        const FCk_Handle_Gait& InGait);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Gait", DisplayName = "[Ck][Gait] Get Breath Phase")
    static float
    Get_BreathPhase(
        const FCk_Handle_Gait& InGait);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Gait", DisplayName = "[Ck][Gait] Get Last Motion")
    static FCk_Gait_Motion
    Get_LastMotion(
        const FCk_Handle_Gait& InGait);

    // Monotonic count of airborne->grounded edges since Add. Consumers diff it against their own copy.
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Gait", DisplayName = "[Ck][Gait] Get Landing Count")
    static int32
    Get_LandingCount(
        const FCk_Handle_Gait& InGait);

    // Downward speed (cm/s, >= 0) just before the most recent landing.
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Gait", DisplayName = "[Ck][Gait] Get Last Land Impact Speed")
    static float
    Get_LastLandImpactSpeed(
        const FCk_Handle_Gait& InGait);

public:
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Gait", DisplayName = "[Ck][Gait] Request Update Spec", meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_Gait
    Request_UpdateSpec(
        UPARAM(ref) FCk_Handle_Gait& InGait,
        const FCk_Request_Gait_UpdateSpec& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Gait", DisplayName = "[Ck][Gait] Request Enable/Disable", meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_Gait
    Request_EnableDisable(
        UPARAM(ref) FCk_Handle_Gait& InGait,
        const FCk_Request_Gait_EnableDisable& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Gait", DisplayName = "[Ck][Gait] Request Reset", meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_Gait
    Request_Reset(
        UPARAM(ref) FCk_Handle_Gait& InGait,
        const FCk_Request_Gait_Reset& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);
};

// --------------------------------------------------------------------------------------------------------------------
