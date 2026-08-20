#pragma once

#include "CoreMinimal.h"

#include <Subsystems/EngineSubsystem.h>

#include "CkPixelArtRenderer_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class FCk_PixelArt_ViewExtension;

// --------------------------------------------------------------------------------------------------------------------

// Owns the pixel-art scene view extension for the whole process. Engine-scoped rather than world-scoped because a
// scene view extension is registered with the renderer, not with a world, and it must survive world transitions —
// the extension's own activation gate is what decides which views it applies to.
UCLASS(NotBlueprintable, DisplayName = "CkSubsystem_PixelArtRenderer")
class CKPIXELARTRENDERER_API UCk_PixelArtRenderer_Subsystem_UE : public UEngineSubsystem
{
    GENERATED_BODY()

public:
    auto Initialize(
        FSubsystemCollectionBase& InCollection) -> void override;

    auto Deinitialize() -> void override;

private:
    TSharedPtr<FCk_PixelArt_ViewExtension, ESPMode::ThreadSafe> _ViewExtension;
};
