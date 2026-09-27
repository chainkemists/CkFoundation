#pragma once

#include "CkProceduralAnimation/CkProceduralAnimation_Fragment_Data.h"
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
    class FProcessor_ProceduralGait_Setup;
    class FProcessor_ProceduralGait_HandleRequests;
    class FProcessor_ProceduralGait_Update;
    struct FFragment_ProceduralGait_Tunables;
}

// --------------------------------------------------------------------------------------------------------------------

// Module-internal helpers shared by the gait's own utils, processors and the walker accelerant; only the support weight is
// exported, for its test.
namespace ck_procedural_gait_utils
{
    // A leg's reach: the sum of its chain's segment lengths.
    auto
        Get_Reach(
            const FCk_ProceduralLeg_ChainGeometry& InChain)
        -> float;

    // Gait admission requires every leg's rest foot within the step's TargetReachFraction of its reach from its hip;
    // otherwise the reach clamp would pull every step inward of the rest pose.
    auto
        Get_IsRestWithinReach(
            const FCk_ProceduralLeg_Placement& InPlacement,
            const FCk_ProceduralLeg_ChainGeometry& InChain,
            const FCk_ProceduralGait_Step& InStep)
        -> bool;

    // How much a published foot counts in the body pose's conform fit: a trusted plant fully, an untrusted plant (one that
    // touched down where its gait found no ground it trusts) not at all, and a swinging foot fading out over the first third
    // of its swing and, when its target is trusted, back in over the last third, so the fit does not step when the planted
    // set changes.
    CKPROCEDURALANIMATION_API auto
        Get_SupportWeight(
            const FCk_ProceduralLeg_Foot& InFoot)
        -> float;
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
    friend class ck::FProcessor_ProceduralGait_Setup;
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
              DisplayName="[Ck][ProceduralGait] Get Status")
    static ECk_ProceduralAnimation_Status
    Get_Status(
        const FCk_Handle_ProceduralGait& InGait);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|ProceduralGait",
              DisplayName="[Ck][ProceduralGait] Get Failure")
    static ECk_ProceduralGait_Failure
    Get_Failure(
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

    // The plane the last solve fitted through the supporting feet (a surface motion with the PlantedFeet height source rides
    // on it): Fitted, Held while too few feet support the body, None otherwise and unless the gait is Ready.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|ProceduralGait",
              DisplayName="[Ck][ProceduralGait] Get Feet Plane")
    static FCk_ProceduralGait_FeetPlane
    Get_FeetPlane(
        const FCk_Handle_ProceduralGait& InGait);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|ProceduralGait",
              DisplayName="[Ck][ProceduralGait] Request Apply Preset",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_ProceduralGait
    Request_ApplyPreset(
        UPARAM(ref) FCk_Handle_ProceduralGait& InGait,
        const FCk_Request_ProceduralGait_ApplyPreset& InRequest,
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
    // Also lowers the cadence speed reference to the reach floor of the legs enabled in InEnabledMask.
    static auto
    DoBuild_SolverSettings(
        const ck::FFragment_ProceduralGait_Tunables& InTunables,
        const TArray<FCk_Handle_ProceduralLeg>& InLegs,
        uint64 InEnabledMask)
        -> ck::FProceduralGaitBuiltSettings;

    static auto
    DoFind_LegBeyondReach(
        const TArray<FCk_Handle_ProceduralLeg>& InLegs,
        const FCk_ProceduralGait_Step& InStep)
        -> FCk_Handle_ProceduralLeg;
};

// --------------------------------------------------------------------------------------------------------------------
