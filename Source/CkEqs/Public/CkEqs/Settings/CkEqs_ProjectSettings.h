#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkSettings/ProjectSettings/CkProjectSettings.h"

#include "CkEqs_ProjectSettings.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(meta = (DisplayName = "Eqs"))
class CKEQS_API UCk_Eqs_ProjectSettings_UE : public UCk_Plugin_ProjectSettings_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Eqs_ProjectSettings_UE);

private:
    // The 256 default is 256 candidates × 2-3 physics queries each, sized to fit a 60 fps frame.
    UPROPERTY(Config, EditDefaultsOnly, Category = "Eqs",
        meta = (AllowPrivateAccess = true, ClampMin = 0, UIMin = 0, ClampMax = 4096, UIMax = 4096,
            ToolTip = "GLOBAL per-frame candidate-evaluation budget across all active queries. 0 disables the deferred path entirely (one Warning per query rejection). Provisional default 256; tune against the gym reference scene."))
    int32 _MaxCandidatesPerFrame = 256;

    UPROPERTY(Config, EditDefaultsOnly, Category = "Eqs",
        meta = (AllowPrivateAccess = true, ClampMin = 1, UIMin = 1, ClampMax = 16384, UIMax = 16384,
            ToolTip = "Hard cap on candidates evaluated by Request_RunQuery_Immediate. Larger generators are truncated with a Warning. Bound on synchronous main-thread work."))
    int32 _MaxCandidates_ImmediatePathHardCap = 1024;

    // Only consulted when the querier has no FCk_CrowdAgent_Spec (the agent's own
    // radius wins). Feeds InAgentRadiusForFirstSkip in FCk_Nav_Algorithm::FindPathSync.
    UPROPERTY(Config, EditDefaultsOnly, Category = "Eqs|PathCost",
        meta = (AllowPrivateAccess = true, ClampMin = 0.0,
            ToolTip = "Fallback agent radius (cm) for PathCost queries on entities without a crowd agent component. 0 disables the first-waypoint skip."))
    float _DefaultPathCostAgentRadius = 0.0f;

public:
    CK_PROPERTY_GET(_MaxCandidatesPerFrame);
    CK_PROPERTY_GET(_MaxCandidates_ImmediatePathHardCap);
    CK_PROPERTY_GET(_DefaultPathCostAgentRadius);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKEQS_API UCk_Utils_Eqs_Settings_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Eqs_Settings_UE);

public:
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Eqs|Settings")
    static int32 Get_MaxCandidatesPerFrame();

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Eqs|Settings")
    static int32 Get_MaxCandidates_ImmediatePathHardCap();

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Eqs|Settings")
    static float Get_DefaultPathCostAgentRadius();
};

// --------------------------------------------------------------------------------------------------------------------
