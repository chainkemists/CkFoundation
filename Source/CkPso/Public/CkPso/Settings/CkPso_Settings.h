#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Time/CkTime.h"

#include "CkSettings/ProjectSettings/CkProjectSettings.h"

#include <Kismet/BlueprintFunctionLibrary.h>
#include <Misc/Optional.h>

#include "CkPso_Settings.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(meta = (DisplayName = "PSO"))
class CKPSO_API UCk_Pso_ProjectSettings_UE : public UCk_Plugin_ProjectSettings_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Pso_ProjectSettings_UE);

private:
    /**
     * Hold the loading screen until pending PSO work (the bundled pipeline cache plus runtime precache requests)
     * drains. Bounded by _DrainMaxWait and _DrainStallTimeout, so a cold driver cache can only lengthen the screen,
     * never wedge it. Runtime override: ck.Pso.WaitForPrecache (-1 = use this setting, 0 = off, 1 = on).
     */
    UPROPERTY(Config, EditDefaultsOnly, Category = "Drain",
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _WaitForPsoPrecache = ECk_EnableDisable::Enable;

    /** How long the pending count must sit at zero before the window completes; requests trickle in as components register. */
    UPROPERTY(Config, EditDefaultsOnly, Category = "Drain",
              meta = (AllowPrivateAccess = true))
    FCk_Time _DrainQuietPeriod = FCk_Time{0.25};

    /** Fail-open budget for one drain window, re-openings included. Zero disables the check. */
    UPROPERTY(Config, EditDefaultsOnly, Category = "Drain",
              meta = (AllowPrivateAccess = true))
    FCk_Time _DrainMaxWait = FCk_Time{60.0};

    /**
     * Fail-open when the pending count has not changed for this long. A rising count is work still arriving and is
     * bounded by _DrainMaxWait instead. Zero disables the check.
     */
    UPROPERTY(Config, EditDefaultsOnly, Category = "Drain",
              meta = (AllowPrivateAccess = true))
    FCk_Time _DrainStallTimeout = FCk_Time{10.0};

public:
    CK_PROPERTY_GET(_WaitForPsoPrecache);
    CK_PROPERTY_GET(_DrainQuietPeriod);
    CK_PROPERTY_GET(_DrainMaxWait);
    CK_PROPERTY_GET(_DrainStallTimeout);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKPSO_API UCk_Utils_Pso_Settings_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Pso_Settings_UE);

public:
    /** The effective gate: ck.Pso.WaitForPrecache when it is 0 or 1, the project setting otherwise. */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Pso|Settings",
              DisplayName = "[Ck][Pso] Get Wait For Pso Precache")
    static ECk_EnableDisable
    Get_WaitForPsoPrecache();

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Pso|Settings",
              DisplayName = "[Ck][Pso] Get Drain Quiet Period")
    static FCk_Time
    Get_DrainQuietPeriod();

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Pso|Settings",
              DisplayName = "[Ck][Pso] Get Drain Max Wait")
    static FCk_Time
    Get_DrainMaxWait();

    /** The effective stall timeout: ck.Pso.Debug.StallTimeoutOverride when it is >= 0 (never in Shipping), the project setting otherwise. */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Pso|Settings",
              DisplayName = "[Ck][Pso] Get Drain Stall Timeout")
    static FCk_Time
    Get_DrainStallTimeout();

public:
    /**
     * ck.Pso.Debug.SimulatedRemaining when it is >= 0, unset otherwise. A set value replaces the engine's pending
     * count; it is the headless test seam and the only way to drive a progress bar in PIE. Always unset in Shipping.
     */
    static auto
    Get_SimulatedRemaining() -> TOptional<uint32>;

    static auto
    Get_LogDrainEveryFrame() -> bool;
};

// --------------------------------------------------------------------------------------------------------------------
