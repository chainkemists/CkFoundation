#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkPathNetwork/Network/CkPathNetwork_Types.h"

#include <CoreMinimal.h>

#include "CkPathNetwork_Detector.generated.h"

// --------------------------------------------------------------------------------------------------------------------
// Per-game sidewalk detection: a game subclasses this (C++/BP/AS) and rasterizes its own source of
// truth (e.g. landscape paint weights) into an occupancy mask, keeping everything downstream
// (vectorize -> build -> route -> follow) game-agnostic. Runtime rebuilds need a runtime-callable subclass.
// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, Blueprintable, BlueprintType, EditInlineNew, DefaultToInstanced)
class CKPATHNETWORK_API UCk_PathNetwork_Detector_UE : public UObject
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_PathNetwork_Detector_UE);

public:
    // Rasterize path/sidewalk coverage inside InWorldBounds. Return an invalid mask (SizeX/Y == 0)
    // to report "nothing detected".
    UFUNCTION(BlueprintNativeEvent,
              Category = "Ck|PathNetwork|Detector",
              DisplayName = "[Ck][PathNetwork] Get Detection Mask")
    FCk_PathNetwork_DetectionMask
    Get_DetectionMask(
        const FBox& InWorldBounds) const;
};

// --------------------------------------------------------------------------------------------------------------------
