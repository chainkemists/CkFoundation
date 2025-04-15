#pragma once

#include "CkRaySense/CkRaySense_Fragment_Data.h"

#include "CkECS/Handle/CkHandle.h"
#include "CkNet/CkNet_Utils.h"
#include "CkSignal/CkSignal_Fragment_Data.h"

#include "CkRaySense_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_RaySense_LineTrace_Update;
    class FProcessor_RaySense_BoxSweep_Update;
    class FProcessor_RaySense_SphereSweep_Update;
    class FProcessor_RaySense_CapsuleSweep_Update;
    class FProcessor_RaySense_CylinderSweep_Update;
}

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKRAYSENSE_API UCk_Utils_RaySense_UE : public UCk_Utils_Ecs_Net_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_RaySense_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_RaySense);

public:
    friend class ck::FProcessor_RaySense_LineTrace_Update;
    friend class ck::FProcessor_RaySense_BoxSweep_Update;
    friend class ck::FProcessor_RaySense_SphereSweep_Update;
    friend class ck::FProcessor_RaySense_CapsuleSweep_Update;
    friend class ck::FProcessor_RaySense_CylinderSweep_Update;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|RaySense",
              DisplayName="[Ck][RaySense] Add Feature")
    static FCk_Handle_RaySense
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_RaySense_ParamsData& InParams);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|RaySense",
              DisplayName="[Ck][RaySense] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|RaySense",
        DisplayName="[Ck][RaySense] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_RaySense
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|RaySense",
        DisplayName="[Ck][RaySense] Handle -> RaySense Handle",
        meta = (CompactNodeTitle = "<AsRaySense>", BlueprintAutocast))
    static FCk_Handle_RaySense
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid RaySense Handle",
        Category = "Ck|Utils|RaySense",
        meta = (CompactNodeTitle = "INVALID_RaySenseHandle", Keywords = "make"))
    static FCk_Handle_RaySense
    Get_InvalidHandle() { return {}; };

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|RaySense",
        DisplayName="[Ck][RaySense] Request Enable/Disable")
    static FCk_Handle_RaySense
    Request_EnableDisable(
        UPARAM(ref) FCk_Handle_RaySense& InHandle,
        const FCk_Request_RaySense_EnableDisable& InRequest);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|RaySense",
              DisplayName = "[Ck][RaySense] Bind To OnTraceHit")
    static FCk_Handle_RaySense
    BindTo_OnTraceHit(
        UPARAM(ref) FCk_Handle_RaySense& InHandle,
        ECk_Signal_BindingPolicy InBehavior,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_RaySense_LineTrace& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|RaySense",
              DisplayName = "[Ck][RaySense] Unbind From OnTraceHit")
    static FCk_Handle_RaySense
    UnbindFrom_OnTraceHit(
        UPARAM(ref) FCk_Handle_RaySense& InHandle,
        const FCk_Delegate_RaySense_LineTrace& InDelegate);

private:
    static auto
    DoGet_ShouldIgnoreTraceHit(
        FCk_Handle_RaySense& InHandle,
        const FHitResult& InTraceHit) -> bool;
};

// --------------------------------------------------------------------------------------------------------------------