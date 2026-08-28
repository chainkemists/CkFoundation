#pragma once

#include "CkCrowd/AvoidanceVolume/CkCrowdAvoidanceVolume_Fragment_Data.h"
#include "CkNavigation/Nav/CkNav_Fragment_Data.h"
#include "CkEcsExt/Transform/CkTransform_Fragment_Data.h"

#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkCrowdAvoidanceVolume_Utils.generated.h"

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_CrowdAvoidanceVolume"))
class CKCROWD_API UCk_Utils_CrowdAvoidanceVolume_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_CrowdAvoidanceVolume_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_CrowdAvoidanceVolume);

public:
    // Composes a static physical OBB, local-steering probe, and finite-cost nav-area markup.
    // Crowd query overlays exclude the marked area after Recast confirms its rebuild.
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

    // Canonical permanent exclusion for every Crowd path phase and provider. This is C++ only:
    // requests own the value and CkNavigation applies it to a private copy of the host filter.
    static FCk_Nav_QueryFilterOverlay
    Get_NavQueryFilterOverlay();

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
