#pragma once

#include "CkCrowd/AvoidanceVolume/CkCrowdAvoidanceVolume_Fragment_Data.h"
#include "CkNavigation/Nav/CkNav_Fragment_Data.h"
#include "CkEcsExt/Transform/CkTransform_Fragment_Data.h"

#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkCrowdAvoidanceVolume_Utils.generated.h"

enum class ECk_CrowdAvoidanceVolume_QueryPhase : uint8
{
    Strict,
    Permissive
};

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_CrowdAvoidanceVolume"))
class CKCROWD_API UCk_Utils_CrowdAvoidanceVolume_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_CrowdAvoidanceVolume_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_CrowdAvoidanceVolume);

public:
    // Composes a static physical OBB, local-steering probe, and policy-selected finite-cost nav-area markup.
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|CrowdAvoidanceVolume",
        DisplayName = "[Ck][CrowdAvoidanceVolume] Add Feature")
    static FCk_Handle_CrowdAvoidanceVolume
    Add(
        UPARAM(ref) FCk_Handle_Transform& InOwner,
        const FCk_Fragment_CrowdAvoidanceVolume_ParamsData& InParams);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|CrowdAvoidanceVolume",
        DisplayName = "[Ck][CrowdAvoidanceVolume] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|CrowdAvoidanceVolume",
        DisplayName = "[Ck][CrowdAvoidanceVolume] Get Is Navigation Confirmed")
    static bool
    Get_IsNavigationConfirmed(
        const FCk_Handle_CrowdAvoidanceVolume& InVolume);

    // A copied diagnostic view for debugger and PIE rendering. InAnyEntityInWorld only selects an ECS world; no
    // returned value retains that entity, a registry, an ECS handle, a fragment, or a UObject.
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|CrowdAvoidanceVolume",
        DisplayName = "[Ck][CrowdAvoidanceVolume] Get Debug Snapshots", meta = (DevelopmentOnly))
    static TArray<FCk_CrowdAvoidanceVolume_DebugSnapshot>
    Get_DebugSnapshots(
        const FCk_Handle& InAnyEntityInWorld);

    // Returns true only while this ECS world has a live, painted AvoidIfPossible volume. Invalid or
    // teardown selectors fail closed without an ensure, because debugger and PIE callers may race teardown.
    static bool
    Get_HasAvoidIfPossibleVolumes(
        const FCk_Handle& InAnyEntityInWorld);

    // Pure policy contract shared by overlay construction and unit coverage. Invalid enum values
    // fail closed as excluded.
    static bool
    Get_IsTraversalPolicyExcluded(
        ECk_CrowdAvoidanceVolume_TraversalPolicy InTraversalPolicy,
        ECk_CrowdAvoidanceVolume_QueryPhase InPhase);

    // Phase-specific Recast exclusions. Strict excludes AvoidIfPossible and HardExclude; Permissive
    // excludes HardExclude only. CostOnly is never excluded. Requests own this value and CkNavigation
    // applies it to a private copy of the host filter.
    static FCk_Nav_QueryFilterOverlay
    Get_NavQueryFilterOverlay(
        ECk_CrowdAvoidanceVolume_QueryPhase InPhase);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|CrowdAvoidanceVolume",
        DisplayName = "[Ck][CrowdAvoidanceVolume] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_CrowdAvoidanceVolume
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|CrowdAvoidanceVolume",
        DisplayName = "[Ck][CrowdAvoidanceVolume] Handle -> Crowd Avoidance Volume Handle",
        meta = (CompactNodeTitle = "<AsCrowdAvoidanceVolume>", BlueprintAutocast))
    static FCk_Handle_CrowdAvoidanceVolume
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid Crowd Avoidance Volume Handle",
        Category = "Ck|Utils|CrowdAvoidanceVolume",
        meta = (CompactNodeTitle = "INVALID_CrowdAvoidanceVolumeHandle", Keywords = "make"))
    static FCk_Handle_CrowdAvoidanceVolume
    Get_InvalidHandle() { return {}; };
};

// --------------------------------------------------------------------------------------------------------------------
