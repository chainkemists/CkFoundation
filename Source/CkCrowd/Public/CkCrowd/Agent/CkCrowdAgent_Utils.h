#pragma once

#include "CkCrowd/Agent/CkCrowdAgent_Fragment.h"
#include "CkCrowd/Agent/CkCrowdAgent_Fragment_Data.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcsExt/CkEcsExt_Utils.h"

#include "CkCrowdAgent_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_CrowdAgent"))
class CKCROWD_API UCk_Utils_CrowdAgent_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_CrowdAgent_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_CrowdAgent);

public:
    // Add the crowd-agent feature to InOwner. Creates a new child entity carrying the
    // CrowdAgent fragments + FTag_CrowdAgent_NeedsSetup; returns the typesafe handle.
    // Gate 0: structural setup only. Gate 2+ adds path / steering / velocity-bridge fragments
    // through the same Add() call.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|CrowdAgent",
              DisplayName="[Ck][CrowdAgent] Add Feature")
    static FCk_Handle_CrowdAgent
    Add(
        UPARAM(ref) FCk_Handle& InOwner,
        const FCk_Fragment_CrowdAgent_ParamsData& InParams);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|CrowdAgent",
              DisplayName="[Ck][CrowdAgent] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

    // Read the steering processor's per-frame output. Sub-task 2C will copy this into
    // FFragment_Velocity_Current via the velocity-bridge processor; until then this getter is the
    // primary way for tests/diagnostics to observe what the steering layer is producing.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|CrowdAgent",
              DisplayName="[Ck][CrowdAgent] Get Desired Velocity")
    static FVector
    Get_DesiredVelocity(
        const FCk_Handle_CrowdAgent& InHandle);

    // Read the face-angle processor's current target yaw in DEGREES (converted from the radians
    // stored on the fragment for caller convenience). The agent's actual yaw — lerped toward this
    // target at _MaxTurnRate — is on its Transform's SceneNode rotation.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|CrowdAgent",
              DisplayName="[Ck][CrowdAgent] Get Target Yaw (degrees)")
    static float
    Get_TargetYawDegrees(
        const FCk_Handle_CrowdAgent& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|CrowdAgent",
        DisplayName="[Ck][CrowdAgent] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_CrowdAgent
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|CrowdAgent",
        DisplayName="[Ck][CrowdAgent] Handle -> CrowdAgent Handle",
        meta = (CompactNodeTitle = "<AsCrowdAgent>", BlueprintAutocast))
    static FCk_Handle_CrowdAgent
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid CrowdAgent Handle",
        Category = "Ck|Utils|CrowdAgent",
        meta = (CompactNodeTitle = "INVALID_CrowdAgentHandle", Keywords = "make"))
    static FCk_Handle_CrowdAgent
    Get_InvalidHandle() { return {}; };
};

// --------------------------------------------------------------------------------------------------------------------
