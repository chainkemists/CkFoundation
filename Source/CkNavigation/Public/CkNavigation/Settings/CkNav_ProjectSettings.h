#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkSettings/ProjectSettings/CkProjectSettings.h"

#include <GameplayTagContainer.h>
#include <Templates/SubclassOf.h>

#include "CkNav_ProjectSettings.generated.h"

class UNavigationQueryFilter;

// --------------------------------------------------------------------------------------------------------------------

UCLASS(meta = (DisplayName = "Navigation"))
class CKNAVIGATION_API UCk_Nav_ProjectSettings_UE : public UCk_Plugin_ProjectSettings_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Nav_ProjectSettings_UE);

private:
    UPROPERTY(Config, EditDefaultsOnly, Category = "Pathfinding",
        meta = (AllowPrivateAccess = true, ClampMin = 0, UIMin = 0, ClampMax = 64, UIMax = 64,
            ToolTip = "Maximum FindPath calls drained per processor tick. Round-robined across pending requests."))
    int32 _MaxPathQueriesPerFrame = 8;

    UPROPERTY(Config, EditDefaultsOnly, Category = "Pathfinding",
        meta = (AllowPrivateAccess = true, ClampMin = 50.0, UIMin = 50.0, ClampMax = 5000.0, UIMax = 5000.0,
            ToolTip = "Half-extent (cm) used to project Start/End onto the navmesh. Default 500cm covers the rental-store geometry; bump higher for steep slopes or thin navmesh."))
    float _NavQuerySearchHalfExtent = 500.0f;

    UPROPERTY(Config, EditDefaultsOnly, Category = "Pathfinding",
        meta = (AllowPrivateAccess = true, ClampMin = -1.0, UIMin = -1.0, ClampMax = 5000.0, UIMax = 5000.0,
            ToolTip = "Vertical (Z) half-extent (cm) for the projection box. -1 (default) = use the horizontal extent (uniform cube, today's behavior). >= 0 opts in to an asymmetric XY/XY/Z box."))
    float _NavQueryVerticalHalfExtent = -1.0f;

    // Keyed by FCk_Request_Nav_FindPath::_QueryFilter; unmapped/empty -> NavData's default filter.
    UPROPERTY(Config, EditDefaultsOnly, Category = "Pathfinding",
        meta = (AllowPrivateAccess = true))
    TMap<FGameplayTag, TSoftClassPtr<UNavigationQueryFilter>> _QueryFilters;

public:
    CK_PROPERTY_GET(_MaxPathQueriesPerFrame);
    CK_PROPERTY_GET(_NavQuerySearchHalfExtent);
    CK_PROPERTY_GET(_NavQueryVerticalHalfExtent);
    CK_PROPERTY_GET(_QueryFilters);
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

    // Raw value; may be negative — the "unset" sentinel meaning "use the horizontal extent".
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Nav|Settings")
    static float Get_NavQueryVerticalHalfExtent();

    // The folded projection box, sentinel resolved. Every probe that must agree with
    // FindPathSync's internal projection builds its box from THIS, never the raw scalar cube.
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Nav|Settings")
    static FVector Get_NavQueryProjectionExtentVec();

    // Empty tag or no mapping -> null class (callers fall back to NavData's default filter);
    // a non-empty tag with no mapping additionally fires an ensure.
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Nav|Settings")
    static TSubclassOf<UNavigationQueryFilter> Get_QueryFilterClass(const FGameplayTag& InFilterTag);
};

// --------------------------------------------------------------------------------------------------------------------
