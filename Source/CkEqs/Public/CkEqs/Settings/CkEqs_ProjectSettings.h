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
    // GLOBAL per-frame budget across ALL active queries in this registry.
    // 0 = disabled-with-warn (NOT unbounded — use Request_RunQuery_Immediate for synchronous work).
    // 256 keeps 60 fps comfortable: 256 candidates × 2-3 physics queries each fits the frame.
    UPROPERTY(Config, EditDefaultsOnly, Category = "Eqs",
        meta = (AllowPrivateAccess = true, ClampMin = 0, UIMin = 0, ClampMax = 4096, UIMax = 4096,
            ToolTip = "GLOBAL per-frame candidate-evaluation budget across all active queries. 0 disables the deferred path entirely (one Warning per query rejection). Provisional default 256; tune against the gym reference scene."))
    int32 _MaxCandidatesPerFrame = 256;

    // Hard-cap on Request_RunQuery_Immediate to prevent accidental main-thread DoS.
    // Generators producing more than this number of candidates have the list truncated with a Warning.
    UPROPERTY(Config, EditDefaultsOnly, Category = "Eqs",
        meta = (AllowPrivateAccess = true, ClampMin = 1, UIMin = 1, ClampMax = 16384, UIMax = 16384,
            ToolTip = "Hard cap on candidates evaluated by Request_RunQuery_Immediate. Larger generators are truncated with a Warning. Bound on synchronous main-thread work."))
    int32 _MaxCandidates_ImmediatePathHardCap = 1024;

    // v1.1: fallback agent radius used when a PathCost query is issued from an entity that does
    // NOT have FCk_Fragment_CrowdAgent_ParamsData. The crowd agent radius takes precedence when
    // present; this value applies only when there is no crowd agent on the querier.
    // Used as InAgentRadiusForFirstSkip in FCk_Nav_Algorithm::FindPathSync — controls the first-
    // waypoint skip threshold (skip if agent is within 2×radius of it). 0 disables the skip.
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
