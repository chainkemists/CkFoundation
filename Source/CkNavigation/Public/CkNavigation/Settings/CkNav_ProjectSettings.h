#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkSettings/ProjectSettings/CkProjectSettings.h"

#include "CkNav_ProjectSettings.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(meta = (DisplayName = "Navigation"))
class CKNAVIGATION_API UCk_Nav_ProjectSettings_UE : public UCk_Plugin_ProjectSettings_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Nav_ProjectSettings_UE);

private:
    // Cap on FindPathSync invocations per processor tick. Keeps the per-frame budget
    // bounded under replan storms / mass-spawn scenarios. 0 disables (returns
    // BudgetExceeded for every queued request).
    UPROPERTY(Config, EditDefaultsOnly, Category = "Pathfinding",
        meta = (AllowPrivateAccess = true, ClampMin = 0, UIMin = 0, ClampMax = 64, UIMax = 64,
            ToolTip = "Maximum FindPath calls drained per processor tick. Round-robined across pending requests."))
    int32 _MaxPathQueriesPerFrame = 8;

    // Half-extent (cm) for the projection lookup that snaps Start/End to navmesh
    // before issuing the path query. Larger values are more forgiving but slower.
    UPROPERTY(Config, EditDefaultsOnly, Category = "Pathfinding",
        meta = (AllowPrivateAccess = true, ClampMin = 50.0, UIMin = 50.0, ClampMax = 5000.0, UIMax = 5000.0,
            ToolTip = "Half-extent (cm) used to project Start/End onto the navmesh. Default 500cm covers the rental-store geometry; bump higher for steep slopes or thin navmesh."))
    float _NavQuerySearchHalfExtent = 500.0f;

public:
    CK_PROPERTY_GET(_MaxPathQueriesPerFrame);
    CK_PROPERTY_GET(_NavQuerySearchHalfExtent);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKNAVIGATION_API UCk_Utils_Nav_Settings_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Nav_Settings_UE);

public:
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Nav|Settings")
    static int32 Get_MaxPathQueriesPerFrame();

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Nav|Settings")
    static float Get_NavQuerySearchHalfExtent();
};

// --------------------------------------------------------------------------------------------------------------------
