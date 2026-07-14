#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkSettings/ProjectSettings/CkProjectSettings.h"

#include "CkPathNetwork_ProjectSettings.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(meta = (DisplayName = "Path Network"))
class CKPATHNETWORK_API UCk_PathNetwork_ProjectSettings_UE : public UCk_Plugin_ProjectSettings_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_PathNetwork_ProjectSettings_UE);

private:
    // Cap on route plans drained per processor tick. Each plan is an A* over the augmented graph
    // plus (on acceptance) one navmesh query per off-path leg.
    UPROPERTY(Config, EditDefaultsOnly, Category = "Routing",
        meta = (AllowPrivateAccess = true, ClampMin = 0, UIMin = 0, ClampMax = 64, UIMax = 64))
    int32 _MaxRouteQueriesPerFrame = 4;

    // How many entry/exit candidates (edge projections) to generate around the start AND around
    // the goal. Higher = better exit selection, larger search.
    UPROPERTY(Config, EditDefaultsOnly, Category = "Routing",
        meta = (AllowPrivateAccess = true, ClampMin = 1, UIMin = 1, ClampMax = 16, UIMax = 16))
    int32 _GoalCandidateCount = 5;

    // Initial radius (cm) of the candidate search around start/goal; doubles until enough edges
    // are found or _CandidateSearchMaxDoublings is reached.
    UPROPERTY(Config, EditDefaultsOnly, Category = "Routing",
        meta = (AllowPrivateAccess = true, ClampMin = 100.0, UIMin = 100.0))
    float _CandidateSearchRadius = 3000.0f;

    UPROPERTY(Config, EditDefaultsOnly, Category = "Routing",
        meta = (AllowPrivateAccess = true, ClampMin = 0, UIMin = 0, ClampMax = 8, UIMax = 8))
    int32 _CandidateSearchMaxDoublings = 3;

    // An off-path leg whose real (navmesh) length exceeds euclidean x this tolerance gets
    // repriced and the search re-runs — catches "shortcuts" through walls.
    UPROPERTY(Config, EditDefaultsOnly, Category = "Routing",
        meta = (AllowPrivateAccess = true, ClampMin = 1.0, UIMin = 1.0, ClampMax = 5.0, UIMax = 5.0))
    float _RepriceTolerance = 1.5f;

    UPROPERTY(Config, EditDefaultsOnly, Category = "Routing",
        meta = (AllowPrivateAccess = true, ClampMin = 0, UIMin = 0, ClampMax = 4, UIMax = 4))
    int32 _MaxRepriceIterations = 2;

public:
    CK_PROPERTY_GET(_MaxRouteQueriesPerFrame);
    CK_PROPERTY_GET(_GoalCandidateCount);
    CK_PROPERTY_GET(_CandidateSearchRadius);
    CK_PROPERTY_GET(_CandidateSearchMaxDoublings);
    CK_PROPERTY_GET(_RepriceTolerance);
    CK_PROPERTY_GET(_MaxRepriceIterations);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKPATHNETWORK_API UCk_Utils_PathNetwork_Settings_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_PathNetwork_Settings_UE);

public:
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|PathNetwork|Settings")
    static int32 Get_MaxRouteQueriesPerFrame();

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|PathNetwork|Settings")
    static int32 Get_GoalCandidateCount();

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|PathNetwork|Settings")
    static float Get_CandidateSearchRadius();

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|PathNetwork|Settings")
    static int32 Get_CandidateSearchMaxDoublings();

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|PathNetwork|Settings")
    static float Get_RepriceTolerance();

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|PathNetwork|Settings")
    static int32 Get_MaxRepriceIterations();
};

// --------------------------------------------------------------------------------------------------------------------
