#pragma once

#include "CkAggro/CkAggroTarget_Fragment.h"
#include "CkAggro/CkAggroTarget_Fragment_Data.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Signal/CkSignal_Fragment_Data.h"

#include "CkEcsExt/CkEcsExt_Utils.h"

#include "CkAggroTarget_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_AggroTarget"))
class CKAGGRO_API UCk_Utils_AggroTarget_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_AggroTarget_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_AggroTarget);

public:
    friend class UCk_Utils_Ecs_Base_UE;

public:
    // Direct-attach the (self-sufficient) AggroTarget feature onto InHandle — no owner, no record. Standalone threat
    // tracking. Tracks InParams' TrackedEntity, or InHandle itself when that is unset.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AggroTarget",
              DisplayName="[Ck][AggroTarget] Add Feature")
    static FCk_Handle_AggroTarget
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_AggroTarget_ParamsData& InParams);

    // Create an internal child target under InOwner (any entity): builds a new entity, Adds the feature to it, and
    // connects it to InOwner's AggroTargets record (adding the record to InOwner if missing).
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AggroTarget",
              DisplayName="[Ck][AggroTarget] Create")
    static FCk_Handle_AggroTarget
    Create(
        UPARAM(ref) FCk_Handle& InOwner,
        const FCk_Fragment_AggroTarget_ParamsData& InParams);

public:
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    // InOwner is the record/scoring owner, or an invalid handle for a standalone Add.
    static void
    DoAdd_Fragments(
        FCk_Handle& InHandle,
        const FCk_Fragment_AggroTarget_ParamsData& InParams,
        const FCk_Handle& InOwner);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|AggroTarget",
        DisplayName="[Ck][AggroTarget] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_AggroTarget
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|AggroTarget",
        DisplayName="[Ck][AggroTarget] Handle -> Aggro Target Handle",
        meta = (CompactNodeTitle = "<AsAggroTarget>", BlueprintAutocast))
    static FCk_Handle_AggroTarget
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid Aggro Target Handle",
        Category = "Ck|Utils|AggroTarget",
        meta = (CompactNodeTitle = "INVALID_AggroTargetHandle", Keywords = "make"))
    static FCk_Handle_AggroTarget
    Get_InvalidHandle() { return {}; }

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AggroTarget",
              DisplayName="[Ck][AggroTarget] Get Tracked Entity")
    static FCk_Handle
    Get_TrackedEntity(
        const FCk_Handle_AggroTarget& InTarget);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AggroTarget",
              DisplayName="[Ck][AggroTarget] Get Instigator")
    static FCk_Handle
    Get_Instigator(
        const FCk_Handle_AggroTarget& InTarget);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AggroTarget",
              DisplayName="[Ck][AggroTarget] Get Source")
    static FCk_Handle
    Get_Source(
        const FCk_Handle_AggroTarget& InTarget);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AggroTarget",
              DisplayName="[Ck][AggroTarget] Get Aggro Owner")
    static FCk_Handle
    Get_AggroOwner(
        const FCk_Handle_AggroTarget& InTarget);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AggroTarget",
              DisplayName="[Ck][AggroTarget] Get Threat")
    static float
    Get_Threat(
        const FCk_Handle_AggroTarget& InTarget);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AggroTarget",
              DisplayName="[Ck][AggroTarget] Get Score")
    static float
    Get_Score(
        const FCk_Handle_AggroTarget& InTarget);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AggroTarget",
              DisplayName="[Ck][AggroTarget] Get Distance")
    static float
    Get_Distance(
        const FCk_Handle_AggroTarget& InTarget);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AggroTarget",
              DisplayName="[Ck][AggroTarget] Get Last Known Location")
    static FVector
    Get_LastKnownLocation(
        const FCk_Handle_AggroTarget& InTarget);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AggroTarget",
              DisplayName="[Ck][AggroTarget] Get Is Perceived")
    static bool
    Get_IsPerceived(
        const FCk_Handle_AggroTarget& InTarget);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AggroTarget",
              DisplayName="[Ck][AggroTarget] Get Is Active Target")
    static bool
    Get_IsActiveTarget(
        const FCk_Handle_AggroTarget& InTarget);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AggroTarget",
              DisplayName="[Ck][AggroTarget] Get Is Within Retention")
    static bool
    Get_IsWithinRetention(
        const FCk_Handle_AggroTarget& InTarget);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AggroTarget",
              DisplayName="[Ck][AggroTarget] Get Age")
    static FCk_Time
    Get_Age(
        const FCk_Handle_AggroTarget& InTarget);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AggroTarget",
              DisplayName="[Ck][AggroTarget] Get Time Since Last Threat")
    static FCk_Time
    Get_TimeSinceLastThreat(
        const FCk_Handle_AggroTarget& InTarget);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AggroTarget",
              DisplayName="[Ck][AggroTarget] Request Add Threat",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_AggroTarget
    Request_AddThreat(
        UPARAM(ref) FCk_Handle_AggroTarget& InTarget,
        float InThreatDelta,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AggroTarget",
              DisplayName="[Ck][AggroTarget] Request Set Threat",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_AggroTarget
    Request_SetThreat(
        UPARAM(ref) FCk_Handle_AggroTarget& InTarget,
        float InThreat,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AggroTarget",
              DisplayName="[Ck][AggroTarget] Request Mark Perceived",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_AggroTarget
    Request_MarkPerceived(
        UPARAM(ref) FCk_Handle_AggroTarget& InTarget,
        FCk_Request_AggroTarget_MarkPerceived InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AggroTarget",
              DisplayName="[Ck][AggroTarget] Request Mark Unperceived",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_AggroTarget
    Request_MarkUnperceived(
        UPARAM(ref) FCk_Handle_AggroTarget& InTarget,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AggroTarget",
              DisplayName="[Ck][AggroTarget] Request Reset Perception",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_AggroTarget
    Request_ResetPerception(
        UPARAM(ref) FCk_Handle_AggroTarget& InTarget,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AggroTarget",
              DisplayName="[Ck][AggroTarget] Request Forget",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_AggroTarget
    Request_Forget(
        UPARAM(ref) FCk_Handle_AggroTarget& InTarget,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AggroTarget",
              DisplayName = "[Ck][AggroTarget] Bind To OnThreatChanged")
    static FCk_Handle_AggroTarget
    BindTo_OnThreatChanged(
        UPARAM(ref) FCk_Handle_AggroTarget& InTarget,
        const FCk_Delegate_AggroTarget_OnThreatChanged& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AggroTarget",
              DisplayName = "[Ck][AggroTarget] Unbind From OnThreatChanged")
    static FCk_Handle_AggroTarget
    UnbindFrom_OnThreatChanged(
        UPARAM(ref) FCk_Handle_AggroTarget& InTarget,
        const FCk_Delegate_AggroTarget_OnThreatChanged& InDelegate);
};

// --------------------------------------------------------------------------------------------------------------------
