#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkPathNetwork/Network/CkPathNetwork_Types.h"

#include <CoreMinimal.h>

#include "CkPathNetwork_Detector.generated.h"

// --------------------------------------------------------------------------------------------------------------------
// Per-game sidewalk detection. The framework never knows HOW paths are marked in a level — a game
// subclasses this (C++, Blueprint, or AngelScript) and rasterizes its own source of truth (e.g.
// landscape paint-layer weights) into an occupancy mask. Everything downstream (vectorize -> build
// -> route -> follow) is game-agnostic.
//
// The detector must be runtime-callable if the game wants runtime rebuilds; editor-only detectors
// are fine for games that only bake in-editor.
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
