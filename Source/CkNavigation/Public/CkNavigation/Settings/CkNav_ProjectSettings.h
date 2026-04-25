#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkSettings/Public/CkSettings/ProjectSettings/CkProjectSettings.h"

#include <CoreMinimal.h>

#include "CkNav_ProjectSettings.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(meta = (DisplayName = "Navigation"))
class CKNAVIGATION_API UCk_Nav_ProjectSettings_UE : public UCk_Plugin_ProjectSettings_UE
{
    GENERATED_BODY()

    CK_GENERATED_BODY(UCk_Nav_ProjectSettings_UE);

private:
    // Maximum number of nav agents that dtCrowd can hold simultaneously.
    UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Crowd",
              meta = (AllowPrivateAccess = true, ClampMin = 1, UIMin = 1, ClampMax = 512, UIMax = 512))
    int32 _MaxCrowdAgents = 64;

    // Must be >= the largest agent radius that will be registered (UE cm).
    UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Crowd",
              meta = (AllowPrivateAccess = true, ClampMin = 1.0f, UIMin = 1.0f))
    float _MaxAgentRadius = 200.0f;

    // Half-extents (UE cm) used when snapping a world position to the nearest navmesh poly.
    UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "PathFinding",
              meta = (AllowPrivateAccess = true, ClampMin = 1.0f, UIMin = 1.0f))
    float _NavQuerySearchHalfExtent = 250.0f;

    // V1-ENFORCED. Max FindPathSync invocations per frame across all agents.
    // 0 = disabled (warns at init). Default 8 keeps 60 fps comfortable under repath-storm conditions.
    UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "PathFinding",
              meta = (AllowPrivateAccess = true, ClampMin = 0, UIMin = 0))
    int32 _MaxPathQueriesPerFrame = 8;

    // CrowdPushPosition is a no-op if |entity - npos| <= this. Avoids corrupting
    // dtCrowd's path corridor with every-frame npos sprays.
    UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Crowd",
              meta = (AllowPrivateAccess = true, ClampMin = 0.0f, UIMin = 0.0f))
    float _TeleportThresholdUu = 10.0f;

    // P3-3 / Pass-3.1 N3: debounce window for full nav-mesh rebuilds (after pointer-equality check).
    // Coalesces streaming-tile rebuild bursts into one crowd re-allocation. Starting guess; tune in playtest.
    UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "PathFinding",
              meta = (AllowPrivateAccess = true, ClampMin = 0.0f, UIMin = 0.0f))
    float _NavRebuildDebounceSeconds = 0.5f;

public:
    CK_PROPERTY_GET(_MaxCrowdAgents);
    CK_PROPERTY_GET(_MaxAgentRadius);
    CK_PROPERTY_GET(_NavQuerySearchHalfExtent);
    CK_PROPERTY_GET(_MaxPathQueriesPerFrame);
    CK_PROPERTY_GET(_TeleportThresholdUu);
    CK_PROPERTY_GET(_NavRebuildDebounceSeconds);
};

// --------------------------------------------------------------------------------------------------------------------

class CKNAVIGATION_API UCk_Utils_Nav_ProjectSettings
{
public:
    static auto Get_MaxCrowdAgents() -> int32;
    static auto Get_MaxAgentRadius() -> float;
    static auto Get_NavQuerySearchHalfExtent() -> float;
    static auto Get_MaxPathQueriesPerFrame() -> int32;
    static auto Get_TeleportThresholdUu() -> float;
    static auto Get_NavRebuildDebounceSeconds() -> float;
};

// --------------------------------------------------------------------------------------------------------------------
